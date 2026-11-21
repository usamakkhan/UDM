"""Bounded, named WPR collection; the benchmark always runs unelevated.

The controller prints a separate administrator recorder command. It never
elevates a download process or cancels a recording outside its random instance.
No recording starts during `plan` or module import.
"""
from __future__ import annotations

import argparse
import ctypes
from ctypes import wintypes as W
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import time
import uuid
import xml.etree.ElementTree as ET

HERE = Path(__file__).resolve().parent
PROFILE = HERE / "finish-io.wprp"
PROFILE_NAME = "UDMFinish.Verbose.Memory"
INSTANCE = re.compile(r"UDMFinish-[0-9a-f]{32}\Z")
CLEANUP_RESERVE = 15
CANCEL_RESERVE = 12


def sha(path):
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest().upper()


def read_json(path):
    return json.loads(Path(path).read_text(encoding="utf-8-sig"))


def write_json(path, data):
    path = Path(path)
    if path.is_symlink() or path.is_junction():
        raise RuntimeError("Refusing a linked status file")
    temp = path.with_name(path.name + ".tmp")
    with temp.open("x", encoding="utf-8") as stream:
        json.dump(data, stream, indent=2)
        stream.write("\n")
    os.replace(temp, path)


def local_path(path):
    path = Path(os.path.abspath(path))
    if not path.is_absolute() or str(path).startswith("\\\\"):
        raise ValueError("A local absolute path is required")
    for ancestor in [path, *path.parents]:
        if ancestor.is_symlink() or ancestor.is_junction():
            raise ValueError("Reparse paths are not allowed")
    return path


def validate_profile(path, expected):
    path = local_path(path)
    content = path.read_bytes()
    if len(content) > 65536 or hashlib.sha256(content).hexdigest().upper() != expected:
        raise ValueError("Profile length/hash mismatch")
    root = ET.fromstring(content)
    allowed = {"WindowsPerformanceRecorder", "Profiles", "SystemCollector",
               "BufferSize", "Buffers", "SystemProvider", "Keywords", "Keyword",
               "Stacks", "Stack", "Profile", "Collectors", "SystemCollectorId",
               "SystemProviderId"}
    if any(node.tag not in allowed for node in root.iter()):
        raise ValueError("Unexpected profile element")
    collector = root.findall("./Profiles/SystemCollector")
    if len(collector) != 1 or collector[0].find("BufferSize").get("Value") != "64" or collector[0].find("Buffers").get("Value") != "1024":
        raise ValueError("Unexpected system buffer configuration")
    variants = root.findall("./Profiles/Profile")
    if len(variants) != 2 or {p.get("LoggingMode") for p in variants} != {"Memory", "File"} or any(p.get("Name") != "UDMFinish" for p in variants):
        raise ValueError("Unexpected profile variants")
    keywords = [n.get("Value") for n in root.findall("./Profiles/SystemProvider/Keywords/Keyword")]
    stacks = [n.get("Value") for n in root.findall("./Profiles/SystemProvider/Stacks/Stack")]
    if set(keywords) != {"ProcessThread", "Loader", "CSwitch", "ReadyThread", "DiskIO", "DiskIOInit", "FileIO", "FileIOInit"} or len(keywords) != 8:
        raise ValueError("Unexpected kernel keywords")
    if set(stacks) != {"DiskReadInit", "DiskWriteInit", "DiskFlushInit", "FileFlush", "FileOpEnd"} or len(stacks) != 5:
        raise ValueError("Unexpected kernel stacks")
    by_mode = {p.get("LoggingMode"): p for p in variants}
    if by_mode["Memory"].get("Base") != "UDMFinish.Verbose.File" or by_mode["File"].get("Base") is not None:
        raise ValueError("Unexpected inherited profile")
    return path


def is_admin():
    return os.name == "nt" and bool(ctypes.windll.shell32.IsUserAnAdmin())


