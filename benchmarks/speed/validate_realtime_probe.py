"""Read-only acceptance gates for the direct realtime feasibility probe.

Synthetic checks validate this policy, not real kernel event decoding. A passing
self-process feasibility report never establishes UDM lifetime or root cause.
"""
from __future__ import annotations
import argparse
import copy
import hashlib
import json
import stat
from pathlib import Path

REPORT_BYTES = 1024 * 1024
PAYLOAD_BYTES = 8192
PAYLOAD_SHA256 = hashlib.sha256(bytes(PAYLOAD_BYTES)).hexdigest().upper()
ERROR_WMI_INSTANCE_NOT_FOUND = 4201
ERROR_CANCELLED = 1223
ERROR_CTX_CLOSE_PENDING = 7007

class Rejected(ValueError):
    pass

def require(condition, message):
    if not condition:
        raise Rejected(message)

def integer(value, minimum=0, maximum=None):
    return type(value) is int and value >= minimum and (maximum is None or value <= maximum)

def common(probe, expected_pid):
    require(isinstance(probe, dict), "Probe must be an object")
    require(probe.get("passed") is True and probe.get("started") is True,
            "Probe did not acknowledge a successful session")
    require(probe.get("targetPid") == expected_pid and integer(probe.get("targetPid"), 1, 0xFFFFFFFF),
            "Target PID does not match the observed helper")
    for key in ("startStatus", "openStatus", "stopStatus", "eventsLost", "realtimeBuffersLost", "targetSchemaErrors"):
        require(type(probe.get(key)) is int and probe[key] == 0, key + " must be exactly zero")
    require(type(probe.get("processTraceStatus")) is int and probe["processTraceStatus"] in (0, ERROR_CANCELLED),
            "Consumer returned an unsupported status")
    require(type(probe.get("closeTraceStatus")) is int and probe["closeTraceStatus"] in (0, ERROR_CTX_CLOSE_PENDING),
            "Consumer closure was not accepted")
    require(type(probe.get("postStopQueryStatus")) is int and probe["postStopQueryStatus"] == ERROR_WMI_INSTANCE_NOT_FOUND
            and probe.get("sessionAbsent") is True, "Owned session absence was not verified")
    require(integer(probe.get("elapsedMs"), 0, 15000), "Probe exceeded its acceptance deadline")
    require(probe.get("callbackFailed") is False and probe.get("error") == "", "Callback or workload failed")
    require(integer(probe.get("actualBuffers"), 1, 256) and type(probe.get("bufferSizeKiB")) is int and probe["bufferSizeKiB"] == 64,
            "Actual session buffers exceed the 16 MiB acceptance bound")
    require(integer(probe.get("decodedEvents"), 3), "Too few decoded events")
    require(probe.get("targetSelfIsElevated") is True, "Require the declared elevated self-process feasibility scope")
    require(probe.get("rootCauseVerified") is False and probe.get("fullTargetLifetimeVerified") is False,
            "Self-process feasibility cannot claim root cause or full lifetime")

def validate_report(report, expected_pid):
    require(integer(expected_pid, 1, 0xFFFFFFFF), "Require an independently observed helper PID")
    require(isinstance(report, dict) and report.get("passed") is True, "Probe run did not pass")
    require(report.get("feasibilityOnly") is True and report.get("kernelCauseVerified") is False
            and report.get("installerCreated") is False, "Unsupported scope or side effects")
    probes = report.get("probes")
    require(isinstance(probes, list) and len(probes) == 2, "Require delivery and kernel phases")
    delivery, kernel = probes
    common(delivery, expected_pid)
    common(kernel, expected_pid)
    require(delivery.get("kernel") is False and kernel.get("kernel") is True, "Wrong phase ordering")
    require(isinstance(delivery.get("probeSequences"), list) and all(type(value) is int for value in delivery["probeSequences"])
            and delivery["probeSequences"] == [1, 2, 3] and delivery.get("badSentinel") is False,
            "Exact delivery sequence/sentinel not verified")
    calls, pairs = kernel.get("flushCalls"), kernel.get("dataFlushPairs")
    require(isinstance(calls, list) and len(calls) == 2 and isinstance(pairs, list) and len(pairs) == 2,
            "Require exactly two native calls and two file-flush pairs")
    require(integer(kernel.get("counterFrequency"), 1, (1 << 63) - 1), "Missing counter frequency")
    previous_end = 0
    producer_thread = None
    durations = []
    for call, pair in zip(calls, pairs):
        require(isinstance(call, dict) and isinstance(pair, dict), "Invalid flush record")
        require(call.get("ok") is True and type(call.get("error")) is int and call["error"] == 0,
                "Native flush call failed")
        for value in (call.get("beginCounter"), call.get("endCounter"), pair.get("beginCounter"), pair.get("endCounter")):
            require(integer(value, 1, (1 << 63) - 1), "Missing or invalid raw timestamp")
        begin, end = call["beginCounter"], call["endCounter"]
        require(previous_end <= begin <= end, "Native calls overlap or reverse")
        require(begin <= pair["beginCounter"] <= pair["endCounter"] <= end,
                "File event interval is not contained in its actual native flush call")
        require(type(pair.get("status")) is int and pair["status"] == 0, "File operation did not succeed")
        require(pair.get("relativePath") == "probe.data", "File identity is not the exact owned payload")
        require(integer(pair.get("thread"), 1, 0xFFFFFFFF), "Missing issuing thread identity")
        if producer_thread is None:
            producer_thread = pair["thread"]
        require(pair["thread"] == producer_thread, "Sequential native calls have different issuing threads")
        previous_end = end
        durations.append((end - begin) * 1000 / kernel["counterFrequency"])
    return {"accepted": True, "scope": "Two self-process feasibility flushes only", "helperPid": expected_pid,
            "nativeFlushMs": durations, "nativeCallIntervalsContainFileEvents": True,
            "sessionShutdownVerified": True, "kernelCauseVerified": False, "fullTargetLifetimeVerified": False}

