"""Watch and confirm File Info on an isolated production HLS fixture."""

import ctypes
import argparse
from ctypes import wintypes
import hashlib
import json
import os
from pathlib import Path
import re
import sys
import time
import atexit


parser=argparse.ArgumentParser()
parser.add_argument('root',type=Path)
parser.add_argument('exe',type=Path)
parser.add_argument('--actionable-only',action='store_true')
parser.add_argument('--pid',type=int)
parser.add_argument('--retry-admission-once',action='store_true')
parser.add_argument('--retry-start-write-once',action='store_true')
args=parser.parse_args()
root = args.root.resolve()
exe = args.exe.resolve()
sidecar = exe.with_name("udm-data.json")
if not (root / ".udm-live-file-info-fixture").is_file() or not exe.is_file():
    raise SystemExit("Expected a private cold File Info fixture.")
if args.actionable_only:
    if not args.pid or exe.name!='Udm.CapturePresentationUiTests.exe':
        raise SystemExit('Expected the exact spawned component process.')
else:
    if not sidecar.is_file() or Path(json.loads(sidecar.read_text(encoding="utf-8"))["dataDirectory"]).resolve() != root / "state":
        raise SystemExit("Expected the fixture's private catalog.")

user = ctypes.WinDLL("user32", use_last_error=True)
kernel = ctypes.WinDLL("kernel32", use_last_error=True)
callback = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
user.EnumWindows.argtypes = [callback, wintypes.LPARAM]
user.EnumChildWindows.argtypes = [wintypes.HWND, callback, wintypes.LPARAM]
user.GetWindowThreadProcessId.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.DWORD)]
user.GetWindowTextW.argtypes = [wintypes.HWND, wintypes.LPWSTR, ctypes.c_int]
user.IsWindowVisible.argtypes = user.IsWindowEnabled.argtypes = [wintypes.HWND]
user.PostMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
user.SendMessageTimeoutW.argtypes = [
    wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM,
    wintypes.UINT, wintypes.UINT, ctypes.POINTER(ctypes.c_size_t),
]
user.SendMessageTimeoutW.restype = wintypes.LPARAM
kernel.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
kernel.OpenProcess.restype = wintypes.HANDLE
kernel.QueryFullProcessImageNameW.argtypes = [
    wintypes.HANDLE, wintypes.DWORD, wintypes.LPWSTR, ctypes.POINTER(wintypes.DWORD),
]
kernel.CloseHandle.argtypes = [wintypes.HANDLE]


def belongs_to_fixture(window):
    pid = wintypes.DWORD()
    user.GetWindowThreadProcessId(window, ctypes.byref(pid))
    if args.pid is not None and pid.value!=args.pid:
        return False
    process = kernel.OpenProcess(0x1000, False, pid)
    if not process:
        return False
    try:
        image = ctypes.create_unicode_buffer(32768)
        length = wintypes.DWORD(len(image))
        return (kernel.QueryFullProcessImageNameW(process, 0, image, ctypes.byref(length))
                and os.path.normcase(image.value) == os.path.normcase(str(exe)))
    finally:
        kernel.CloseHandle(process)


def windows():
    result = []

    @callback
    def visit(window, _):
        if user.IsWindowVisible(window):
            title = ctypes.create_unicode_buffer(256)
            user.GetWindowTextW(window, title, len(title))
            if (title.value in ("Download File Info", "UDM") or title.value.startswith("UDM ")) and belongs_to_fixture(window):
                result.append((window, title.value))
        return True

    user.EnumWindows(visit, 0)
    return result


def controls(window):
    result = {}

    @callback
    def visit(child, _):
        if user.IsWindowVisible(child):
            label = ctypes.create_unicode_buffer(256)
            returned = ctypes.c_size_t()
            # WM_GETTEXT reads standard controls in the other process.
            if user.SendMessageTimeoutW(child, 0x000D, len(label), ctypes.addressof(label),
                                        0x0002, 50, ctypes.byref(returned)) and label.value:
                result[label.value] = child
        return True

    user.EnumChildWindows(window, visit, 0)
    return result


def epoch_ms():
    return time.time_ns() // 1_000_000


barrier = root / 'state' / 'state.json.tmp'
if args.retry_admission_once or args.retry_start_write_once:
    if args.actionable_only or barrier.exists():
        raise SystemExit('Retry fault requires a fresh production catalog temp path.')
    if args.retry_admission_once:
        barrier.mkdir()
    def release_barrier():
        if barrier.is_dir() and not barrier.is_symlink():
            barrier.rmdir()  # Only our empty fixture barrier; never recursive.
    atexit.register(release_barrier)
