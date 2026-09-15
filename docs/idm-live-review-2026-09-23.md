# IDM live workflow review — 23 September 2026

**Access is restored.** IDM and UDM are now accessible in the same Windows session. This follow-up verifies actual UI behavior and settings after the earlier resource/documentation audit. UDM remains at 0.12.2; this pass changes the evidence and implementation backlog, not its binaries.

## Findings that change the implementation plan

- Completed-file double-click is configurable between Open and Properties; Properties is selected on this PC. UDM should preserve the requested Properties default while eventually supporting an explicit preference.
- The completed-file menu includes Open with, Move/Rename (Ctrl-M), Redownload and queue actions. IDM Properties also exposes file type, source page and Referer.
- Prefetch during File Info is enabled. This can affect perceived startup latency; no prefetch timing or speed comparison was measured here. Future comparisons must use the same measurement boundary.
- Current IDM connection count is eight, although its list offers up to 32. A higher maximum is not evidence of a speed advantage.
- The pending main queue is empty while 21 completed history entries remain. UDM must distinguish queue membership from its persistent queue label.
- Manual Add URL with an existing localhost URL produced File Info with a numbered filename, not a duplicate-choice dialog at that stage. The four policy choices do not establish the trigger rules for every entry point.

## Live observations

| ID | Surface | Observed result | Route |
|---|---|---|---|
| L01 Access restored | Live UI | A second IDM instance is now running in Windows session 1, alongside UDM. The computer-use tool returned its main window and successfully navigated its dialogs. | window inventory; process/session inspection |
| L02 Completed-file properties | Live UI | Double-clicking an existing completed local benchmark record opened File Properties. The live dialog shows file type, status, exact bytes, Save To with a separate Move action, address, description, parent page, Referer, login/password fields, and Open. No fields were edited. | main list > completed benchmark double-click |
| L03 Double-click preference | Live UI | The completed-download context menu has On Double click > Open / Properties. Properties is currently selected. This is a configurable behavior, not an unconditional IDM rule. | completed record context menu > On Double click |
| L04 Completed-file commands | Live UI | The menu exposes Open, Open with, Open folder, Move/Rename (Ctrl-M), Redownload, Remove, Add to queue, On Double click and Properties. Resume, Stop, Refresh address and Delete from queue were disabled for the inspected completed item. | completed record context menu |
| L05 Actual File menu | Live UI | This installation exposes Stop Download, Remove, Download Now and Redownload in File. It does not place Properties in that menu; UDM currently provides that additional route. | File menu |
| L06 View customization | Live UI | View exposes Hide categories, Arrange files, Toolbar, IDM tray icon, Customize URL List, Dark Mode support, Font and Language. Dark Mode support is checked. Font and tray-icon submenu contents were not enumerated. | View menu |
| L07 Column controls | Live UI | Columns dialog provides visible-column checkboxes, Move Up/Down, Show/Hide, Reset and explicit pixel width. The main accessibility tree exposes 12 columns, including Date Added, Save To, Referer and Parent web page. No column preferences were changed. | View > Customize URL List |
| L08 Find and Find Next | Live UI | Downloads contains Find (Ctrl-F) and Find Next (F3). The Find dialog offers filename, description and URL/site/parent-page/Referer criteria, plus match case and whole-string matching. | Downloads menu; Ctrl-F dialog |
| L09 Browser integration surface | Live UI | General settings list browsers individually, Add browser, integration Restart, configurable keys, context-menu editing and browser-panel editing. No browser setting was changed. | Configuration > General |
| L10 Prefetch preference | Live UI | Start downloading immediately while displaying Download File Info is checked on this installation. This establishes the configured behavior; network prefetch timing was not measured in this pass. | Configuration > Downloads |
| L11 Download dialog preferences | Live UI | The installation enables start and completion dialogs and queue-selection panels after Download Later and batch additions. The option to ignore modification-time changes on resume is unchecked. | Configuration > Downloads |
| L12 Duplicate policy choices | Live UI | The duplicate-policy list contains ask, numbered duplicate, overwrite, and show completed/resume incomplete. The current selection is ask. No different policy was selected. | Configuration > Downloads > duplicate-policy list |
| L13 Progress customization | Live UI | Controls expose Speed Limiter and completion-tab visibility and Hide tab buttons. Start view is normal size. The dialog states queue-originated progress starts minimized in the system tray. Queue execution was not tested. | Downloads > customize progress dialog |
| L14 Connection counts | Live UI | Offered counts are 1, 2, 4, 8, 16, 24 and 32. The configured value is 8. More available connections do not prove greater throughput. | Configuration > Connection > count list |
| L15 Per-server overrides and quota | Live UI | Connection settings include an empty server-exceptions table with New/Edit/Delete and a configurable MB-per-N-hours quota with an optional warning. Quota is disabled on this installation. | Configuration > Connection |
| L16 Save destinations and metadata | Live UI | Save to provides category destinations, remember-last-folder behavior, a separate temporary-directory setting and an option to use the server-provided file creation date. The creation-date option is unchecked. | Configuration > Save to |
| L17 Capture filters | Live UI | File types exposes automatic extension capture, excluded sites and a separate address-exception list editor. | Configuration > File types |
| L18 Web-player panels | Live UI | The media-panel dialog has full/mini modes, media-type selection, per-type minimum size, exceptions, automatic player-capture suppression and an optional panel for protected content that may be undownloadable. Full mode is selected. Presence of this control does not prove DRM download support. | General > customize download panels > For web-players |
| L19 Selected-text link panel | Live UI | A separate For selected files tab controls a panel shown when selected page text contains links. It has full/mini display and all/off/site-specific modes. All selected links is selected. | General > customize download panels > For selected files |
| L20 Force and bypass keys | Live UI | On this installation, bypass is enabled using Del. Force capture is disabled; its controls include modifier keys and Ins. There are additional left-click and dependent-page-resource rules. This corrects any assumption that a documented Alt/Ctrl example is the current local setting. | General > Keys |
| L21 Main queue schedule | Live UI | Scheduler shows main download and synchronization queues plus Download limits. Main schedule includes startup, dated/daily start, stop time, bounded retries, open-file, disconnect, exit and power-action controls. These actions were only inspected. | Scheduler > Main download queue > Schedule |
| L22 Pending queue versus history | Live UI | Files in the queue was empty while the main history had 21 completed items. Its concurrency control is 4 and reorder/remove controls are present. This directly distinguishes pending membership from download history. | Scheduler > Main download queue > Files in the queue |
| L23 Synchronization schedule | Live UI | Selecting Synchronization queue exposes Periodic synchronization and repeat-every-hours/minutes scheduling. No synchronization run or replacement was executed. | Scheduler > Synchronization queue > Schedule |
| L24 Manual duplicate-entry probe | Live UI | Manually entered the exact URL of the existing completed localhost benchmark. It opened Download File Info and proposed idm-steady-q1_2.bin despite the ask policy. It did not display Duplicate download link at this stage. The test was canceled before Start Download or Download Later. Further work must distinguish browser/manual entry points and later confirmation stages. | Add URL > existing localhost benchmark > File Info > Cancel |
| L25 Exit state | Live UI | IDM returned to its main window. The visible list still contained 21 completed records. No Start Download or Download Later action was issued and no new history record appeared. No existing file was opened, moved or overwritten, and no different settings values were applied through this review. Network activity while File Info was open was not measured. | final main-window accessibility count |

