"""Exercise production HTTP hashing against an owned, independently hashed origin."""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "benchmarks" / "speed"))
from origin import Origin, Options, expected_sha256


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--output-root", type=Path, required=True)
    args = parser.parse_args()
    root = args.output_root.resolve()
    if root.exists():
        raise ValueError("A fresh isolated output root is required")
    root.mkdir(parents=True)
    digest = expected_sha256(8 * 1048576, 20261115)
    results = []
    for mode in ["plain", "verified", "mismatch", "ranged"]:
        origin = Origin(Options(size=8 * 1048576, seed=20261115, rate=8_000_000,
                                latency_ms=20, chunk=65521, no_range=mode != "ranged",
                                accept_ranges="bytes" if mode == "ranged" else "none"))
        try:
            with (root / (mode + ".log")).open("w") as log:
                process = subprocess.Popen([str(args.exe.resolve()), str(root / mode),
                                            origin.url, digest, mode], stdout=log,
                                           stderr=subprocess.STDOUT)
                (root / (mode + "-process.json")).write_text(json.dumps({"pid": process.pid}))
                # A timeout preserves evidence and leaves the owned process intact.
                code = process.wait(timeout=60)
            evidence = json.loads((root / mode / "results.json").read_text(encoding="utf-8-sig"))
            if code or not evidence["passed"]:
                raise RuntimeError(f"Inspect {mode} results and original process before retry")
            result = {"case": mode, "checks": len(evidence["checks"]), "passed": True,
                      "requests": origin.snapshot(), "nativeResults": evidence}
            results.append(result)
            print(json.dumps({"case": mode, "checks": result["checks"], "passed": True}), flush=True)
        finally:
            origin.close()
    (root / "results.json").write_text(json.dumps({
        "passed": True, "executableSha256": hashlib.sha256(args.exe.read_bytes()).hexdigest().upper(),
        "payloadSha256": digest, "payloadBytes": 8 * 1048576,
        "checks": sum(r["checks"] for r in results), "cases": results,
        "limits": "Correctness and path coverage, not a performance comparison."}, indent=2) + "\n")


if __name__ == "__main__":
    main()