print("ready", flush=True)
actionable = 0
catalog_read_retries = 0
retry_epoch = 0
retry_dialog = None
decision_retry_phase = 0
decision_dialog = None
decision_job = None
deadline = time.monotonic() + 65
while time.monotonic() < deadline:
    visible = windows()
    assert not any(title.startswith("UDM ") for _, title in visible), "The main UDM list became visible."
    dialogs = [window for window, title in visible if title == "Download File Info"]
    assert len(dialogs) <= 1, "More than one File Info dialog opened."
    if decision_retry_phase == 1:
        notices = [window for window, title in visible if title == 'UDM']
        if notices:
            assert len(notices) == 1
            notice = controls(notices[0])
            assert 'Cannot write state temporary file.' in notice, 'The expected decision persistence error was not reported.'
            release_barrier()
            assert user.PostMessageW(notice['OK'], 0x00F5, 0, 0)
            decision_retry_phase = 2
    if dialogs:
        if decision_retry_phase == 2 and (any(title == 'UDM' for _, title in visible) or not user.IsWindowEnabled(dialogs[0])):
            time.sleep(0.015)
            continue
        if decision_dialog:
            assert dialogs[0] == decision_dialog, 'The decision save error replaced File Info.'
        if retry_dialog:
            assert dialogs[0] == retry_dialog, 'Retry replaced the original File Info window.'
        items = controls(dialogs[0])
        if args.retry_admission_once and not retry_epoch:
            retry = items.get('&Retry')
            if retry and user.IsWindowEnabled(retry):
                catalog = json.loads((root / 'state' / 'state.json').read_text(encoding='utf-8-sig'))
                assert not catalog['Downloads'], 'The failed admission left a visible history row.'
                # Child enumeration is not an atomic UI snapshot: the status can
                # be read before the same pulse exposes its Retry button. Wait
                # within the existing deadline for both, without pressing Retry.
                if 'UDM could not accept this download. Your edits are retained; choose Retry.' not in items:
                    time.sleep(0.015)
                    continue
                retry_dialog = dialogs[0]
                release_barrier()
                retry_epoch = epoch_ms()
                if not user.PostMessageW(retry, 0x00F5, 0, 0):
                    raise OSError(ctypes.get_last_error(), 'Could not press the fixture Retry button')
        start = items.get("&Start Download")
        if start and user.IsWindowEnabled(start):
            if not actionable:
                actionable=epoch_ms()
                (root / 'external-actionable.json').write_text(json.dumps({'actionableEpochMs':actionable,'observer':'Exact executable and private catalog/PID guarded external observation','mainListVisible':False}),encoding='utf-8')
            if args.actionable_only:
                raise SystemExit(0)
            for label in items:
                exact = re.fullmatch(r"Exact final size: (\d+) bytes", label)
                if not exact:
                    continue
                observed = epoch_ms()
                try:
                    catalog_text = (root / "state" / "state.json").read_text(encoding="utf-8-sig")
                except (PermissionError, FileNotFoundError):
                    # An atomic catalog replacement may briefly prevent this
                    # observer's read. Retry through the existing outer deadline;
                    # keep malformed JSON and invariant failures fatal.
                    catalog_read_retries += 1
                    break
                catalog = json.loads(catalog_text)
                assert len(catalog["Downloads"]) == 1
                job = catalog["Downloads"][0]
                if decision_job:
                    assert job['Id'] == decision_job, 'A decision retry created a different download.'
                destination = Path(job["Folder"]) / job["FileName"]
                assert destination.parent == root / "downloads" and not destination.exists()
                staging = destination.with_name(f".udm-{job['Id']}.adaptive{destination.suffix}")
                size = int(exact[1])
                assert staging.stat().st_size == size
                with staging.open("rb") as stream:
                    digest = hashlib.file_digest(stream, "sha256").hexdigest()
                evidence = {
                    "actionableEpochMs": actionable, "exactVisibleEpochMs": observed,
                    "sizeBeforeStart": size, "sha256BeforeStart": digest,
                    "startEpochMs": epoch_ms(),
                    "catalogReadRetries": catalog_read_retries,
                }
                if args.retry_admission_once:
                    assert retry_epoch and dialogs[0] == retry_dialog
                    evidence.update(admissionRetried=True, sameDialogAfterRetry=True, retryEpochMs=retry_epoch, rejectedCatalogDownloads=0)
                if args.retry_start_write_once and decision_retry_phase == 0:
                    try:
                        barrier.mkdir()
                    except FileExistsError:
                        break  # A fixture writer still owns the temp name.
                    decision_job = job['Id']
                    decision_dialog = dialogs[0]
                    decision_retry_phase = 1
                    assert user.PostMessageW(start, 0x00F5, 0, 0)
                    break
                if args.retry_start_write_once:
                    if decision_retry_phase != 2:
                        break
                    assert not destination.exists()
                    evidence.update(decisionWriteRetried=True, sameDialogAfterDecisionFailure=True, sameJobAfterDecisionFailure=True)
                # BM_CLICK on the verified fixture button only.
                if not user.PostMessageW(start, 0x00F5, 0, 0):
                    raise OSError(ctypes.get_last_error(), "Could not press the fixture Start button")
                (root / "file-info-result.json").write_text(json.dumps(evidence), encoding="utf-8")
                raise SystemExit(0)
    time.sleep(0.015)

raise SystemExit("Timed out waiting for actionable File Info with exact size.")
