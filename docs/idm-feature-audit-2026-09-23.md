# IDM workflow audit against UDM 0.12.2

Audit date: 23 September 2026 UTC (22 September local). **95 concrete feature/workflow checks** and **105 installed IDM dialog templates** inventoried. This is a gap assessment, not a claim of exhaustive behavior or source-code equivalence.

## Evidence and limits

The installed IDMan.exe SHA-256 still matches the earlier read-only resource extraction: `03cc62e9adb77a380f9dc12f67ccaaee5106f12844aa73ce32c914ddd16d607c`. [The resource inventory](reference/pe-analysis.json) provides control names and layout metadata. A dialog's presence proves a UI surface exists, not that its runtime behavior has been exercised. No reference executable code or artwork was copied.

Current UDM native source and browser bundles were inspected. Historic C# feature claims in [analysis.md](analysis.md), [idm-workflow.md](idm-workflow.md) and [deep-inspection.md](deep-inspection.md) are not current implementation status; this audit supersedes their gap lists. In particular, the native completion dialog lacks Open with despite an older UI report listing it.

IDM did not expose a targetable window through the current computer-control session, including after an attempt to show its existing instance. Therefore this pass uses installed resource metadata plus official documentation for IDM, with live UDM checks listed separately. It does not pretend to be a fresh end-to-end test of every IDM dialog.

## Status meanings

- **Verified**: concrete prior or current test evidence linked in the row; not universal compatibility.
- **Implemented**: a working code path was found; this workflow was not freshly tested in the audit.
- **Partial**: important portions are missing or validation does not cover the requested parity.
- **Missing**: no native UDM workflow found in inspected code.
- **Different**: UDM deliberately or currently behaves differently; not automatically a defect.
- **Unverified**: needs a reference or live acceptance test.
- **Fixed in 0.12.2**: code changed in this audit; see validation below for actual exercised cases.

| Status | Checks |
|---|---:|
| Verified | 7 |
| Partial | 33 |
| Implemented | 20 |
| Fixed in 0.12.2 | 3 |
| Missing | 25 |
| Different | 3 |
| Unverified | 4 |

These are inventory counts, not a parity percentage: the rows have very different implementation costs.

## List and file handling

