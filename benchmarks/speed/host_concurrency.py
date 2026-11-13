"""Check independent UDM jobs against a connection-limited loopback origin."""
from __future__ import annotations

import argparse
import json
import math
import subprocess
import threading
import time
from pathlib import Path

from bench import UdmClient, sha256_file
from origin import Options, Origin, expected_sha256


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--udm", type=Path, required=True)
    parser.add_argument("--work", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--jobs", type=int, default=3)
    parser.add_argument("--server-connections", type=int, default=2)
    parser.add_argument("--size-mib", type=int, default=8)
    parser.add_argument("--rate-kib", type=int, default=512)
    parser.add_argument("--timeout", type=float, default=60)
    parser.add_argument("--retries", type=int, default=3)
    parser.add_argument("--proxy-mode", choices=("direct", "windows"), default="direct")
    parser.add_argument("--lift-cap-after", type=float, default=0)
    args = parser.parse_args(argv)
    if not 1 <= args.jobs <= 32 or not 1 <= args.server_connections <= 32:
        parser.error("Use 1 to 32 jobs and server connections.")
    if not 1 <= args.size_mib <= 64 or args.rate_kib < 1 or not math.isfinite(args.timeout) or args.timeout <= 0:
        parser.error("Use 1 to 64 MiB, a positive rate and a positive timeout.")
    if not 0 <= args.retries <= 10 or not math.isfinite(args.lift_cap_after) or not 0 <= args.lift_cap_after < args.timeout:
        parser.error("Use 0 to 10 retries and a finite cap-removal delay below the timeout.")
    executable = args.udm.resolve(strict=True)
    executable_sha256 = sha256_file(executable)
    work, output = args.work.resolve(), args.out.resolve()
    if work == output or work in output.parents or output in work.parents:
        parser.error("Use separate work and output paths.")
    if output.exists():
        parser.error("Use a new output file.")
    work.mkdir(parents=True, exist_ok=False)
    folder = work / "downloads"
    folder.mkdir()
    output.parent.mkdir(parents=True, exist_ok=True)
    options = Options(size=args.size_mib * 1024 * 1024, rate=args.rate_kib * 1024,
                      max_conns=args.server_connections, latency_ms=20, chunk=16 * 1024)
    origin = Origin(options)
    proxy_mode = "Connect directly" if args.proxy_mode == "direct" else "Use Windows proxy / PAC settings"
    client = UdmClient(executable, work, {"Parallel": args.jobs, "ProxyMode": proxy_mode, "Retries": args.retries})
    processes = []
    started = time.monotonic()
    cap_removed = []
    def remove_cap():
        with origin.lock:
            options.max_conns = 0
            cap_removed.append(round(time.monotonic() - started, 3))
    timer = threading.Timer(args.lift_cap_after, remove_cap) if args.lift_cap_after else None
    expected_names = [f"job-{index}.bin" for index in range(1, args.jobs + 1)]
    try:
        client.prepare(1, folder)
        if timer:
            timer.start()
        # Keep the primary process alive until all jobs reach a terminal state.
        # /q belongs to its launching download and can stop the other jobs early.
        startup = subprocess.STARTUPINFO()
        startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
        startup.wShowWindow = 0
        for index, name in enumerate(expected_names, 1):
            command = [str(executable), "--background", "--instance-tag", client.tag,
                       "--data-dir", str(client.data), "/d", origin.url + f"?job={index}",
                       "/p", str(folder), "/f", name, "/n"]
            processes.append(subprocess.Popen(command, cwd=str(executable.parent), startupinfo=startup))
            if index == 1:
                deadline = time.monotonic() + min(15, args.timeout)
                while time.monotonic() < deadline and not origin.snapshot()["requests"]:
                    if client.failed():
                        break
                    time.sleep(0.05)
        deadline = started + args.timeout
        jobs = []
        while time.monotonic() < deadline:
            jobs = (client.state() or {}).get("Downloads", [])
            if len(jobs) == args.jobs and all(job.get("Status") in ("Complete", "Failed") for job in jobs):
                break
            time.sleep(0.1)
        digest = expected_sha256(options.size, options.seed)
        records = []
        by_name = {job["FileName"]: job for job in jobs}
        for name in expected_names:
            job = by_name.get(name, {})
            path = folder / name
            records.append({"name": name, "status": job.get("Status", "Missing"),
                            "received": job.get("Received", 0), "error": job.get("Error", ""),
                            "sha256_ok": sha256_file(path) == digest if path.is_file() else False})
        passed = len(jobs) == args.jobs and set(by_name) == set(expected_names) and all(
            record["status"] == "Complete" and record["sha256_ok"] for record in records)
        result = {"case": "independent-host-concurrency", "passed": passed,
                  "executable_sha256": executable_sha256,
                  "jobs_requested": args.jobs, "connections_per_job": 1,
                  "retries": args.retries, "cap_removed_after_seconds": cap_removed[0] if cap_removed else None,
                  "proxy_mode": args.proxy_mode,
                  "server_connections": args.server_connections, "bytes_per_file": options.size,
                  "rate_bytes_per_second": options.rate, "expected_sha256": digest,
                  "elapsed_seconds": round(time.monotonic() - started, 3),
                  "jobs": records, "origin": origin.snapshot()}
    finally:
        if timer:
            timer.cancel()
            if timer.ident is not None:
                timer.join()
        for process in processes:
            client.kill(process)
        origin.close()
        # This newly created directory contains only this fixture's outputs.
        # Retain the small private catalog for diagnosing a failed admission.
        for name in expected_names:
            (folder / name).unlink(missing_ok=True)
        for part in (client.data / "parts").rglob("*"):
            if not part.is_file() or part.suffix not in (".part", ".data"):
                continue
            if not part.resolve().is_relative_to(work):
                raise RuntimeError("Partial file escaped the fixture directory.")
            part.unlink()
    result["temporary_payloads_removed"] = True
    with output.open("x", encoding="utf-8") as receipt:
        json.dump(result, receipt, indent=2)
    print(json.dumps(result, indent=2), flush=True)
    return 0 if passed else 1


if __name__ == "__main__":
    raise SystemExit(main())
