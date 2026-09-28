# UDM 0.51.0 / browser 0.37.0 verification

This update rebuilds the main download dialogs and adds missing working controls. Full IDM parity is not established.

## Changes

The File Info, File Properties, Progress, Completion and Customize Progress dialogs now opt into Tahoma 8-point, font-based Windows dialog units. Light-theme buttons use the Windows native renderer. Existing dialogs outside this scope retain their earlier layout. The selected resource inventory was checked against installed IDMan.exe SHA-256 03CC62E9ADB77A380F9DC12F67CCAAEE5106F12844AA73CE32C914DDD16D607C; no IDM binary, artwork, driver or licensing files were changed.

File Info adds the category + button, an editable recent-destination list, and explicit category folder memory. More retains queue, authentication, stream links and metadata preview. Properties places its type/status/size, destination, address, description, source page, Referer and credentials in the reference layout; More retains advanced options, last-attempt details and scanner access. Completion uses the reference button order and includes Address and Saved as fields, with an extra region only when scanner results exist.

Progress includes status, size, bytes, rate, time, resume capability, segment map and connection rows. Speed edits apply immediately. Temporary caps remain in memory and reset on resume; remembered caps persist. Hide tab removes the selected optional tab and saves its preference. Right-click opens retained actions, information and customization.

The completion tab now has separate disconnect, Exit UDM, power-mode and force checkboxes. Plans can combine actions, with a single power state and deterministic order. They persist, wait for successful download/scanner completion, and are consumed once before the existing cancellable countdown. Invalid settings and failed saves preserve the prior plan. Force termination is off by default, restricted to shutdown/restart, and requires explicit confirmation because it can discard other applications' unsaved work. Additional completion options (countdown, wait, open file/folder, close progress) are available from the progress context menu. No power/disconnect action was executed during testing.

## Fresh verification

- 967 native checks passed. The 47 focused dialog/completion checks are included in that total.
- 8 native-host protocol checks passed against version 0.51.0.
- 90 selected control rectangles match the reference resource measurements. Combo dropdown heights and runtime-dependent placement are excluded. This is a static geometry audit, not visual or behavioral equivalence.
- Actual Windows Tahoma font metrics, caption fit and basic containment passed at 96, 120, 144 and 192 DPI.
- The browser and signed network runtime are unchanged in this release. Prior browser fixture results are not presented as new test runs.

## Remaining gaps

Desktop control failed before initialization: failed to write kernel assets (path not found). These dialogs compile and their backend checks pass, but fresh rendered-window, button-click, keyboard, dark-theme and screen-reader acceptance is pending. The app remains system-DPI-aware; font-metric tests do not prove mixed-monitor behavior.

The latest public YouTube test in the 0.43 evidence required browser attestation for both audio and video. This release does not resolve or retest that compatibility gap. Existing browser-profile activation, Firefox acceptance, clean-machine install, publisher signing, equal IDM throughput and driver equivalence remain unestablished. The finite workflow inventory still has 88 implemented, 12 partial, 2 unverified and 1 user-deferred English-only item; this is not a percentage of IDM parity.

## Evidence and package

See [test summary](evidence-0.51.0/summary.json), [control comparison](evidence-0.51.0/layout-audit.json), [workflow inventory](idm-parity-0.51.0.md) and [deployment receipt](evidence-0.51.0/deployment.json). Previous installers and source/binary backups are retained.

Windows implementation references: [MFC MapDialogRect](https://learn.microsoft.com/en-us/cpp/mfc/reference/cdialog-class?view=msvc-170#mapdialogrect), [WM_DPICHANGED](https://learn.microsoft.com/en-us/windows/win32/hidpi/wm-dpichanged), and [ExitWindowsEx flags](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-exitwindowsex).

## Current-PC deployment

[UDM 0.51.0 installer](../installer-out/UDM-0.51.0-Browser-0.37.0-Setup-x64.exe) was built. 40 files were deployed with SHA-256 verification and backups. Installer SHA-256: `F5397CFD8DD7018338DAC04D5678DDEB0A82D530AF923960F1519A334800D23B`.

The running native bridge reports 0.51.0 and the correct D:/UDM/user-data catalog. The complete state file, including all 23 downloads, remains byte-identical. Browser files and the signed network runtime remain unchanged. No extension source update or reload is required specifically for 0.51.0; prior current-profile activation is still unverified.