def validate_payload_bytes(data):
    require(len(data) == PAYLOAD_BYTES, "Probe payload length differs")
    digest = hashlib.sha256(data).hexdigest().upper()
    require(digest == PAYLOAD_SHA256, "Probe payload bytes differ")
    return {"payloadBytes": len(data), "payloadSha256": digest, "payloadVerified": True}

def parse_report_bytes(raw):
    require(len(raw) <= REPORT_BYTES, "Report exceeds 1 MiB bound")
    return json.loads(raw.decode("utf-8-sig"))

def read_report(path):
    info = path.lstat()
    require(stat.S_ISREG(info.st_mode) and not path.is_symlink()
            and not (getattr(info, "st_file_attributes", 0) & 0x400), "Report is not an ordinary file")
    with path.open("rb") as stream:
        raw = stream.read(REPORT_BYTES + 1)
    return parse_report_bytes(raw)

def read_payload(root):
    root = root.resolve(strict=True)
    path = root / "probe.data"
    info = path.lstat()
    require(stat.S_ISREG(info.st_mode) and not path.is_symlink()
            and not (getattr(info, "st_file_attributes", 0) & 0x400), "Payload is not an ordinary file")
    require(path.resolve(strict=True).parent == root and info.st_size == PAYLOAD_BYTES,
            "Payload escaped its declared root or has a different length")
    with path.open("rb") as stream:
        data = stream.read(PAYLOAD_BYTES + 1)
    return validate_payload_bytes(data)

