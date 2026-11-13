"""Verify pinned PAC routes, ordered failover and no implicit direct fallback."""
from __future__ import annotations

import argparse
import json
import socket
import time
from pathlib import Path

from bench import UdmClient, sha256_file
from origin import Options, Origin, expected_sha256


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--udm", type=Path, required=True)
    parser.add_argument("--work", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    executable = args.udm.resolve(strict=True)
    work, output = args.work.resolve(), args.out.resolve()
    if work == output or work in output.parents or output in work.parents or output.exists():
        parser.error("Use fresh independent work and output paths.")
    work.mkdir(parents=True, exist_ok=False)
    output.parent.mkdir(parents=True, exist_ok=True)
    records = []
    for name in ("proxy-only", "ordered-failover", "no-implicit-direct", "windows-local-bypass"):
        root = work / name
        root.mkdir()
        folder = root / "downloads"
        folder.mkdir()
        # Absolute-form requests prove the payload was served by the proxy.
        target = "http://udm-route-only.invalid/bench/file.bin"
        options = Options(size=1024 * 1024, path=target)
        origin = Origin(options)
        dead = socket.socket()
        dead.bind(("127.0.0.1", 0))
        unavailable = f"127.0.0.1:{dead.getsockname()[1]}"
        endpoint = f"127.0.0.1:{origin.port}"
        route = f"PROXY {endpoint}"
        if name == "ordered-failover":
            route = f"PROXY {unavailable}; " + route
        elif name == "no-implicit-direct":
            # A made-up direct alternative would fail DNS instead of returning
            # the selected local proxy's connection-refusal error (12029).
            route = f"PROXY {unavailable}"
        elif name == "windows-local-bypass":
            # The Windows resolver explicitly returns fBypass for a local
            # destination even when the PAC script names only a proxy.
            options.path = "/bench/file.bin"
            target = f"http://127.0.0.1:{origin.port}/bench/file.bin"
            route = f"PROXY {unavailable}"
        script = "function FindProxyForURL(url,host){return " + json.dumps(route) + ";}"
        client = UdmClient(executable, root, {"ProxyMode": "Use automatic configuration script",
                           "ProxyAutoConfigScript": script, "Retries": 0})
        path = folder / "payload.bin"
        process = None
        try:
            client.prepare(1, folder)
            process = client.start(target, folder, path.name)
            deadline = time.monotonic() + 45
            while time.monotonic() < deadline:
                job = client.job() or {}
                if job.get("Status") in ("Complete", "Failed"):
                    break
                time.sleep(0.1)
            job = client.job() or {}
            traffic = origin.snapshot()
            digest_ok = path.is_file() and sha256_file(path) == expected_sha256(options.size, options.seed)
            if name == "no-implicit-direct":
                passed = (job.get("Status") == "Failed" and traffic["requests"] == 0
                          and not path.exists() and "12029" in job.get("Error", ""))
            else:
                passed = job.get("Status") == "Complete" and digest_ok and traffic["requests"] == 1
            records.append({"case": name, "passed": passed, "status": job.get("Status", "Missing"),
                            "sha256_ok": digest_ok, "requests": traffic["requests"],
                            "bytes_sent": traffic["bytes_sent"], "error": job.get("Error", "")})
        finally:
            client.kill(process)
            origin.close()
            dead.close()
            path.unlink(missing_ok=True)
            for part in (client.data / "parts").rglob("*"):
                if part.is_file() and part.suffix in (".part", ".data"):
                    if not part.resolve().is_relative_to(work):
                        raise RuntimeError("Partial escaped the fresh fixture directory.")
                    part.unlink()
    result = {"passed": all(item["passed"] for item in records), "cases": records,
              "executable_sha256": sha256_file(executable), "temporary_payloads_removed": True}
    with output.open("x", encoding="utf-8") as receipt:
        json.dump(result, receipt, indent=2)
    print(json.dumps(result, indent=2), flush=True)
    return 0 if result["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
