# Columns frame investigation — 18 October 2026

The installed Columns update remains four pixels taller in the live captures (332 × 314 versus 332 × 310). This investigation does not establish the cause or fix that difference.

## Static evidence

The retained Ghidra export for IDMan.exe SHA-256 `03cc62e9adb77a380f9dc12f67ccaaee5106f12844aa73ce32c914ddd16d607c` identifies the Columns constructor at `004a9bf0`: it creates resource 209. The initialization routine at `004aa840` calls `0045cd80`, which forwards to the shared routine at `0045c360`.

That shared routine reads the dialog font, optionally reads `selectedFont`, creates a font, and applies it to the dialog and its children. Conditional branches also resize the dialog and its controls. These are observations from inferred pseudocode, not a trace of branches taken by the running application. No reference implementation was copied into UDM. The current user's `Software\DownloadManager\selectedFont` value was absent during this check; a custom saved font is therefore not established as the cause.

## Owned-window probe

`tools/idm-research/dialog-frame-probe.cpp` creates its own hidden Win32 dialogs. It compares native creation against `AdjustWindowRectExForDpi` under unaware, system-aware, and per-monitor-v2 thread contexts. An optional local reference executable is mapped as data only to read dialog 209; its code is never executed by the probe. No existing application window is manipulated.

The probe was compiled with MSVC and UDM's Common Controls v6 manifest. At the current desktop scaling, native and adjusted outer sizes agreed: 370 × 327 at the unaware context's reported 96 DPI; 431 × 401 at 120 DPI in both aware contexts. Synthetic and reference-resource dialogs agreed. Adding `WS_EX_TOOLWINDOW` did not change the measured geometry. Raw output is in `columns-frame-probe-20261018.txt`.

These raw Win32 measurements must not be compared numerically with scaled/cropped computer-control screenshots. The probe does not run IDM's initialization routine or instantiate UDM's `Form`, so it rules out only a basic native-create versus adjustment discrepancy in this fixture. It does not prove either application's runtime client dimensions.

## Reproduction and remaining work

From a Visual Studio developer shell, compile the probe with Unicode definitions, C++17, `user32.lib`, `gdi32.lib`, and `comctl32.lib`, embedding `native/App.manifest`. Run it without arguments for the synthetic fixture, or pass the local reference executable path to include resource 209. Keep binaries and object files outside tracked source directories.

The next live comparison must establish actual client and frame bounds in the same coordinate space, alongside the active font. An arbitrary four-pixel subtraction is not justified by this evidence. The installed app is unchanged by this investigation.