class PinnedProcess:
    """A live process handle plus its creation time, never PID-only ownership."""
    def __init__(self, pid, creation=None):
        if os.name != "nt":
            raise RuntimeError("Windows process handles are required")
        self.k = ctypes.WinDLL("kernel32", use_last_error=True)
        self.k.OpenProcess.argtypes = [W.DWORD, W.BOOL, W.DWORD]
        self.k.OpenProcess.restype = W.HANDLE
        self.k.GetProcessTimes.argtypes = [W.HANDLE, *([ctypes.POINTER(W.FILETIME)] * 4)]
        self.k.WaitForSingleObject.argtypes = [W.HANDLE, W.DWORD]
        self.k.WaitForSingleObject.restype = W.DWORD
        self.k.CloseHandle.argtypes = [W.HANDLE]
        self.handle = self.k.OpenProcess(0x100000 | 0x1000, False, pid)
        if not self.handle:
            raise ctypes.WinError(ctypes.get_last_error())
        times = [W.FILETIME() for _ in range(4)]
        if not self.k.GetProcessTimes(self.handle, *(ctypes.byref(t) for t in times)):
            self.close()
            raise ctypes.WinError(ctypes.get_last_error())
        self.creation = str((times[0].dwHighDateTime << 32) | times[0].dwLowDateTime)
        if creation is not None and self.creation != str(creation):
            self.close()
            raise RuntimeError("The process identity changed")

    def alive(self):
        state = self.k.WaitForSingleObject(self.handle, 0)
        if state == 0xFFFFFFFF:
            raise ctypes.WinError(ctypes.get_last_error())
        return state == 258

    def close(self):
        if self.handle:
            self.k.CloseHandle(self.handle)
            self.handle = None

    def __enter__(self):
        return self

    def __exit__(self, *unused):
        self.close()


class WprBackend:
    def __init__(self, root, profile, profile_hash):
        if os.name != "nt":
            raise RuntimeError("Windows is required")
        self.root = root
        self.profile = validate_profile(profile, profile_hash)
        self.wpr = str(Path(os.environ["SystemRoot"]) / "System32/wpr.exe")
        self.logman = str(Path(os.environ["SystemRoot"]) / "System32/logman.exe")
        self.commands = []
        self.dropped_commands = 0
        self.verified_kernel = False

    def remember(self, row):
        if len(self.commands) >= 256:
            self.commands.pop(0)
            self.dropped_commands += 1
        self.commands.append(row)

    def command(self, args, timeout=5):
        try:
            p = subprocess.run(args, stdin=subprocess.DEVNULL, capture_output=True,
                               text=True, errors="replace", timeout=timeout,
                               creationflags=subprocess.CREATE_NO_WINDOW)
            row = {"args": args, "exitCode": p.returncode, "output": p.stdout,
                   "error": p.stderr, "timedOut": False}
        except subprocess.TimeoutExpired as error:
            row = {"args": args, "exitCode": None, "timedOut": True,
                   "output": str(error.stdout or ""), "error": str(error.stderr or "")}
        self.remember(row)
        return row

    def preflight(self, instance):
        sessions = self.command([self.logman, "query", "-ets"])
        if sessions["exitCode"] != 0 or sessions["timedOut"]:
            raise RuntimeError("Cannot enumerate existing tracing sessions")
        if re.search(r"WPR_initiated_|NT Kernel Logger", sessions["output"], re.I):
            raise RuntimeError("Existing recording left untouched; inspect it before collecting")
        details = self.command([self.wpr, "-profiledetails",
                                str(self.profile) + "!UDMFinish.Verbose", "-instancename", instance])
        if details["exitCode"] != 0 or PROFILE_NAME not in details["output"] or ("WPR_initiated_" + instance) not in details["output"]:
            raise RuntimeError("Named memory-profile validation failed")

    def start_owned(self, instance):
        result = self.command([self.wpr, "-start", str(self.profile) + "!UDMFinish.Verbose",
                               "-instancename", instance])
        if result["exitCode"] != 0 or result["timedOut"]:
            raise RuntimeError("WPR start failed or timed out")
        if not self.kernel_present(instance):
            raise RuntimeError("The owned kernel session did not confirm startup")
        self.verified_kernel = True

    def kernel_present(self, instance):
        result = self.command([self.logman, "query", "-ets"], timeout=2)
        if result["exitCode"] != 0 or result["timedOut"]:
            raise RuntimeError("Kernel session enumeration is unverified")
        return ("WPR_initiated_" + instance + "_").lower() in result["output"].lower()

    def owns_instance(self, instance):
        if self.kernel_present(instance):
            return True
        if self.verified_kernel:
            return False  # Collection stopped even if WPR is still merging ETL.
        result = self.command([self.wpr, "-status", "profiles", "collectors", "-details",
                               "-instancename", instance], timeout=2)
        if result["exitCode"] == 0 and not result["timedOut"]:
            if PROFILE_NAME in result["output"] and ("WPR_initiated_" + instance) in result["output"]:
                return True
            if "WPR is not recording" in result["output"]:
                return False
        raise RuntimeError("Recorder state is unverified; do not assume it stopped")

    def cancel_owned(self, instance):
        # The random instance is mandatory even after a partial/failed start.
        return self.command([self.wpr, "-cancel", "-instancename", instance])

    def stop_export(self, instance, output, stop_deadline):
        if time.monotonic() >= stop_deadline:
            raise TimeoutError("No stop/export budget remains; owned cancellation is required")
        args = [self.wpr, "-stop", str(output), "UDM owned finish attribution",
                "-skipPdbGen", "-compress", "-instancename", instance]
        p = subprocess.Popen(args, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE, text=True,
                             creationflags=subprocess.CREATE_NO_WINDOW)
        inactive_at = None
        try:
            while p.poll() is None:
                now = time.monotonic()
                if inactive_at is None:
                    try:
                        if self.owns_instance(instance) is False:
                            inactive_at = time.monotonic()
                    except RuntimeError:
                        pass
                    if inactive_at is None and now >= stop_deadline:
                        raise TimeoutError("Recording did not confirm stop within cleanup budget")
                elif now - inactive_at >= 60:
                    raise TimeoutError("Recording stopped but ETL export did not finish")
                time.sleep(.05)
            stdout, stderr = p.communicate(timeout=2)
            if inactive_at is None and self.owns_instance(instance) is False:
                inactive_at = time.monotonic()
            self.remember({"args": args, "exitCode": p.returncode,
                                  "output": stdout, "error": stderr, "timedOut": False})
            return {"inactiveAt": inactive_at,
                    "exportComplete": p.returncode == 0 and output.is_file() and output.stat().st_size > 0}
        except Exception:
            # Only this command's child handle is terminated, never a workload.
            if p.poll() is None:
                p.kill()
            stdout, stderr = p.communicate(timeout=3)
            self.remember({"args": args, "exitCode": p.returncode,
                                  "output": stdout, "error": stderr, "timedOut": True})
            raise