Reference: [main dialog](https://www.internetdownloadmanager.com/support/using_idm/using_idm.html) and [file properties](https://www.internetdownloadmanager.com/support/properties.html); resource IDs identify the installed dialogs.

| ID / workflow | Status | Current UDM finding | Evidence / IDM resource |
|---|---|---|---|
| F001 Completed-file double-click | Verified | Opens File Properties; explicit Open remains separate. | native/App.cpp; native/Ui.hpp; docs/reference/properties-0.12.1.json; R135 |
| F002 Properties from File and context menus | Verified | Both routes tested against the completed Microsoft ISO in 0.12.1. | native/App.cpp; docs/reference/properties-0.12.1.json; R135 |
| F003 Completed properties editing | Partial | Details are read-only; no completed-file rename, relocation, URL or description editing. | native/Ui.hpp completedProperties; native/Core.cpp configure; R135 |
| F004 Paused-file properties | Implemented | URL, destination, category, description, queue, connections, limit and expected hash can be edited while stopped. | native/Ui.hpp downloadInfo; native/Core.cpp configure; R135 |
| F005 Properties source page and authentication | Partial | Refresh knows a source page, but properties omit source-page, Referer and credential editors. | native/Ui.hpp; native/Core.cpp recoveryPage; R135 |
| F006 Action enablement | Fixed in 0.12.2 | Toolbar, menus and command dispatch now share selection/state checks; active records cannot be removed through these actions. | native/App.cpp commandEnabled; R102 |
| F007 Sorting every displayed column | Fixed in 0.12.2 | Uses size, progress, ETA, rate, date and description values; formerly most columns sorted filenames. | native/App.cpp refresh; R102 |
| F008 Keyboard context menu | Fixed in 0.12.2 | Menu key and Shift+F10 open the selected download menu; selection and focus survive sorting. | native/App.cpp OnContextMenu; R102 |
| F009 Column resizing and reordering | Partial | Native header drag/reorder works in-session; layout is not saved. | native/App.cpp OnCreate; R209 |
| F010 Column visibility chooser | Missing | No Columns dialog or header customization menu. | native/App.cpp; R209 |
| F011 Toolbar customization | Missing | Fixed eleven buttons; no size/style/order/visibility preferences. | native/App.cpp; R102 |
| F012 Find-file filtering | Partial | Case-insensitive filename and URL search; no description, parent-page, case-sensitive or whole-string criteria. | native/App.cpp refresh; R506 |
| F013 Open downloaded file and folder | Implemented | Explicit shell actions; completed properties disable Open if the target is absent. | native/App.cpp; native/Ui.hpp; R135 |
| F014 Select and remove history records | Implemented | Multi-selection with confirmation; removes records while retaining completed files. | native/App.cpp; native/Core.cpp remove; R338,339 |
| F015 Remove all completed records | Implemented | Confirmation and keep-files semantics; enabled only when completed records exist after 0.12.2. | native/App.cpp; R338,339 |
| F016 Delete files from disk workflow | Missing | No explicit record-plus-file deletion option or recycle-bin workflow. | native/App.cpp; R338,339 |
| F017 Duplicate-link choices | Partial | Destination collisions get numbered names automatically; no same-URL resume, overwrite or remembered-choice dialog. | native/Core.cpp add; R298 |
| F018 Category folders and rules | Implemented | Extension and host rules, remembered destinations, and custom categories exist. | native/Core.cpp add; native/WorkflowsUi.hpp categoryRule; R204 |
| F019 Category edit/delete/tree menus | Partial | Rules can be updated in Options; no category rename/delete or category-tree context menu. | native/App.cpp buildTree; native/WorkflowsUi.hpp; R204 |
| F020 Hide category pane | Missing | The pane is fixed in the layout. | native/App.cpp layout; R102 |

## Adding and starting

Reference: [starting downloads](https://www.internetdownloadmanager.com/support/using_idm/starting.html), plus installed File Info, duplicate and batch dialogs.

| ID / workflow | Status | Current UDM finding | Evidence / IDM resource |
|---|---|---|---|
| F021 Add URL with credentials | Implemented | HTTP/HTTPS/FTP URL entry and optional Basic authorization; no URL-history drop-down. | native/Ui.hpp addAddress; R148 |
| F022 File Info start/later/cancel | Implemented | Destination, category, remembered path, description and queue choices exist. | native/Ui.hpp downloadInfo; R130 |
| F023 Pre-download size/type preview | Partial | Info shows saved size; a newly added URL is not asynchronously probed while the dialog is open. | native/Ui.hpp downloadInfo; R130 |
| F024 ZIP contents preview | Missing | No ZIP central-directory preview workflow. | native/Ui.hpp; R196 |
| F025 Download while File Info stays open | Missing | Transfer starts after the start action; no prefetch preference. | native/Ui.hpp downloadInfo; R291 |
| F026 Clipboard capture | Different | Opt-in nonmodal suggestion for a valid URL; no automatic extension-filtered modal prompt. | native/App.cpp clipboardText and refresh; R193 |
| F027 Drop links onto main window | Implemented | OLE Unicode-text URL drop opens Add URL; browser compatibility not freshly tested. | native/App.cpp DropTarget; R187 |
| F028 Floating drop basket | Missing | No separate floating drop-target window. | native/App.cpp; R187 |
| F029 Batch numeric/letter sequences | Partial | Supports bracket ranges such as [001-100] and [a-z]; lacks the reference preview grid and batch authentication controls. | native/Ui.hpp batchDialog; native/Core.cpp expand; R280 |
| F030 Import/export URL lists | Partial | Text URL list import/export; no rich metadata interchange or import selection grid. | native/App.cpp; R191,228 |
| F031 Command-line integration | Partial | Add/folder/name/background/paused supported; /n currently means paused, unlike IDM silent mode. Existing-instance forwarding drops folder/paused fields. | native/App.cpp Application::InitInstance; R148 |

## Progress and completion

Reference: [completion dialog](https://www.internetdownloadmanager.com/support/using_idm/completeD.html), [limiter semantics](https://www.internetdownloadmanager.com/register/new_faq/functions11.html), and installed progress/completion controls.

| ID / workflow | Status | Current UDM finding | Evidence / IDM resource |
|---|---|---|---|
| F032 Status, bytes, ETA and resume capability | Implemented | Live labels, aggregate progress and connection rows are wired to native job state. | native/Ui.hpp Progress; R363 |
| F033 Segment progress map | Implemented | Draws downloaded intervals from the native range plan. | native/Ui.hpp RangeMap; R363 |
| F034 Pause/resume and reopen progress | Implemented | Modeless progress and main-list actions call Manager; previous stalled-request checks passed. | native/Ui.hpp Progress; docs/stress-testing-0.10.1.md; R103 |
| F035 Close progress stops transfer | Different | Close/Escape and Hide destroy the window while the transfer continues; IDM documentation describes stopping on close. | native/Ui.hpp Form::OnCancel and Progress; R103 |
| F036 Per-file speed limiter | Implemented | Temporary session cap and remembered per-file cap exist. | native/Ui.hpp Progress; native/Core.cpp charge; R365 |
| F037 Aggregate speed limiter | Different | UDM also has an aggregate cap across jobs; IDM documents its global control as a cap for each download. | native/Core.cpp charge; R337 |
| F038 Completion dialog | Partial | Open, Open folder, Close and suppression checkbox exist; Open with is absent in the current native implementation. | native/App.cpp complete; R286 |
| F039 Drag completed file to another app | Missing | No outgoing file drag source on the completion dialog. | native/App.cpp complete; R286 |
| F040 Completion actions | Missing | No per-download exit/shutdown/disconnect actions or countdown/cancel flow. | native/Ui.hpp Progress; R364,150,292 |
| F041 Progress customization | Missing | No start-minimized preference, selectable tab visibility, or progress layout editor. | native/Ui.hpp Progress; native/WorkflowsUi.hpp Options; R367 |
| F042 Per-event sounds | Partial | One Windows completion notification sound; no event-specific sound files. | native/App.cpp complete; native/WorkflowsUi.hpp Options; R195 |

## Recovery and transfer

Reference: [refreshing an expired address](https://www.internetdownloadmanager.com/register/new_faq/sites2_3.html). Transfer implementation and verification are from UDM sources and recorded tests.

| ID / workflow | Status | Current UDM finding | Evidence / IDM resource |
|---|---|---|---|
| F043 Expired-address recovery | Verified | Completed the actual Microsoft ISO after preserving partial bytes; full SHA-256 matched Microsoft's published value. | docs/reference/recovery-0.12.0.json; R135 |
| F044 Refresh from original page | Partial | Can open the source page and wait for a fresh offer; current browser GUI handoff has not been revalidated end-to-end. | native/Ui.hpp RefreshAddress; native/Bridge.cpp; native/Core.cpp; R135 |
| F045 Ambiguous replacement link | Partial | Waiting capture matches the filename for ten minutes; nonmatching captures follow normal add. No explicit ambiguous-match chooser. | native/Core.cpp captureAddressRefresh; R135 |
| F046 Safe partial-file reuse | Verified | Same-length/range/validator checks prevent combining incompatible data; 28 recovery checks passed in 0.12.0. | native/Transfer.cpp; native/RecoveryChecks.hpp; R135 |
| F047 HTTP segmented download and dynamic splitting | Verified | Original native range engine and adaptive help passed prior local stress checks; universal speed parity is unproven. | native/Transfer.cpp; docs/reference/engine-stress-0.11.0.json; R363 |
| F048 Connection count and per-server overrides | Partial | 1-16 connections per job; no per-host connection rules UI. | native/Core.cpp; native/WorkflowsUi.hpp; R226,141 |
| F049 Stalled-request cancellation and retries | Verified | Asynchronous cancellation and bounded retries have prior regression evidence; not rerun for this UI audit. | native/Transfer.cpp; docs/stress-testing-0.10.1.md; R363 |
| F050 Restart persistence | Implemented | Atomic JSON state and saved part plans; startup converts interrupted transient states to Paused. | native/Core.cpp Manager and save; R102 |
| F051 Authenticated downloads | Partial | Browser request headers/cookies and saved origin credentials exist; broad challenge-based authentication is not validated. | native/Transfer.cpp; native/Core.cpp; browser/chromium/background.js; R145,146 |
| F052 FTP transfer and resume | Partial | Native FTP path is sequential and clears partials; no resumable segmented FTP or FTP settings dialog. | native/Transfer.cpp transfer; R140 |
| F053 Expected SHA-256 checking | Verified | Supported by UDM; actual ISO independently matched its expected hash. This is an additional UDM integrity feature. | native/Transfer.cpp; docs/reference/recovery-0.12.0.json; RUDM addition |
| F054 Disk-full and interrupted publication recovery | Unverified | Some failure checks exist; systematic UI recovery for disk-full, moved destinations and removable drives remains required. | native/Transfer.cpp; native/Core.cpp; R102 |

## Queues and scheduling

Reference: [queue model](https://internetdownloadmanager.com/support/idm-scheduler/idm_queues.html) and [scheduler](https://www.internetdownloadmanager.com/support/schedulerD.html), alongside installed Schedule dialog 306.

| ID / workflow | Status | Current UDM finding | Evidence / IDM resource |
|---|---|---|---|
| F055 Named queues and concurrency | Implemented | Named queues, manual start/stop, per-queue parallelism and retries are implemented. | native/Core.cpp tick; native/WorkflowsUi.hpp Scheduler; R308 |
| F056 Daily/weekday/overnight scheduling | Implemented | Native schedule windows support day masks and crossing midnight; current timezone/DST UI coverage is incomplete. | native/Core.cpp inWindow; native/WorkflowsUi.hpp Scheduler; R306 |
| F057 One-time schedule | Implemented | Dated start/stop and OnceStarted persistence exist. | native/Core.cpp inWindow; native/WorkflowsUi.hpp Scheduler; R306 |
| F058 Periodic synchronization queues | Missing | No remote-change check/replacement queue type. | native/Core.cpp; native/WorkflowsUi.hpp Scheduler; R306 |
| F059 Repeat every N hours/minutes | Missing | Daily or one-time windows only. | native/Core.cpp defaultQueue and inWindow; R306 |
| F060 Queue completion actions | Missing | No open-file, application exit, disconnect or power actions. | native/WorkflowsUi.hpp Scheduler; R306 |
| F061 Separate queued membership from queue label | Partial | Completed jobs keep Queue and remain in queue filtering; no explicit Add to queue/Remove from queue state. | native/App.cpp refresh; native/Core.cpp; R307 |
| F062 Queue ordering | Partial | Up/down commands exist; no drag between queue trees or scheduler lists. | native/App.cpp; native/WorkflowsUi.hpp Scheduler; R307 |
| F063 Queue menus and toolbar chooser | Missing | Start/Stop act on selected/default queue; no queue-tree context menus or toolbar drop-downs. | native/App.cpp; R308 |
| F064 Suppress dialogs for scheduled work | Missing | Completion/event suppression is preference based, not automatically tied to scheduler-originated jobs. | native/Core.cpp start; native/App.cpp complete; R291 |
| F065 Wake computer for scheduled downloads | Unverified | No UDM wake-timer integration found; IDM behavior on this PC was not established by this audit. | native/Core.cpp inWindow; RNeeds reference test |

## Browser and media

Reference: installed browser/media settings resources and [prior component inspection](deep-inspection.md). Broad platform compatibility is explicitly unverified.

| ID / workflow | Status | Current UDM finding | Evidence / IDM resource |
|---|---|---|---|
| F066 Chromium/Firefox native messaging | Implemented | Separate native host and browser bundles; prior framing/protocol tests exist. | native/Bridge.cpp; native/HostMain.cpp; browser; R296 |
| F067 Automatic ordinary download interception | Partial | Opt-in extension type/host filtering with pause, durable ACK, cancel-on-success and browser-resume fallback; fresh browser GUI verification still needed. | browser/chromium/background.js; native/Bridge.cpp; R294 |
| F068 Single-link context menu | Implemented | Download with UDM on links/video/audio sends browser context to desktop. | browser/chromium/background.js; R228 |
| F069 Download all/selected page links | Partial | Popup discovers links for individual action; no selected-text or page-wide batch-review handoff. | browser/chromium/popup.js; browser/chromium/background.js; R228 |
| F070 Capture force/bypass keys | Missing | No user-configurable modifier-key capture rules. | browser/chromium/background.js; native/WorkflowsUi.hpp; R229 |
| F071 Capture address exceptions | Partial | Host exclusions exist; no per-URL wildcard exception editor or cancellation-based exclusion prompt. | browser/chromium/background.js; native/WorkflowsUi.hpp; R303,304,302 |
| F072 Cross-site video panels | Partial | Original panel follows player geometry, scrolling, clipping and fullscreen; needs broader real-site testing. | browser/chromium/content.js; docs/improvements-0.11.0.md; R340 |
| F073 Panel preferences | Partial | Movable compact panel and site permissions exist; no full/mini mode, media-type, minimum-size and per-player exception configuration. | browser/chromium/popup.html; browser/chromium/content.js; R340,370 |
| F074 Current-video quality association | Partial | Video identity and captured format filtering are implemented; current live YouTube/SABR handoff is not proven by this audit. | browser/chromium; native/Streaming.cpp; docs/browser-capture.md; R340 |
| F075 Direct media and recorded HLS/static DASH | Partial | Supported clear recorded formats have parser/assembly checks; unsupported/live/DRM formats are rejected. Not all platforms are covered. | native/Adaptive.cpp; native/AdaptiveChecks.hpp; R340 |
| F076 Native video/audio assembly | Implemented | Original engine plus local FFmpeg/FFprobe assembly; fresh browser capture is required, not a yt-dlp URL resolver. | native/Transfer.cpp mediaTransfer; native/Adaptive.cpp; R286 |
| F077 Live streams, subtitles, alternate tracks | Missing | No complete user workflow for these media variants. | native/Adaptive.cpp; browser/chromium/popup.html; RNeeds reference test |
| F078 Network driver | Partial | Original WFP monitoring source/build exists; kernel loading/signing/deployment remains unvalidated. Metadata counters do not reveal encrypted media URLs. | native/Network.cpp; docs/deep-inspection.md; Rreference/pe-analysis.json |

## Settings and integration

Reference: installed Options dialog controls and [official options documentation](https://www.internetdownloadmanager.com/support/options.html).

| ID / workflow | Status | Current UDM finding | Evidence / IDM resource |
|---|---|---|---|
| F079 Proxy support | Partial | Windows/default or named proxy with credentials; no SOCKS4/5 or full per-protocol/PAC/bypass configuration surface. | native/Transfer.cpp; native/WorkflowsUi.hpp; R140,369 |
| F080 Saved site logins | Partial | Encrypted HTTPS-origin credentials; no path-specific rules or authentication editor in properties. | native/Core.cpp add; native/WorkflowsUi.hpp; R145,146 |
| F081 Download quota | Partial | Fixed hourly MB window; no selectable multi-hour window or exceeded-quota dialog. | native/Core.cpp charge; native/WorkflowsUi.hpp; R194,141 |
| F082 Temporary-files location | Missing | Parts use the application data directory; no separate relocation preference. | native/Transfer.cpp; native/Core.cpp; R143 |
| F083 Virus scanner integration | Partial | Launches configured scanner after saving; does not wait for scan verdict or quarantine/hold opening based on result. | native/Core.cpp start; native/WorkflowsUi.hpp; R494 |
| F084 Dial-up/VPN connection automation | Missing | No connection, reconnection or hang-up settings workflow. | native/WorkflowsUi.hpp; R185,186 |
| F085 Manual-download User-Agent setting | Missing | Transport has a fixed UDM identity; captured headers are separate. No settings field for manual UA. | native/Transfer.cpp; native/WorkflowsUi.hpp; R291 |
| F086 Tray controls | Partial | Restore, Add URL and Exit only; no tray queue/limiter/drop-basket menus. | native/App.cpp OnTray; R187,337 |
| F087 Start with Windows preference | Missing | No startup toggle in the native Options dialog. | native/WorkflowsUi.hpp; R139 |
| F088 Language selection | Missing | Native UI strings are English; no localization catalog/chooser. | native/App.cpp; native/Ui.hpp; native/WorkflowsUi.hpp; R208 |
| F089 Signed installer, updater and repair | Partial | Portable packaging and registration scripts exist; no validated signed distribution/update/rollback workflow. | package.ps1; browser/register-host.ps1; R182,190 |

## Grabber

Reference: [official Grabber workflow](https://www.internetdownloadmanager.com/support/idm-grabber/idm_grabber.html) and installed Grabber dialog templates.

| ID / workflow | Status | Current UDM finding | Evidence / IDM resource |
|---|---|---|---|
| F090 Saved projects and extension filters | Implemented | Bounded same-origin HTML exploration, saved projects, link checkboxes and queued adds exist. | native/Transfer.cpp explore; native/WorkflowsUi.hpp GrabberDialog; R239,262 |
| F091 Mirroring and offline browsing | Missing | No offline link rewriting, site hierarchy preservation or complete asset mirroring. | native/Transfer.cpp explore; R239 |
| F092 Templates and advanced exploration filters | Missing | No multistep wizard/template library, page/file filter rules or authenticated rendered-site exploration. | native/WorkflowsUi.hpp GrabberDialog; R240,244,245,268 |
| F093 Project-specific scheduling and tree actions | Missing | Projects can add to a queue; no project schedule, rename/delete or context-menu workflow. | native/App.cpp buildTree; native/WorkflowsUi.hpp GrabberDialog; R284,269,270 |

## Quality and performance

These are acceptance requirements. No fresh speed ranking or complete UI compatibility is inferred from binary imports.

| ID / workflow | Status | Current UDM finding | Evidence / IDM resource |
|---|---|---|---|
| F094 Same-server real-world IDM speed parity | Unverified | Prior results are limited by server, timing, link and cache conditions; no new comparative speed test was run in this audit. | docs/reverse-engineering-0.10.md; RComparison required |
| F095 Mixed-DPI/accessibility behavior | Unverified | Standard MFC controls expose accessibility, but screen-reader, high-contrast and mixed-monitor coverage is incomplete. | native/App.cpp; native/Ui.hpp; R102 |

## Recommended implementation order and acceptance criteria

1. **Duplicate and file-property workflows.** Detect the same canonical download; offer resume/show existing/numbered duplicate, with overwrite only as an explicit action. Completed-property rename/relocation must validate collisions, move successfully before committing history, preserve metadata, and recover from denied moves. Test same and cross-volume destinations and missing files.
2. **Everyday list behavior.** This release fixes sorting/action states/keyboard menus. Next add persisted columns, copy-address, Open with, richer search and category editing. Test each command for no selection, one completed, one paused, one active and mixed selections.
3. **Recovery from a real browser click.** Fresh-link offer must be associated with the intended waiting job; test renamed links, ambiguous matches, expiry, incorrect size/validator, cancellation, browser restart and native-host failure. Keep the already verified byte-preservation safeguards.
4. **Queue semantics.** Separate history's queue label from actual pending membership, add/remove membership, queue menus and drag/drop. Test completion, failure/retry order, manual stop and restart. Add synchronization as a separate explicit mode, with a clear replacement policy.
5. **Completion/close behavior.** Resolve Stop versus Hide semantics with explicit labels; add completion actions with visible cancellable countdowns. Test that cancellation prevents power/exit actions, including when another download remains active.
6. **Browser batch capture and exceptions.** Add page/selection link collection with review, type filters and queue choice; support bypass keys and scoped exclusions. Test cookie/referrer context and browser fallback on native failure.
7. **Options and grabber depth.** Per-host connection limits, temporary directory migration, proxy variants, offline mirroring and templates are separate substantial features. Do not mark these complete merely because a similarly named tab exists.
8. **Media and delivery qualification.** Current YouTube/SABR live capture, multiple real video sites, signed installation/rollback and driver lifecycle need dedicated testing. Driver metadata monitoring alone cannot deliver application/media parity.

## Changes in 0.12.2

- A single command-state policy now controls menus, toolbar and shortcut dispatch. Completed items cannot Resume/Stop; active selections cannot Delete or edit properties; mixed selections cannot silently perform an invalid remove/move.
- Column sorting uses the corresponding values, with stable equal-value ordering and preserved selection/focus. Missing ETA/date use sentinel values; direction reverses when the same header is clicked again.
- Menu key and Shift+F10 open the selected download's context menu at its row. Empty space has no download menu.

## Validation

The native build passed, and UDM 0.12.2 is deployed and running. Native-host ping/framing passed. Twelve recorded checks covered no-selection/completed/paused/active/mixed action states, size/date/description sorting, preserved selection, Shift+F10, the deployed mouse menu, and unchanged history. The live active-state test used only a throttled loopback server in an isolated data directory. All ten existing user download records remained identical. See [machine-readable results](reference/feature-audit-0.12.2.json).

Live ETA/rate ordering with multiple concurrent transfers, the physical Menu key, browser capture and Internet speed comparisons were not tested in this pass. The 125 native engine checks belong to 0.12.0 and were not rerun for these UI-only changes. Only the HTTP user-agent version string changed outside App.cpp/App.rc and release documentation.

## Installed dialog index

All 105 parsed templates are indexed below for follow-up inspection. Blank captions are internal pages. Registration/update templates are inventory evidence, not a requirement to reproduce IDM's licensing system.

| Resource ID | Caption | Controls |
|---|---|---:|
| 100 | About Internet Download Manager | 17 |
| 102 | Internet Download Manager 6.43 | 6 |
| 103 | Download progress | 9 |
| 105 | Tip of the Day | 7 |
| 130 | Download File Info | 19 |
| 135 | File Properties | 31 |
| 138 | Internet Download Manager Registration | 14 |
| 139 | General | 18 |
| 140 | Proxy / Socks | 32 |
| 141 | Connection | 22 |
| 143 | Save to | 19 |
| 144 | New version of Internet Download Manager is available | 3 |
| 145 | Sites Logins | 7 |
| 146 | Site login | 10 |
| 147 | Dialog | 5 |
| 148 | Enter new address to download | 10 |
| 150 | Shut down | 4 |
| 182 | Quick Update | 2 |
| 184 | Dial up | 3 |
| 185 | Dial Up / VPN | 20 |
| 186 | Dial Up Options | 7 |
| 187 | IDM drop target. Drop web-links for downloading here | 1 |
| 190 | Download IDM components | 3 |
| 191 | Export download list | 5 |
| 193 | Automatically start downloading the following extensions: | 3 |
| 194 | Download limits exceeded! | 2 |
| 195 | Sounds | 7 |
| 196 | Zip preview | 3 |
| 204 | IDM categories | 16 |
| 208 | IDM language | 4 |
| 209 | Columns | 13 |
| 226 | Max. connections number for a server | 8 |
| 228 | Download All Links with IDM | 24 |
| 229 | Using special keys | 19 |
| 230 | Checking Internet Connection | 3 |
| 231 | General | 17 |
| 232 | Internet Download Manager Problem Report | 8 |
| 236 | IDM tip: Automatic link capture | 6 |
| 239 | IDM Site Grabber | 4 |
| 240 | (internal page) | 25 |
| 241 | IDM Site Grabber | 11 |
| 242 | (internal page) | 0 |
| 244 | Filters | 5 |
| 245 | Filter | 7 |
| 257 | (internal page) | 22 |
| 259 | (internal page) | 13 |
| 262 | Settings of IDM Grabber | 11 |
| 264 | (internal page) | 21 |
| 268 | Template Name | 4 |
| 269 | Saving the project of IDM Grabber | 5 |
| 270 | Load project | 3 |
| 271 | IDM Grabber Statistics | 18 |
| 279 | Connecting to the site | 10 |
| 280 | Batch download | 32 |
| 281 | IDM Site Grabber | 7 |
| 284 | Schedule the grabber project | 44 |
| 286 | Download complete | 12 |
| 290 | IDM update notice | 5 |
| 291 | Downloads | 23 |
| 292 | Rebooting computer | 4 |
| 294 | File types | 12 |
| 296 | Browsers integration | 4 |
| 297 | Browser details | 3 |
| 298 | Duplicate download link | 8 |
| 300 | Internet Download Manager | 4 |
| 302 | Cancelling an automatically started download | 9 |
| 303 | The list of address exceptions | 7 |
| 304 | Add an address to exceptions list | 4 |
| 305 | IDM could not take over the download | 4 |
| 306 | Schedule | 37 |
| 307 | Files in the queue | 8 |
| 308 | Scheduler | 13 |
| 309 | (internal page) | 8 |
| 310 | Enter queue name | 4 |
| 332 | IDM Queues | 3 |
| 334 |  Chrome, Edge, IE  | 5 |
| 335 | Firefox and other Mozilla based | 5 |
| 336 | Report for IDM developers | 7 |
| 337 | Speed Limiter settings | 7 |
| 338 | Confirm deletion of downloads | 5 |
| 339 | Confirm deletion of downloads | 4 |
| 340 | (internal page) | 19 |
| 341 | Add file type | 3 |
| 342 | IDM | 3 |
| 344 | IDM | 2 |
| 345 | (internal page) | 6 |
| 348 | Dialog | 14 |
| 349 | Enter site name | 3 |
| 352 | Exit IDM | 1 |
| 363 | Download status | 13 |
| 364 | Options on completion | 14 |
| 365 | Speed Limiter | 8 |
| 367 | Customize download progress dialog | 9 |
| 369 | Advanced proxy settings | 48 |
| 370 | Download panel exceptions | 5 |
| 381 | Google Chrome Integration | 4 |
| 487 | Find file | 9 |
| 488 | Minimum size | 7 |
| 489 | Integration into Microsoft Edge browser | 5 |
| 494 | Virus checking | 8 |
| 504 | (internal page) | 0 |
| 505 | (internal page) | 5 |
| 506 | Find file | 10 |
| 30721 | New | 5 |
| 30734 | (internal page) | 0 |
