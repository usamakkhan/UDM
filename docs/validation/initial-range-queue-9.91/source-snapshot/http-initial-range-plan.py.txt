"""Verify bounded fresh ranges and fallback using independent owned HTTP origins."""
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
    cases = [
        ('split-8', 64 << 20, 8, 'split', {}),
        ('split-3', 16 << 20, 3, 'split', {}),
        ('unadvertised', 16 << 20, 4, 'split', dict(accept_ranges='')),
        ('connection-limit', 16 << 20, 4, 'split', dict(max_conns=2)),
        ('range-ignored', 16 << 20, 4, 'split', dict(no_range=True, accept_ranges='bytes')),
        ('primary-ahead', 16 << 20, 4, 'split', dict(rate=0, latency_ms=150)),
        ('resume-8', 64 << 20, 8, 'resume', {}),
    ]
    rows = []
    for name, size, connections, mode, changes in cases:
        options = dict(size=size, rate=8 << 20, chunk=64 << 10)
        options.update(changes)
        origin = Origin(Options(**options))
        folder = root / name
        try:
            digest = expected_sha256(size, origin.o.seed)
            begun = time.perf_counter()
            process = subprocess.Popen([str(args.exe.resolve()), str(folder), origin.url,
                                        digest, str(size), mode, str(connections)],
                                       stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
            (root / (name + '-process.json')).write_text(json.dumps(dict(pid=process.pid)))
            # Native driver has its own deadline. On an observation timeout,
            # preserve the original live process for inspection rather than rerun.
            stdout, stderr = process.communicate(timeout=90)
            lifetime = (time.perf_counter() - begun) * 1000
            deadline = time.monotonic() + 3
            while time.monotonic() < deadline:
                with origin.lock:
                    active = origin.active
                if not active:
                    break
                time.sleep(.01)
            result = json.loads((folder / 'results.json').read_text(encoding='utf-8-sig'))
            result.update(origin=origin.snapshot(), parentLifetimeIncludingProofMs=lifetime,
                          originHandlersActiveAtSnapshot=active,
                          exitCode=process.returncode, stdout=stdout, stderr=stderr)
            result['passed'] = result['passed'] and process.returncode == 0
            # Ordinary accepted helpers must have exact bounded initial ends.
            if name in ('split-8', 'split-3', 'unadvertised'):
                requests = [r for c in result['origin']['detail'] for r in c['requests']]
                # These fixtures fit a complete queue of 4 MiB pieces.
                first_end = 4 << 20
                second_end = (8 << 20) - 1
                bounded = any(r['status'] == 206 and r['start'] == first_end - 64
                              and r['end'] == second_end for r in requests)
                result['boundedInitialHelperObserved'] = bounded
                result['passed'] = result['passed'] and bounded
            (root / (name + '.json')).write_text(json.dumps(result, indent=2), encoding='utf-8')
            rows.append(dict(case=name, passed=result['passed'], checks=len(result['checks']),
                             originBytes=result['origin']['bytes_sent'], requests=result['origin']['requests']))
            print(json.dumps(rows[-1]), flush=True)
        finally:
            origin.close()
    summary = dict(passed=all(x['passed'] for x in rows), cases=rows,
                   scope='Owned engine correctness/fallback; not normal App/IDM performance acceptance.')
    (root / 'summary.json').write_text(json.dumps(summary, indent=2), encoding='utf-8')
    return 0 if summary['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