def record_session(backend, instance, output, owner_alive, finish_requested,
                   ready, max_seconds=45, clock=time.monotonic, sleep=time.sleep):
    if not INSTANCE.fullmatch(instance) or not 20 <= max_seconds <= 45:
        raise ValueError("Invalid instance or capture budget")
    attempted = started = False
    began = inactive_at = None
    reason = "preflight-failed"
    exported = stopped = False
    errors = []
    try:
        if not owner_alive():
            raise RuntimeError("Controller exited before capture")
        backend.preflight(instance)
        if not owner_alive():
            raise RuntimeError("Controller exited during preflight")
        began = clock()
        attempted = True
        backend.start_owned(instance)
        started = True
        ready()
        while True:
            if not owner_alive():
                reason = "controller-exited"
                break
            if finish_requested():
                reason = "controller-complete"
                break
            if clock() >= began + max_seconds - CLEANUP_RESERVE:
                reason = "deadline"
                break
            sleep(.05)
    except Exception as error:
        reason = "error-after-start" if started else "start-failed" if attempted else "preflight-failed"
        errors.append(str(error))
    finally:
        if attempted:
            if started:
                try:
                    outcome = backend.stop_export(instance, output,
                                                  began + max_seconds - CANCEL_RESERVE)
                    inactive_at = outcome.get("inactiveAt")
                    exported = outcome.get("exportComplete", False)
                    if not exported:
                        errors.append("ETL export is incomplete; retain partial evidence")
                except Exception as error:
                    errors.append(str(error))
            try:
                stopped = backend.owns_instance(instance) is False
            except Exception as error:
                errors.append(str(error))
            if not stopped:
                try:
                    backend.cancel_owned(instance)
                    stopped = backend.owns_instance(instance) is False
                except Exception as error:
                    errors.append(str(error))
            if inactive_at is None and stopped:
                inactive_at = clock()
    seconds = inactive_at - began if inactive_at is not None and began is not None else None
    bounded = stopped and seconds is not None and seconds <= max_seconds
    return {"instance": instance, "startAttempted": attempted, "started": started,
            "recordingAttemptQpcSeconds": began,
            "recordingStopped": stopped, "recordingObservedSeconds": seconds,
            "budgetSeconds": max_seconds, "bounded": bounded,
            "exportComplete": exported, "stopReason": reason, "errors": errors,
            "captureComplete": started and stopped and bounded and exported and
                               reason == "controller-complete" and not errors}


def request_root(value):
    root = local_path(value)
    base = local_path(Path(os.environ["LOCALAPPDATA"]) / "UDM-Development")
    if not root.is_relative_to(base) or root == base:
        raise ValueError("Capture output must be inside the private UDM-Development root")
    return root


def load_request(root, expected):
    content = (root / "request.json").read_bytes()
    if len(content) > 65536 or hashlib.sha256(content).hexdigest().upper() != expected:
        raise ValueError("Request length/hash mismatch")
    request = json.loads(content)
    if request.get("schema") != 1 or request.get("outputRoot") != str(root) or not INSTANCE.fullmatch(request.get("instance", "")):
        raise ValueError("Invalid capture request")
    if not 20 <= request.get("maxSeconds", 0) <= 45:
        raise ValueError("Invalid capture budget")
    return request


