# UDM 0.52.0 / browser 0.37.0 verification

This configuration and download-workflow update brings UDM closer to the measured IDM layouts. It does not establish full parity or a percentage of parity.

## What changed

Configuration now has General, File types, Connection, Save to, Downloads, Proxy / Socks, Sites Logins, Dial Up / VPN and Sounds tabs. The pages use Tahoma 8-point Windows dialog units. General includes executable-level capture switches and Add browser; the browser extension/native host remains the capture mechanism. Adding an executable does not install an extension or add legacy IE interception. Restart stops only UDM native-host processes located alongside the current executable so extension clients can reconnect.

Options edits remain a draft until Apply/OK. A three-way merge preserves unrelated settings changed while the dialog was open, including independent category-path edits. Nested editors collect the draft first and refresh the visible controls afterward. Cancel discards the current draft; changes already accepted with Apply remain saved.

Save to has editable category file types, destination folders and per-category remember-last-folder. Explicit extension assignments override built-ins; a wildcard assignment is a fallback. Removing a built-in extension changes routing for future downloads. Folder-memory settings carry through rename and are removed on category deletion. Download File Info updates the remembered folder when enabled. Existing extension-and-host rules still apply.

Download Later and batch imports can open a compact queue picker. Its + stages a new queue, Start queue processing starts the selected queue only after the download action succeeds, and Do not ask again persists independently for single and batch downloads. Cancelling the picker leaves the download dialog open. Queue-only browser capture produces paused members for ordinary files and HLS/DASH media, even with start-dialog suppression enabled.

Add URL uses the compact Address, authorization, Login, Password, OK and Cancel arrangement. Duplicate handling uses radio choices. Replacement retains UDM's verified previous-version behavior; remembering Replace remains disabled. Choosing an already complete record still opens Properties, so this part does not yet match IDM's completed-dialog behavior.

Proxy controls expose the default route and per-protocol overrides, including SOCKS, through Advanced. Get System copies a simple configured Windows proxy and bypass list; automatic/PAC or protocol-mapped Windows configurations use the Windows resolver. It clears old custom overrides in the draft so they cannot silently supersede the imported route. Custom PAC URLs are not implemented. Saved site logins remain HTTPS-only. VPN credentials remain in Windows' connection dialog and phonebook.

## Fresh evidence

- 1,008 native regression checks passed, including 41 new Options/workflow checks. Download integrity, segmented resume, media assembly, proxy behavior and persistence checks remained green.
- All 8 native-host framing/protocol checks passed against 0.52.0.
- 6 real isolated app/host checks passed. The running app reported its isolated catalog/version; capture/panel enablement followed the actual Node parent executable even when a message claimed Edge. This does not replace real browser-profile acceptance.
- 107 selected control rectangles match corresponding reference-resource positions. Changed semantics and unsupported rows were excluded. Combo dropdown height, outer property-sheet runtime placement and appearance were not compared.
- The installed reference was hash checked against the resource inventory. No IDM code, binaries, icons or driver files were modified or bundled.

## Remaining differences and limits

Desktop automation fails before initialization with “failed to write kernel assets: path not found.” Native compilation, static layout and backend tests cannot confirm actual rendering, clicking, tab order, keyboard behavior, accessibility or dark-theme appearance. The existing font-metric tests cover 96/120/144/192 DPI; mixed-monitor behavior remains unverified.

The Options window is not identical: legacy IE monitoring, custom PAC URL entry, the TLS 1.3 option, quota-warning checkbox, ignore-modification-time option and direct dial-up credential fields remain different or absent. Keys/menu/panel buttons currently open the shared browser-integration editor. The duplicate completed-dialog and replacement semantics differ as described above. Some subordinate editors retain earlier UDM layouts.

Browser JavaScript stays at 0.37.0 and the signed network runtime is unchanged. Their prior test evidence is not a new test result. Public YouTube attestation compatibility, arbitrary video platforms, browser-specific activation, equal IDM throughput, driver equivalence, clean-machine installation and publisher signing remain unestablished. No new speed-comparison claim is made.

See [test evidence](evidence-0.52.0/summary.json), [layout comparison](evidence-0.52.0/layout-audit.json), [workflow inventory](idm-parity-0.52.0.md) and [deployment receipt](evidence-0.52.0/deployment.json). The finite 103-workflow inventory retains its prior status totals; these are not a percentage of the complete IDM product.

## Current-PC deployment

[UDM 0.52.0 installer](../installer-out/UDM-0.52.0-Browser-0.37.0-Setup-x64.exe) was built. 46 files were deployed with SHA-256 verification and backups. Installer SHA-256: `33CBF071C466B3EB6BBC27A95DF06FD177319C65713ECC06C96A3730044475CB`.

The running native bridge reports 0.52.0 and the correct D:/UDM/user-data catalog. All 23 download records, all queues and every existing preference remain unchanged. First startup saved exactly four new defaults: BrowserDownloadLater=false, CategoryRememberLast={}, QueuePromptBatch=true and QueuePromptLater=true. The state file therefore has a new hash; the original state is retained in the deployment backup. Browser files and the signed network runtime remain unchanged. No extension source update or reload is required specifically for 0.52.0; prior current-profile activation is still unverified.