def self_test():
    checks = []
    def check(value, name):
        require(value, name)
        checks.append({"name": name, "passed": True})
    base = {"passed": True, "started": True, "targetPid": 321, "startStatus": 0, "openStatus": 0,
            "stopStatus": 0, "closeTraceStatus": 0, "processTraceStatus": 0,
            "postStopQueryStatus": ERROR_WMI_INSTANCE_NOT_FOUND, "sessionAbsent": True,
            "eventsLost": 0, "realtimeBuffersLost": 0, "targetSchemaErrors": 0,
            "elapsedMs": 1000, "callbackFailed": False, "error": "", "actualBuffers": 32,
            "bufferSizeKiB": 64, "decodedEvents": 10, "rootCauseVerified": False,
            "fullTargetLifetimeVerified": False, "targetSelfIsElevated": True}
    delivery = dict(base, kernel=False, probeSequences=[1, 2, 3], badSentinel=False)
    kernel = dict(base, kernel=True, counterFrequency=10000,
                  flushCalls=[{"beginCounter": 100, "endCounter": 200, "ok": True, "error": 0},
                              {"beginCounter": 300, "endCounter": 400, "ok": True, "error": 0}],
                  dataFlushPairs=[{"beginCounter": 110, "endCounter": 190, "thread": 7, "relativePath": "probe.data", "status": 0},
                                  {"beginCounter": 310, "endCounter": 390, "thread": 7, "relativePath": "probe.data", "status": 0}])
    report = {"passed": True, "feasibilityOnly": True, "kernelCauseVerified": False,
              "installerCreated": False, "probes": [delivery, kernel]}
    check(validate_report(report, 321)["accepted"], "A complete synthetic feasibility record is accepted without a root-cause claim")
    mutations = [
        (lambda r: r.update(passed=False), "Failed run"),
        (lambda r: r.update(kernelCauseVerified=True), "Unproven root-cause claim"),
        (lambda r: r["probes"].pop(), "Missing kernel phase"),
        (lambda r: r["probes"].reverse(), "Reversed phase order"),
        (lambda r: r["probes"][0].update(probeSequences=[1, 1, 3]), "Duplicate delivery sequence"),
        (lambda r: r["probes"][0].update(badSentinel=True), "Wrong delivery sentinel"),
        (lambda r: r["probes"][0].update(probeSequences=[True, 2, 3]), "Boolean masquerading as a sequence"),
        (lambda r: r["probes"][1].update(targetPid=322), "Different target PID"),
        (lambda r: r["probes"][1].update(eventsLost=1), "Lost events"),
        (lambda r: r["probes"][1].update(eventsLost=False), "Boolean masquerading as zero error count"),
        (lambda r: r["probes"][1].update(realtimeBuffersLost=1), "Lost realtime buffer"),
        (lambda r: r["probes"][1].update(targetSchemaErrors=1), "Target schema failure"),
        (lambda r: r["probes"][1].update(callbackFailed=True), "Callback failure"),
        (lambda r: r["probes"][1].update(sessionAbsent=False), "Session still present"),
        (lambda r: r["probes"][1].update(stopStatus=5), "Stop refused"),
        (lambda r: r["probes"][1].update(closeTraceStatus=5), "Consumer close refused"),
        (lambda r: r["probes"][1].update(processTraceStatus=5), "Consumer failed"),
        (lambda r: r["probes"][1].update(elapsedMs=15001), "Deadline exceeded"),
        (lambda r: r["probes"][1].update(actualBuffers=257), "Actual buffer cap exceeded"),
        (lambda r: r["probes"][1].update(bufferSizeKiB=64.0), "Floating point buffer size"),
        (lambda r: r["probes"][1].update(counterFrequency=0), "Unknown timestamp frequency"),
        (lambda r: r["probes"][1].update(counterFrequency=1 << 63), "Out-of-range frequency"),
        (lambda r: r["probes"][1]["flushCalls"][0].update(beginCounter=1 << 63), "Out-of-range timestamp"),
        (lambda r: r["probes"][1].update(fullTargetLifetimeVerified=True), "Unproven full lifetime"),
        (lambda r: r["probes"][1].update(targetSelfIsElevated=False), "Different writer privilege scope"),
        (lambda r: r["probes"][1]["flushCalls"].pop(), "Missing native flush"),
        (lambda r: r["probes"][1]["dataFlushPairs"].pop(), "Missing file-flush pair"),
        (lambda r: r["probes"][1]["flushCalls"][0].update(ok=False), "Native flush failed"),
        (lambda r: r["probes"][1]["dataFlushPairs"][0].update(endCounter=201), "File event occurs after API returned"),
        (lambda r: r["probes"][1]["dataFlushPairs"][0].update(beginCounter=99), "File event predates API call"),
        (lambda r: r["probes"][1]["dataFlushPairs"][0].update(beginCounter=191), "Reversed event interval"),
        (lambda r: r["probes"][1]["dataFlushPairs"][0].update(relativePath="../probe.data"), "Foreign file path"),
        (lambda r: r["probes"][1]["dataFlushPairs"][0].update(status=1), "Failed file operation"),
        (lambda r: r["probes"][1]["dataFlushPairs"][1].update(thread=8), "Different issuing threads"),
        (lambda r: r["probes"][1]["flushCalls"][1].update(beginCounter=199), "Overlapping native calls"),
    ]
    for mutate, name in mutations:
        candidate = copy.deepcopy(report)
        mutate(candidate)
        rejected = False
        try:
            validate_report(candidate, 321)
        except Rejected:
            rejected = True
        check(rejected, name + " is rejected")
    check(validate_payload_bytes(bytes(PAYLOAD_BYTES))["payloadVerified"], "The exact deterministic payload passes")
    for data, name in [(bytes(PAYLOAD_BYTES - 1), "Short payload"), (bytes(PAYLOAD_BYTES + 1), "Oversized payload"),
                       (b"x" + bytes(PAYLOAD_BYTES - 1), "Wrong same-length payload")]:
        rejected = False
        try:
            validate_payload_bytes(data)
        except Rejected:
            rejected = True
        check(rejected, name + " is rejected")
    check(parse_report_bytes(json.dumps(report).encode()) == report, "Bounded report parsing retains the exact record")
    rejected = False
    try:
        parse_report_bytes(b"x" * (REPORT_BYTES + 1))
    except Rejected:
        rejected = True
    check(rejected, "Oversized actual report bytes are rejected before parsing")
    return {"passed": True, "checks": checks, "realTraceConsumed": False,
            "method": "Synthetic acceptance-policy and in-memory payload checks; no kernel-decoder or runtime validation"}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--report", type=Path)
    parser.add_argument("--expected-pid", type=int)
    args = parser.parse_args()
    if args.self_test:
        require(args.report is None and args.expected_pid is None, "Self-test takes no live input")
        result = self_test()
    else:
        require(args.report is not None and args.expected_pid is not None, "Provide a report and observed helper PID")
        result = validate_report(read_report(args.report), args.expected_pid)
        result.update(read_payload(args.report.parent))
    print(json.dumps(result, indent=2))

if __name__ == "__main__":
    try:
        main()
    except (Rejected, OSError, ValueError, KeyError, TypeError) as error:
        print(json.dumps({"accepted": False, "error": str(error), "kernelCauseVerified": False}))
        raise SystemExit(1)