def recover_owned(backend, instance, attention, sleep=time.sleep):
    """Keep a live supervisor until the exact owned instance confirms absent."""
    if not INSTANCE.fullmatch(instance):
        raise ValueError("Invalid recovery instance")
    attention()
    while True:
        try:
            backend.cancel_owned(instance)
            if backend.owns_instance(instance) is False:
                return
        except Exception:
            pass
        sleep(2)


def recorder(args):
    if not is_admin():
        raise RuntimeError("Only the recorder needs an administrator terminal")
    root = request_root(args.request_root)
    request = load_request(root, args.request_sha256)
    if local_path(request["profile"]) != PROFILE:
        raise ValueError("Only the repository's validated profile is allowed")
    with (root / "recorder.lock").open("x"):
        pass  # Never restart an old request or infer a prior recorder has stopped.
    backend = WprBackend(root, request["profile"], request["profileSha256"])
    with PinnedProcess(request["controllerPid"], request["controllerCreation"]) as owner:
        with PinnedProcess(os.getpid()) as self_pin:
            result = record_session(
                backend, request["instance"], root / "trace.etl", owner.alive,
                lambda: (root / "finish.request").is_file(),
                lambda: write_json(root / "recorder-ready.json", {
                    "pid": os.getpid(), "creation": self_pin.creation,
                    "instance": request["instance"]}), request["maxSeconds"])
        # Remain an identifiable live supervisor if Windows refuses cleanup.
        # Never exit silently while the owned recorder is known/unverified active.
        if result["startAttempted"] and not result["recordingStopped"]:
            recover_owned(backend, request["instance"], lambda: write_json(
                root / "recorder-attention.json", result | {"pid": os.getpid(),
                "snapshotMayBeStale": True}))
            result["recordingStopped"] = True
            result["recordingObservedSeconds"] = time.monotonic() - result["recordingAttemptQpcSeconds"]
            result["bounded"] = False
            result["captureComplete"] = False
    write_json(root / "recorder-result.json", result | {"commands": backend.commands,
               "droppedCommandRecords": backend.dropped_commands,
               "eventQualityVerified": False, "attributionVerified": False})
    return 0 if result["captureComplete"] else 1