## Eight additional inventory entries

| ID | Feature | UDM status |
|---|---|---|
| F096 | Configurable completed-file double-click | Partial |
| F097 | Redownload command | Missing |
| F098 | Dark-mode preference | Missing |
| F099 | Font customization | Missing |
| F100 | Tray-icon appearance choices | Missing |
| F101 | Find Next navigation | Missing |
| F102 | Use server file creation date | Missing |
| F103 | Selected-text download panel | Missing |

The [master audit](idm-feature-audit-2026-09-23.md) now contains **103 checks**: **32 missing**, **34 partial**, and other implemented/verified/different/unverified categories. These are unequal-sized inventory items, not a percentage of parity.

## Boundaries and next acceptance checks

The 25 observations are UI evidence. They do not prove successful network operations, completed-file relocation, duplicate overwrite/resume, media detection, scheduler power actions, or driver behavior. The local-link probe was canceled; the final IDM list still had 21 completed records. No different configuration values were selected. The Add URL history may retain the manually entered localhost address.

Next implementation work should focus on transactional completed-file Move/Rename, Open with, Redownload, explicit duplicate policies, persisted list preferences and pending-queue membership. Before implementing exact duplicate parity, test manual and browser entry points separately, including same URL, changed filename, existing disk file and incomplete history record.

[Machine-readable live evidence](reference/idm-live-review-2026-09-23.json) links back to the installed reference hash and records these limitations.
