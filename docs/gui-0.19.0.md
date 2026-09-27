# UDM 0.19.0 - desktop workflow completion pass

Installed 25 September 2026. This release extends the native C++17/MFC desktop and keeps UDM's own branding and icons. It closes most concrete desktop workflow gaps recorded in the [0.18 GUI review](gui-0.18.0.md). It does not establish complete one-for-one IDM parity. Browser integration remains 0.17.0 with the same native protocol; no extension reload is needed.

## Implemented workflows

| Surface | Behavior | Verification |
|---|---|---|
| Scheduler Automation | Whole-file retry count/delay, repeat interval, completed HTTP/HTTPS synchronization and queue finish action/countdown. Queue drafts Apply or roll back together. One-time schedules cannot repeat; repeating schedules cannot arm finish actions. | Native bounds, scheduling, retry, synchronization, failure and persistence checks; live Schedule/Automation layout review. |
| Synchronization | Uses a bounded range probe, size and strong ETag or Last-Modified checks. Unchanged files do not redownload. Changed files use verified replacement, retaining the old version until the new file is verified. | Native unchanged/changed/error/cancellation and replacement checks. No original user file was synchronized. |
| Queue completion actions | None, Open file, Exit UDM, Disconnect dial-up/VPN, Sleep, Hibernate, Shut down and Restart. Explicit actions receive a cancelable countdown; new pending/active work cancels it. Defaults remain None. | Native eligibility/one-shot/failure checks. Exit countdown opened live and was canceled. No power, disconnect or Open-file action was executed. |
| Queue drag ordering | Drag pending rows within a queue or onto another queue in the tree; Scheduler list ordering is supported. Failed persistence restores membership/order. | Native ordering/rollback tests; live paused-row drag from Main to Second queue persisted. Scheduler in-list dragging was not separately tested live. |
| Completion windows | Modeless dialogs retain Open, Open with, Open folder, Close, suppression and file dragging; add Minimize. They no longer block the next queued download. Repeating/action queues suppress per-file popups. | Two serial local downloads completed while both dialogs remained open; both files matched the expected SHA-256. Both dialogs closed normally. |
| Floating drop basket | Toggle from View or tray; accepts URL drops; menu offers Open UDM, Add URL, Paste link and Hide. Double-click opens UDM; position persists. Visible menu button and right-click menu. | Installed basket/menu button tested; Add URL opened and canceled. Browser URL drop and reposition not exercised live. |
| Recycle downloaded file | Separate from Remove from list. Previews paths, with an unchecked option to remove history. Uses Windows Recycle Bin and undo; refuses directories, reparse points and files needed by an unfinished replacement. No permanent-delete fallback. | Native disposable-file test verified recycling and retained history. No user file was recycled or final GUI recycle confirmation submitted. |
| ZIP contents | Bounded central-directory preview: names, original/compressed sizes and encryption flags, with ZIP64 handling. No extraction/execution. Rejects malformed, split and truncated archives. | Native valid/malformed/truncated tests; live two-entry archive displayed correct names/sizes. ZIP64 not separately fixture-tested. |
| Rich catalog import/export | Checked TXT or .udmcatalog export. Metadata includes URL/file/category/queue/description/hashes. Credentials omitted by default; optional same-Windows-account encryption. Import previews folder override and optional hash-verified existing files. | Native round-trip, protected credentials, duplicates, collisions, transaction rollback and existing-file tests. Export inspected live and canceled; GUI Save and import submission not exercised. |
| Imported media/queues | Imported media requires fresh browser capture and cannot queue a web page as a video. New imported queues are disabled, with no armed finish action or repetition. | Native media and unarmed-queue tests. Catalogs are not a full settings/scheduler backup. |
| Site Grabber wizard | Three pages: project/URL/template; filters/depth/page limit/folder; exploration/log/results/selection/queue. Saved projects, file-type templates, cancel, Check All/Uncheck All, Download later and Add selected. | Native error reporting; live local exploration found two files and added both paused to the chosen folder. |
| Startup option | Opt-in current-user Startup shortcut for background UDM. Checks shortcut ownership and restores settings if creation fails. | Built/source-reviewed; live option inspected unchecked. Startup was not enabled or modified. |
| Proxy controls | Windows proxy/PAC, direct connection or explicit proxy; bypass list and encrypted credentials. Invalid mode/server input rejected. | Native validation and live layout review. Existing proxy settings unchanged; live proxy/PAC routing not tested. |

Existing 0.18 Properties, Advanced Properties, Find, toolbar customization, category dialogs, batch selection, login/server editors, progress preferences and tray controls remain available. Completed-file double-click still opens Properties.

## Verification and deployment

- **331 native checks passed, zero failed**, 45 more than 0.18. HTTP/resume/recovery, media, scheduling, persistence and new file/catalog workflows ran on isolated fixtures.
- Subsequent desktop-only fixes addressed live drag delivery and basket menu access. No download-engine change followed the passing suite.
- Live tests used separate history and a local HTTP fixture. Two 128 KiB serial downloads each produced SHA-256 ab3d7a0bc4f921296719fcc2d8fd2b9a702779218944905f0f554eaea123fb4b.
- Installed desktop, host and monitor binaries match build outputs by SHA-256. Host diagnostics report 0.19.0, the shared data directory and 15 records.
- Original **15 records** are unchanged: 10 complete, three failed, two paused. Queue data is unchanged. Paused ISO byte counts remain **546,308,096** and **571,867,136**.
- App: D:\UDM\release\UDM.exe. Shared state: D:\UDM\user-data. The installed app is left running without a test dialog; the basket is visible.
- Rollback sources/state/binaries and logs: D:\UDM\backups\gui-0.19-20260925. Exit UDM before restoring a backup.

[Deployment evidence](../benchmarks/gui-0.19/deployment.json) and [native test evidence](../benchmarks/gui-0.19/native-test-evidence.json) record the checks. Test files/history under benchmarks/gui-0.19 are separate from user downloads.

## Remaining differences

These still prevent a claim of complete IDM GUI parity:

1. Full desktop browser-panel configuration, force/bypass keys, per-address exceptions and every platform-specific capture option. The extension was unchanged.
2. Offline website mirroring, local-link rewriting, browser-rendered crawling and broader cross-origin Grabber workflows. This wizard uses the existing bounded same-origin static HTML crawler.
3. SOCKS and dial-up connection establishment. The finish action can disconnect existing RAS connections; it does not set them up.
4. Language catalogs, external toolbar skins and full accessibility, keyboard-only, mixed-DPI and theme-combination audits.
5. Portable credential/settings/schedule backup. Catalog credentials are Windows-account-bound; imported queues deliberately remain disabled.
6. Broad live video compatibility and a controlled full-file IDM speed comparison. This GUI release provides no new evidence of a speed advantage.

Power/network actions, actual startup activation, browser-to-basket drops and every dialog permutation still need separate end-to-end validation. Existing media, distribution and driver limits in the main README continue to apply. No IDM proprietary binary, artwork or driver is included.

## Implementation

New native modules: Queue.cpp, FileWorkflows.cpp, SchedulerUi.hpp, SystemActions.hpp, QueueDragUi.hpp, BasketUi.hpp, ZipPreview.hpp, Catalog.hpp, Startup.hpp and GrabberUi.hpp, plus changes to the existing application/core/transfer files.

Recycling checks Windows' operation flags through [IFileOperationProgressSink::PreDeleteItem](https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nf-shobjidl_core-ifileoperationprogresssink-predeleteitem) and [TRANSFER_SOURCE_FLAGS](https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/ne-shobjidl_core-_transfer_source_flags).
