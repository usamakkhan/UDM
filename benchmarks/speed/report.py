"""Turn a results folder from bench.py into a Markdown table with phase timings.

    python benchmarks/speed/report.py benchmarks/speed/results/2026-10-18-before

Phases (loopback cases, from the origin's clock): ``startup`` is launch to the
first request, ``transfer`` is first request to the last connection close, and
``finish`` is the remainder of the wall time (assembly, verification,
publication). Public cases only have wall time and sampled TCP peaks.
"""
from __future__ import annotations
import json, sys
from pathlib import Path

MiB = 1024 * 1024
ORDER = ["single-8", "single-16", "uncapped-8", "resume-8", "outage-8", "drop-8", "no-range-1", "slow-start-2", "public-alpine-8", "public-kernel-8"]


def phases(r: dict) -> dict:
    o = r.get("origin")
    if not o or not o.get("request_log"):
        return {}
    launch = r.get("relaunch_at") or r.get("launch_at") or 0
    reqs = [q for q in o["request_log"] if q["at"] >= launch]
    conns = [c for c in o["connection_log"] if c["opened"] >= launch and not c["rejected"]]
    if not reqs:
        return {}
    first = min(q["at"] for q in reqs)
    last = max([c["closed"] or 0 for c in conns] + [q["finished"] or 0 for q in reqs])
    wall = r.get("resume_wall_seconds") or r.get("wall_seconds") or 0
    return {"startup": first - launch, "transfer": last - first, "finish": wall - (last - launch), "last": last}


def row(r: dict) -> str:
    o = r.get("origin") or {}
    tcp = r.get("tcp") or {}
    d = r.get("client_details") or {}
    p = phases(r)
    fmt = lambda v: "" if v is None or v == "" else (f"{v:.1f}" if isinstance(v, float) else str(v))
    wasted = o.get("wasted_bytes")
    note = (r.get("client_error") or "").strip()
    if r["client"] == "udm" and d.get("segments") is not None:
        note = (note + f" segments={d.get('segments')} splits={d.get('dynamic_splits')}").strip()
    if not r.get("completed"):
        note = ("FAILED " + note).strip()
    return "| {case} | {client} | {wall} | {thr} | {startup} | {transfer} | {finish} | {conns} | {peak} | {requests} | {rejected} | {drops} | {wasted} | {sha} | {note} |".format(
        case=r["case"], client=r["client"].upper(), wall=fmt(r.get("resume_wall_seconds") or r.get("wall_seconds")),
        thr=fmt(r.get("throughput_MBps")), startup=fmt(p.get("startup")), transfer=fmt(p.get("transfer")), finish=fmt(p.get("finish")),
        conns=o.get("connections", ""), peak=o.get("peak_concurrent", tcp.get("peak_concurrent", "")), requests=o.get("requests", ""),
        rejected=o.get("rejected_connections", ""), drops=o.get("drops", ""), wasted="" if wasted is None else f"{wasted / MiB:.1f}",
        sha="yes" if r.get("sha256_ok") else ("" if r.get("sha256_ok") is None else "NO"), note=note)


def main(folder: str):
    results = [json.loads(f.read_text(encoding="utf-8")) for f in sorted(Path(folder).glob("*.json"))]
    results.sort(key=lambda r: (ORDER.index(r["case"]) if r["case"] in ORDER else 99, r["client"]))
    print("| case | client | wall s | MB/s | startup s | transfer s | finish s | conns | peak | requests | rejected | drops | wasted MiB | sha | notes |")
    print("|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---|")
    for r in results:
        print(row(r))


if __name__ == "__main__":
    main(sys.argv[1])
