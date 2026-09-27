# UDM 0.18.0 — desktop GUI expansion

Implemented 24 September 2026 against the installed IDM 6.43 GUI, the earlier live observations, and UDM's current C++/MFC source. This is a substantial desktop GUI update, **not complete one-for-one IDM parity**. Original UDM branding, icons and implementation are retained. Browser 0.17.0 is unchanged; no extension reload is required for this desktop update.

## New and expanded workflows

| Surface | Implemented behavior | Validation |
|---|---|---|
| File Properties | Shared layout for complete and unfinished downloads: shell file type/icon, status and percentage, exact byte size, Save To with Move/Browse, address, description, parent page with Open page, Referer, login/password, last try and result. Completed double-click still opens Properties. Active properties are read-only. | Live completed fixture; native metadata, authentication, rejection and rollback checks. |
| Advanced Properties | Category, queue, pending membership, connections, per-file speed, expected and downloaded SHA-256, User-Agent, raw Authorization, per-file completion-dialog suppression. Completed files cannot become pending queue members. | Live completed controls; native completed/unfinished updates and redownload inheritance. |
| Find / Find Next | Ctrl+F dialog with filename, description, address/site/parent/Referer field selection; Unicode case-insensitive or case-sensitive matching; whole-string option. F3 wraps through the current list. Ctrl+Shift+F preserves quick filtering as a separate command. | Live search selected capture-fixture without hiding the other records; nine native search checks. |
| Toolbar | Checked button visibility, Move Up/Down order, small/medium/large size, icons/text styles, hide-toolbar preference, persistent layout, hover tooltips. Start/Stop Queue have functional dropdown arrows. | Editor inspected live; chooser opened and dismissed without starting work; settings restart test. All customization combinations are not visually retested. |
| Categories | Tree context menu with Add, Properties and Delete for custom categories. Name, extensions, host restrictions and save folder editor. Rename updates history; delete reassigns to Other; existing files stay in place. Built-in names remain stable. Images and Other are now visible. | Native rule matching, rename, invalid inputs, deletion and write-failure rollback checks. |
| Scheduler | Queue sidebar and separate Schedule / Files-in-the-queue tabs. Drafts survive queue navigation; OK/Apply save, Cancel discards field edits. Manual, weekday/daily and one-time settings; optional dated stop; queue concurrency and connection retry settings. Move retains selected-file focus. New/Delete/Start/Stop and removal from pending membership remain explicit actions. | Live pages showed five pending records, excluding ten complete records. Existing schedule/one-time/queue tests passed. Runtime OnceStarted marker is preserved when applying dialog edits. |
| Batch/import | URL range entry followed by a checked filename/address grid; Check All, Uncheck All, queue, save folder, Download Later, batch credentials and selected-count feedback. Already-added rows are tracked if a later addition fails. | Live [001-003] preview created three rows; Uncheck All changed count to zero; cancellation created no records. Network batch submission was not performed in user history. |
| Export | Checked download selection, filename/status/address columns, Check All/Uncheck All and Save dialog for text URLs. | Built; selection logic reviewed. Rich metadata exchange is still absent. |
| Connection rules | Dedicated server list with New/Edit/Delete dialogs, host/count validation and duplicate-server protection. | List editor inspected live; underlying per-server limits passed native tests. |
| Saved logins | Separate New/Edit/Remove; Edit preloads the selected origin, username and encrypted password. Properties adds per-file credentials. | Built and source-reviewed; no user credentials were edited or sent. |
| Options | Apply button; custom WAV files and preview for complete/failed/paused events; progress start normal/small/minimized, optional tabs and queue minimization. | Progress-preference dialog inspected live; settings persistence checked. Custom audio playback was not exercised. |
| Tray | Pause All, queue choosers, Scheduler, Options and aggregate Speed Limiter in addition to Add URL and Exit. | Built; underlying limiter tests passed. |
| Completed-file drag | CF_HDROP copy/link drag from completed list rows and completion-dialog icon. | Built and source-reviewed; drag into an external application was not performed. |
| Add URL | Editable recent-URL dropdown derived from existing history; existing authorization workflow retained. | Built. No additional URL history database is created. |

New native code also validates queue/category/source-page edits, treats request-header names case-insensitively, keeps Basic passwords containing colons intact, strips account headers on cross-origin URL changes, and restores queue/category state if persistence fails. No new automatic program execution, power action, browser interception setting or Windows security setting was enabled.

## Reference observations in this pass

The installed IDM File Properties window for the paused Ubuntu ISO showed Type, Status/percentage, exact Size, Save To/Browse, Address, Description, parent page, Referer, Login, Password, last try/result, Open and OK. Its Find dialog showed a common search string with field checkboxes and case/whole-string options. Its Scheduler used a queue sidebar and Schedule / Files-in-the-queue tabs, with additional synchronization and completion actions that UDM still lacks. These reference dialogs were closed without applying changes or starting a download.

## Verification and installation

- **286 native checks passed, zero failed**, including **35 new GUI-workflow regression checks**. Full HTTP, resume, duplicate, scheduler, authentication, file operation and media suites ran against isolated fixtures. Test data is under the native-test evidence root.
- GUI-only additions and visual review fixes were compiled afterward. The desktop-only build path avoids relinking unchanged host/test executables. No engine changes were made after the passing suite.
- Live UDM checks covered completed Properties and Advanced Properties, cancellation, Find selection, both Scheduler pages, toolbar editor, actual queue dropdown, batch expansion/selection/cancellation and progress preferences.
- Final shared user history contains **15 records: 10 complete, three failed and two paused**. Downloads and queues match the pre-upgrade snapshot. Both paused ISO byte counts remain **546,308,096** and **571,867,136**. No test downloads were added to user history.
- Deployed application: `D:\UDM\release\UDM.exe`. Shared data remains `D:\UDM\user-data`. Native desktop/host version is **0.18.0**; browser bundle remains **0.17.0**, with the same native protocol.
- Rollback: `D:\UDM\backups\gui-0.18-20260925` contains pre-change native sources, release binaries and history. Build logs are retained there. Installation/test evidence is under `benchmarks/gui-0.18`.

## Remaining GUI gaps

These still prevent a claim of a complete IDM GUI replica:

1. Floating drop basket and an explicit saved-file deletion/recycle-bin workflow. Existing Remove keeps saved files.
2. Queue synchronization/repeat intervals, whole-file retry rotation, drag ordering between queues, and completion actions with a countdown for application exit, disconnect or power operations.
3. Full browser-panel configuration from the desktop, force/bypass key rules, per-address exceptions and every platform-specific capture option.
4. ZIP-content preview and rich history/credential/queue import/export rather than text URLs.
5. Full Site Grabber wizard, offline browsing/mirroring and all template/error/result dialogs.
6. Complete proxy/SOCKS/PAC/dial-up configuration, startup integration, language catalogs and external toolbar skins.
7. Full accessibility, keyboard-only traversal, mixed-DPI and all theme/layout combinations. Keyboard focus and visible selection were added to dialog lists; this is not a complete accessibility audit.

The older 103-item audit is historical and includes features implemented in 0.13–0.18. Use this report with `features-0.16.1.md` and `speed-menu-0.17.0.md`; do not interpret old missing counts as the current status, or count the above unequal workflows as a parity percentage.

[Machine-readable final verification](../benchmarks/gui-0.18/deployment.json) records final executable hashes, native-host diagnostics and history preservation. The full 286-check suite also passed after restoring the original UTF-8 test fixtures following a Windows text-encoding conversion.
