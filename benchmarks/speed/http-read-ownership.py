"""Real origin correctness/accounting checks; not normal App/IDM timing acceptance."""
import argparse
import json
import subprocess
import time
from pathlib import Path

from origin import Options, Origin, expected_sha256


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--output-root', type=Path, required=True)
    args = parser.parse_args()
    root = args.output_root.resolve()
    root.mkdir(parents=True, exist_ok=False)
    rows = []
    for mode, size in [('single', 4 << 20), ('split', 64 << 20), ('resume', 64 << 20)]:
        origin = Origin(Options(size=size, rate=8 << 20, chunk=64 << 10,
                                slow_stride=3, slow_rate=2 << 20))
        folder = root / mode
        try:
            digest = expected_sha256(size, origin.o.seed)
            start = time.perf_counter()
            process = subprocess.run([str(args.exe.resolve()), str(folder), origin.url,
                                      digest, str(size), mode], capture_output=True,
                                     text=True, timeout=90)
            lifetime = (time.perf_counter() - start) * 1000
            # The client has exited. Wait briefly for origin handler close/accounting.
            deadline = time.monotonic() + 3
            while time.monotonic() < deadline:
                with origin.lock:
                    active = origin.active
                if not active:
                    break
                time.sleep(.01)
            snapshot = origin.snapshot()
            result = json.loads((folder / 'results.json').read_text(encoding='utf-8-sig'))
            result.update(origin=snapshot, parentLifetimeIncludingProofMs=lifetime,
                          exitCode=process.returncode, stdout=process.stdout, stderr=process.stderr)
            ranges = [r for c in snapshot['detail'] for r in c['requests'] if r['range']]
            result['multiRangeObserved'] = len(ranges) > 1
            if mode != 'single' and not result['multiRangeObserved']:
                result['passed'] = False
                result['coverageFailure'] = 'Expected real multi-range requests'
            result['passed'] = result['passed'] and process.returncode == 0
            (root / (mode + '.json')).write_text(json.dumps(result, indent=2), encoding='utf-8')
            rows.append(dict(mode=mode, passed=result['passed'],
                             checks=len(result['checks']), bytesSent=snapshot['bytes_sent'],
                             requests=snapshot['requests']))
            print(json.dumps(rows[-1]), flush=True)
        finally:
            origin.close()
    (root / 'summary.json').write_text(json.dumps(dict(
        passed=all(r['passed'] for r in rows), cases=rows,
        scope='Engine correctness and wire accounting; no GUI, IDM, or speed acceptance.'
    ), indent=2), encoding='utf-8')
    return 0 if all(r['passed'] for r in rows) else 1


if __name__ == '__main__':
    raise SystemExit(main())