def controller(args, plan_only=False):
    if os.name != "nt":
        raise RuntimeError("Windows is required")
    if not plan_only and is_admin():
        raise RuntimeError("Run the controller unelevated; elevate only the recorder")
    exe = local_path(args.udm)
    if sha(exe) != args.expected_udm_sha256.upper():
        raise ValueError("Executable hash mismatch")
    validate_profile(PROFILE, sha(PROFILE))
    root = request_root(args.output_root)
    if root.exists():
        raise ValueError("A fresh output root is required")
    request = {"schema": 1, "outputRoot": str(root), "instance": "UDMFinish-" + uuid.uuid4().hex,
               "profile": str(PROFILE), "profileSha256": sha(PROFILE),
               "maxSeconds": args.max_seconds, "controllerPid": os.getpid()}
    with PinnedProcess(os.getpid()) as self_pin:
        request["controllerCreation"] = self_pin.creation
    workload = local_path(args.workload_output_root) if getattr(args, "workload_output_root", None) else root / "workload"
    allowed = [local_path(HERE.parent.parent / ".media-cache"),
               local_path(Path(os.environ["LOCALAPPDATA"]) / "UDM-Development")]
    if (not any(workload.is_relative_to(base) and workload != base for base in allowed)
            or workload == root or root.is_relative_to(workload) or workload.exists()):
        raise ValueError("Workload output requires a fresh child of the owned development/cache roots")
    request["workloadOutputRoot"] = str(workload)
    measurement = HERE / "compare-completion.py"
    measurement_source = None
    if getattr(args, "idm_first", False):
        original = measurement.read_text(encoding="utf-8")
        before = "(['udm','idm']if(trial+(case=='single-64m'))%2 else['idm','udm'])"
        if original.count(before) != 1 or original.count("from origin import ") != 1:
            raise ValueError("Measurement helper changed; trace-order amendment needs review")
        measurement_source = original.replace(before, "(['idm','udm'])").replace(
            "from origin import ", "sys.path.insert(0, " + repr(str(HERE)) + ")\nfrom origin import ")
        compile(measurement_source, "owned-trace-order.py", "exec")
        request["measurementOrder"] = "IDM then UDM, attribution only"
        request["originalMeasurementSha256"] = sha(measurement)
        request["stagedMeasurementSha256"] = hashlib.sha256(measurement_source.encode("utf-8")).hexdigest().upper()
        measurement = root / "owned-trace-order.py"
    command = [sys.executable, str(measurement), "--udm", str(exe),
               "--expected-udm-sha256", args.expected_udm_sha256.upper(), "--output-root",
               str(workload), "--label", args.label, "--trials", "1", "--case",
               args.case, "--flush-after-exit"]
    if plan_only:
        WprBackend(root, PROFILE, request["profileSha256"]).preflight(request["instance"])
        print(json.dumps({"recordingStarted": False, "controllerElevated": is_admin(),
                          "request": request, "workloadCommand": command}, indent=2))
        return 0
    root.mkdir(parents=True)
    if measurement_source is not None:
        measurement.write_text(measurement_source, encoding="utf-8", newline="")
    write_json(root / "request.json", request)
    request_sha = sha(root / "request.json")
    literal = lambda value: "'" + str(value).replace("'", "''") + "'"
    admin_command = "& " + " ".join(literal(x) for x in [sys.executable, __file__, "recorder",
                                                       "--request-root", root,
                                                       "--request-sha256", request_sha])
    print(json.dumps({"requestRoot": str(root), "administratorRecorderCommand": admin_command,
                      "workloadNotStarted": True}), flush=True)
    until = time.monotonic() + 300  # Approval wait only; no collector is active yet.
    while not (root / "recorder-ready.json").exists():
        if (root / "recorder-result.json").exists():
            raise RuntimeError("Recorder ended before readiness; inspect its result")
        if time.monotonic() >= until:
            raise TimeoutError("No live recorder became ready; no workload started")
        time.sleep(.1)
    ready = read_json(root / "recorder-ready.json")
    if ready.get("instance") != request["instance"]:
        raise RuntimeError("Recorder identity mismatch")
    with PinnedProcess(ready["pid"], ready["creation"]) as guard:
        if not guard.alive():
            raise RuntimeError("Recorder exited before the workload")
        code = None
        workload_error = None
        try:
            if measurement_source is not None and sha(measurement) != request["stagedMeasurementSha256"]:
                raise RuntimeError("Staged trace measurement helper changed")
            workload = subprocess.Popen(command)
            with PinnedProcess(workload.pid) as child:
                write_json(root / "workload-process.json", {"pid": workload.pid,
                           "creation": child.creation, "elevated": False})
                code = workload.wait()  # Never interrupt a download to meet trace budget.
        except Exception as error:
            workload_error = str(error)
        finally:
            write_json(root / "finish.request", {"instance": request["instance"],
                       "workloadExitCode": code, "workloadError": workload_error})
        until = time.monotonic() + 80
        while guard.alive() and time.monotonic() < until:
            time.sleep(.1)
        if guard.alive():
            raise TimeoutError("The pinned recorder is still live; inspect it before retrying")
    result = read_json(root / "recorder-result.json")
    write_json(root / "controller-result.json", {
        "workloadExitCode": code, "captureComplete": result["captureComplete"],
        "workloadPassed": code == 0,
        "collectionLifecyclePassed": result["captureComplete"],
        "eventQualityVerified": False, "attributionVerified": False,
        "recordingStopped": result["recordingStopped"], "instance": request["instance"],
        "workloadElevated": False, "etlPrivate": True, "workloadError": workload_error})
    return 0 if code == 0 and result["captureComplete"] else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="action", required=True)
    for name in ["plan", "run"]:
        p = sub.add_parser(name)
        p.add_argument("--output-root", type=Path, required=True)
        p.add_argument("--workload-output-root", type=Path,
                       help="Fresh workload folder in the project cache/private development area; recorder stays private")
        p.add_argument("--idm-first", action="store_true",
                       help="Stage a private attribution-only helper with UDM last, preserving recent UDM events")
        p.add_argument("--udm", type=Path, required=True)
        p.add_argument("--expected-udm-sha256", required=True)
        p.add_argument("--label", required=True)
        p.add_argument("--case", choices=["single-4m", "single-64m", "split-64m"], required=True)
        p.add_argument("--max-seconds", type=int, choices=range(20, 46), default=45)
    p = sub.add_parser("recorder")
    p.add_argument("--request-root", type=Path, required=True)
    p.add_argument("--request-sha256", required=True)
    args = parser.parse_args()
    try:
        return recorder(args) if args.action == "recorder" else controller(args, args.action == "plan")
    except Exception as error:
        print(json.dumps({"error": str(error), "action": args.action}), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
