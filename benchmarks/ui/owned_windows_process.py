"""Launch a Windows test on an explicit desktop, without switching desktops.

subprocess.STARTUPINFO does not expose lpDesktop; assigning that Python
attribute does not populate the native structure. Use CreateProcessW directly.
"""
import ctypes
import os
import subprocess
import time
from pathlib import Path
from ctypes import wintypes


class StartupInfo(ctypes.Structure):
    _fields_ = [("cb", wintypes.DWORD), ("lpReserved", wintypes.LPWSTR),
                ("lpDesktop", wintypes.LPWSTR), ("lpTitle", wintypes.LPWSTR),
                *[(name, wintypes.DWORD) for name in
                  ["dwX", "dwY", "dwXSize", "dwYSize", "dwXCountChars",
                   "dwYCountChars", "dwFillAttribute", "dwFlags"]],
                ("wShowWindow", wintypes.WORD), ("cbReserved2", wintypes.WORD),
                ("lpReserved2", ctypes.POINTER(wintypes.BYTE)),
                ("hStdInput", wintypes.HANDLE), ("hStdOutput", wintypes.HANDLE),
                ("hStdError", wintypes.HANDLE)]


class ProcessInfo(ctypes.Structure):
    _fields_ = [("process", wintypes.HANDLE), ("thread", wintypes.HANDLE),
                ("pid", wintypes.DWORD), ("tid", wintypes.DWORD)]


kernel = ctypes.WinDLL("kernel32", use_last_error=True)
user = ctypes.WinDLL("user32", use_last_error=True)
kernel.CreateProcessW.argtypes = [wintypes.LPCWSTR, wintypes.LPWSTR,
    ctypes.c_void_p, ctypes.c_void_p, wintypes.BOOL, wintypes.DWORD,
    ctypes.c_void_p, wintypes.LPCWSTR, ctypes.POINTER(StartupInfo),
    ctypes.POINTER(ProcessInfo)]
kernel.CreateProcessW.restype = wintypes.BOOL
kernel.CloseHandle.argtypes = [wintypes.HANDLE]
kernel.WaitForSingleObject.argtypes = [wintypes.HANDLE, wintypes.DWORD]
kernel.WaitForSingleObject.restype = wintypes.DWORD
kernel.GetExitCodeProcess.argtypes = [wintypes.HANDLE, ctypes.POINTER(wintypes.DWORD)]
kernel.TerminateProcess.argtypes = [wintypes.HANDLE, wintypes.UINT]
kernel.SetErrorMode.argtypes = [wintypes.UINT]
kernel.SetErrorMode.restype = wintypes.UINT
user.GetProcessWindowStation.restype = wintypes.HANDLE
user.GetUserObjectInformationW.argtypes = [wintypes.HANDLE, ctypes.c_int,
    ctypes.c_void_p, wintypes.DWORD, ctypes.POINTER(wintypes.DWORD)]


def object_name(handle):
    text = ctypes.create_unicode_buffer(1024)
    needed = wintypes.DWORD()
    if not handle or not user.GetUserObjectInformationW(
            handle, 2, text, ctypes.sizeof(text), ctypes.byref(needed)):
        raise ctypes.WinError(ctypes.get_last_error())
    return text.value


class OwnedProcess:
    def __init__(self, args, *, cwd, desktop, attestation_file=None):
        if not desktop.startswith("UDM-") or "\\" in desktop or "/" in desktop:
            raise ValueError("An exact UDM test desktop is required")
        self.args = [str(value) for value in args]
        self.returncode = None
        self.process = None
        self.actual_desktop = None
        self.desktop_verified = False
        info = StartupInfo()
        info.cb = ctypes.sizeof(info)
        info.lpDesktop = object_name(user.GetProcessWindowStation()) + "\\" + desktop
        result = ProcessInfo()
        command = ctypes.create_unicode_buffer(subprocess.list2cmdline(self.args))
        self.expected_desktop = desktop
        environment = dict(os.environ, UDM_TEST_DESKTOP=desktop)
        if attestation_file is not None:
            if Path(attestation_file).exists():
                raise ValueError("A fresh desktop attestation is required")
            environment["UDM_TEST_DESKTOP_ATTESTATION"] = str(attestation_file)
        block = ctypes.create_unicode_buffer("\0".join(
            key + "=" + value for key, value in sorted(environment.items(),
                key=lambda row: row[0].upper())) + "\0\0")
        old_mode = kernel.SetErrorMode(0x8003)
        try:
            ok = kernel.CreateProcessW(self.args[0], command, None, None,
                False, 0x08000400, block, str(cwd), ctypes.byref(info),
                ctypes.byref(result))
            error = ctypes.get_last_error()
        finally:
            kernel.SetErrorMode(old_mode)
        if not ok:
            raise ctypes.WinError(error)
        self.process, self.pid, self.tid = result.process, result.pid, result.tid
        kernel.CloseHandle(result.thread)
        # The child queries its own thread. A desktop handle returned for a
        # foreign thread is not sufficient evidence in the parent's context.
        if attestation_file is not None:
            end = time.monotonic() + 5
            while time.monotonic() < end:
                path = Path(attestation_file)
                if path.is_file():
                    # Creation can be observed before the child's write ends.
                    actual = path.read_text(encoding="utf-8").strip()
                    self.actual_desktop = actual
                    if actual == desktop:
                        self.attest(actual)
                        break
                if self.poll() is not None:
                    break
                time.sleep(0.01)
            if not self.desktop_verified:
                if self.poll() is None:
                    self.terminate()
                    self.wait(timeout=5)
                raise RuntimeError("No valid child desktop attestation: "
                                   + str(self.actual_desktop) + "; exit="
                                   + str(self.returncode))

    def attest(self, actual):
        if actual != self.expected_desktop:
            raise RuntimeError("Child desktop mismatch: " + actual)
        self.actual_desktop = actual
        self.desktop_verified = True

    def poll(self):
        if self.returncode is not None:
            return self.returncode
        state = kernel.WaitForSingleObject(self.process, 0)
        if state == 0:
            code = wintypes.DWORD()
            if not kernel.GetExitCodeProcess(self.process, ctypes.byref(code)):
                raise ctypes.WinError(ctypes.get_last_error())
            self.returncode = code.value
            kernel.CloseHandle(self.process)
            self.process = None
        elif state == 0xFFFFFFFF:
            raise ctypes.WinError(ctypes.get_last_error())
        return self.returncode

    def wait(self, timeout=None):
        if self.returncode is not None:
            return self.returncode
        milliseconds = 0xFFFFFFFF if timeout is None else max(0, int(timeout * 1000))
        result = kernel.WaitForSingleObject(self.process, milliseconds)
        if result == 258:
            raise subprocess.TimeoutExpired(self.args, timeout)
        if result != 0:
            raise ctypes.WinError(ctypes.get_last_error())
        return self.poll()

    def terminate(self):
        if self.poll() is None and not kernel.TerminateProcess(self.process, 1):
            raise ctypes.WinError(ctypes.get_last_error())

    def __del__(self):
        if getattr(self, "process", None):
            kernel.CloseHandle(self.process)
