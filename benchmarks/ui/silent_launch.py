"""Black-box silent launch failures against hash-pinned native App products.

Fresh owned outputs only. An optional older App is launched on an explicit
private desktop and stopped through its exact process handle after a bounded
expected timeout. No desktop is switched and no GUI input is injected.
"""
from pathlib import Path
import argparse
import ctypes
from ctypes import wintypes
import hashlib
import json
import subprocess
import time
import uuid
from owned_windows_process import OwnedProcess


def sha(path):
    with path.open('rb') as handle:
        return hashlib.file_digest(handle, 'sha256').hexdigest().upper()


def save(path, value):
    path.write_text(json.dumps(value, indent=2) + '\n', encoding='utf-8')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', type=Path, required=True)
    parser.add_argument('--expected-sha256', required=True)
    parser.add_argument('--output-root', type=Path, required=True)
    parser.add_argument('--baseline', type=Path)
    parser.add_argument('--expected-baseline-sha256')
    args = parser.parse_args()
    app = args.app.resolve()
    area = args.output_root.resolve()
    assert sha(app) == args.expected_sha256.upper()
    assert bool(args.baseline) == bool(args.expected_baseline_sha256)
    assert not area.exists(), 'Fresh output required'
    area.mkdir()
    (area / '.udm-silent-launch-owned').write_text('Owned silent launch cases\n')
    checks = []
    runs = []
    baseline = None
    failure = None

    def check(ok, message):
        checks.append(dict(name=message, passed=bool(ok)))
        if not ok:
            raise RuntimeError(message)

    try:
        if args.baseline:
            older = args.baseline.resolve()
            assert sha(older) == args.expected_baseline_sha256.upper()
            user = ctypes.WinDLL('user32', use_last_error=True)
            user.CreateDesktopW.argtypes = [wintypes.LPCWSTR, wintypes.LPCWSTR,
                ctypes.c_void_p, wintypes.DWORD, wintypes.DWORD, ctypes.c_void_p]
            user.CreateDesktopW.restype = wintypes.HANDLE
            user.CloseDesktop.argtypes = [wintypes.HANDLE]
            name = 'UDM-Silent-Baseline-' + uuid.uuid4().hex
            desktop = user.CreateDesktopW(name, None, None, 0, 0x1ff, None)
            if not desktop:
                raise ctypes.WinError(ctypes.get_last_error())
            process = None
            try:
                began = time.perf_counter()
                process = OwnedProcess([older, '--background', '--data-dir',
                    area / 'baseline-catalog', '--instance-tag', 'x' * 33, '/n'],
                    cwd=older.parent, desktop=name)
                save(area / 'baseline-process.json', dict(pid=process.pid,
                    binarySha256=sha(older), desktop=name))
                timed_out = False
                try:
                    process.wait(timeout=3)
                except subprocess.TimeoutExpired:
                    timed_out = True
                baseline = dict(timedOut=timed_out,
                    observedWaitMs=(time.perf_counter() - began) * 1000,
                    binarySha256=sha(older), explicitCreateProcessDesktop=name,
                    desktopSwitched=False, userInputInjected=False,
                    childDesktopAttested=False,
                    modalCauseInferredFromSourceNotVisualCapture=True)
                if process.poll() is None:
                    process.terminate()
                    process.wait(timeout=5)
                    baseline['ownedBlockedProcessStopped'] = True
                check(timed_out, 'The old silent invalid-tag launch does not exit within the bounded wait')
                check(not (area / 'baseline-catalog').exists(),
                    'The rejected old instance tag never opens a catalog')
            finally:
                if process is not None and process.poll() is None:
                    process.terminate()
                    process.wait(timeout=5)
                if not user.CloseDesktop(desktop):
                    raise ctypes.WinError(ctypes.get_last_error())
                if baseline is not None:
                    baseline['ownedDesktopClosed'] = True

        cases = [
            ('long-tag-last-quiet', ['--instance-tag', 'x' * 33, '/n'], 'Invalid instance tag'),
            ('long-tag-first-quiet', ['/n', '--instance-tag', 'x' * 33], 'Invalid instance tag'),
            ('tag-characters', ['--instance-tag', 'bad/tag', '--silent'], 'Invalid instance tag'),
            ('missing-folder', ['/n', '--folder'], 'Missing value for --folder'),
            ('unknown-option', ['--not-an-option', '--silent'], 'Unknown UDM option'),
            ('bad-wait-process', ['--silent', '--wait-process', 'not-a-pid'], 'Invalid process identifier'),
            ('missing-data', ['/n', '--data-dir'], 'Missing value for --data-dir'),
            ('missing-shortcut', ['/n', '--shortcut', str(area / 'absent.url')], 'Shortcut file was not found'),
            ('blocked-data', ['/n'], None),
            ('discarded-stderr', ['/n', '--instance-tag', 'x' * 33], None),
        ]
        for name, extra, fragment in cases:
            folder = area / name
            folder.mkdir()
            data = folder / 'catalog'
            obstruction = b'Owned data-path obstruction\n'
            if name == 'blocked-data':
                data.write_bytes(obstruction)
            command = [str(app), '--background', '--instance-tag', 'quiet-' + uuid.uuid4().hex[:12],
                '--data-dir', str(data), *extra]
            began = time.perf_counter()
            process = subprocess.Popen(command, cwd=app.parent, stdin=subprocess.DEVNULL,
                stdout=subprocess.PIPE, stderr=subprocess.DEVNULL if name == 'discarded-stderr'
                else subprocess.PIPE, creationflags=subprocess.CREATE_NO_WINDOW)
            save(folder / 'process.json', dict(pid=process.pid, sourceExecutableSha256=sha(app)))
            try:
                stdout, stderr = process.communicate(timeout=5)
            except subprocess.TimeoutExpired:
                # Only this newly created process handle is stopped. Never scan
                # or stop the user's app instances to recover a failed fixture.
                process.kill()
                process.communicate(timeout=5)
                raise RuntimeError('Silent request opened a blocking path: ' + name)
            elapsed = (time.perf_counter() - began) * 1000
            text = (stderr or b'').decode('utf-8', errors='strict')
            runs.append(dict(case=name, exitCode=process.returncode, elapsedMs=elapsed,
                stderr=text, stdout=stdout.decode('utf-8', errors='replace')))
            check(process.returncode == 1, 'A caught silent launch failure returns error status 1: ' + name)
            if name != 'discarded-stderr':
                check(text.startswith('UDM: ') and text.endswith('\r\n'),
                    'A redirected silent failure reports one complete diagnostic line: ' + name)
                if fragment:
                    check(fragment.lower() in text.lower(), 'The silent diagnostic identifies the actual failure: ' + name)
            if name == 'blocked-data':
                check(data.read_bytes() == obstruction, 'A storage startup failure preserves the obstructing file')
            else:
                check(not data.exists(), 'An invalid silent launch leaves its catalog unopened: ' + name)
        check(sha(app) == args.expected_sha256.upper(), 'The tested App remains the hash-pinned product')
    except Exception as error:
        failure = str(error)
    result = dict(passed=failure is None, checks=checks, error=failure, runs=runs,
        baseline=baseline, sourceExecutableSha256=sha(app), harnessSha256=sha(Path(__file__)),
        interactiveInputInjected=False)
    save(area / 'results.json', result)
    print(str(len(checks)) + ' black-box assertions; ' + ('passed' if failure is None else failure), flush=True)
    return 0 if failure is None else 1


if __name__ == '__main__':
    raise SystemExit(main())
