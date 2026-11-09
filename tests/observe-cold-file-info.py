"""Watch and confirm File Info on an isolated production HLS fixture."""

import ctypes
from ctypes import wintypes
import hashlib
import json
import os
from pathlib import Path
import re
import sys
import time


root = Path(sys.argv[1]).resolve()
exe = Path(sys.argv[2]).resolve()
sidecar = exe.with_name("udm-data.json")
if not (root / ".udm-live-file-info-fixture").is_file() or not exe.is_file() or not sidecar.is_file():
    raise SystemExit("Expected a private cold File Info fixture.")
if Path(json.loads(sidecar.read_text(encoding="utf-8"))["dataDirectory"]).resolve() != root / "state":
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
            if (title.value == "Download File Info" or title.value.startswith("UDM ")) and belongs_to_fixture(window):
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


print("ready", flush=True)
actionable = 0
deadline = time.monotonic() + 65
while time.monotonic() < deadline:
    visible = windows()
    assert not any(title.startswith("UDM ") for _, title in visible), "The main UDM list became visible."
    dialogs = [window for window, title in visible if title == "Download File Info"]
    assert len(dialogs) <= 1, "More than one File Info dialog opened."
    if dialogs:
        items = controls(dialogs[0])
        start = items.get("&Start Download")
        if start and user.IsWindowEnabled(start):
            actionable = actionable or epoch_ms()
            for label in items:
                exact = re.fullmatch(r"Exact final size: (\d+) bytes", label)
                if not exact:
                    continue
                observed = epoch_ms()
                catalog = json.loads((root / "state" / "state.json").read_text(encoding="utf-8-sig"))
                assert len(catalog["Downloads"]) == 1
                job = catalog["Downloads"][0]
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
                }
                # BM_CLICK on the verified fixture button only.
                if not user.PostMessageW(start, 0x00F5, 0, 0):
                    raise OSError(ctypes.get_last_error(), "Could not press the fixture Start button")
                (root / "file-info-result.json").write_text(json.dumps(evidence), encoding="utf-8")
                raise SystemExit(0)
    time.sleep(0.015)

raise SystemExit("Timed out waiting for actionable File Info with exact size.")
