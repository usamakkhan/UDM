# IDM reverse engineering: remaining UDM features

24 September 2026. Reference: IDM 6.43 build 10. UDM: 0.16.1.

This pass identifies **21 missing and 20 partially implemented feature areas** in the register below. These are a prioritized selection from the historical audit plus newly traced gaps, not a percentage-complete score or an exhaustive count of every IDM behavior. Several entries concern separate aspects of one workflow.

The most important newly reproduced gap is **form-generated downloads that require POST**. The current ordinary UDM browser handoff sends a URL and selected headers. IDM's extension has code for preserving the request method, raw/form body and proxy context. This difference can affect whether a download works at all, before transfer speed becomes relevant.

## What was actually inspected

- Fresh read-only PE inventory of **44 native components**: no inspection errors; all hashes match the earlier reference snapshots. This lets the prior component and driver analysis remain applicable.
- **105 dialog templates** in IDMan.exe, **124** across the inventoried components; two compiled menus, **74 distinct menu commands** and 156 string-table entries. These are resource counts, not feature counts.
- **100 candidate MFC command-map bindings**, including ordinary handlers and UI-update entries. Selected menu-to-code paths were verified with Ghidra or Capstone.
- **35 distinct native functions** successfully decompiled in the new targeted traces. Decompiled output is inferred pseudocode, not recovered original C++ source. The pre-existing broad analysis was reused only after input hashes were checked.
- Three bundled browser extension archives were read as ZIP/CRX data without executing their scripts. The formatted Chromium 6.43.1 sources match the freshly inspected archive hashes. The bundled Edge and Firefox manifests are version 2; this does not establish which bundles the user's live browsers currently load.
- Current UDM source was checked against the recovered controls and behavior. One isolated process test reproduced a missing capability; the analysis-only changes were not separately regression-tested.

IDMan.exe SHA-256: `03cc62e9adb77a380f9dc12f67ccaaee5106f12844aa73ce32c914ddd16d607c`.

## Newly reproduced failure

A controlled local endpoint generated a file only when it received `POST` with a small form body:

| Request path | Observed result |
|---|---|
| Direct reference POST with the form body | HTTP 200; exact expected file bytes |
| UDM's current ordinary native handoff | GET with a one-byte range probe, no body; HTTP 405; zero bytes published |

This proves UDM's limitation for this fixture. It is **not** a live IDM-versus-UDM POST comparison. The corresponding IDM support is established here by static extension code: its request observer accepts GET/POST and stores request bodies; its serializer handles raw buffers, URL-encoded forms and multipart form data. UDM's POST-capable media transport does not solve this ordinary-file handoff gap.

Source: `browser/chromium/background.js:83–96`, `native/Bridge.cpp` ordinary add handling, and `native/Transfer.cpp:257`. The test uses separate state under `benchmarks/feature-reverse-20260924/post-download-fixture`; it does not add a record to the user's history. [Machine-readable reproduction](reference/feature-reverse-20260924.json).

## Native paths traced beyond interface labels

| IDM feature | Recovered path | What it establishes |
|---|---|---|
| Floating download basket | Menu command 32809 → `0x00456DE0` | Toggles a dedicated window's visibility and updates its menu check state. |
| Customize toolbar | Command 33001 → `0x004581F0` → `0x00455440` | Sends the toolbar customization message (`0x41B`). The full customizer internals were not traced in this pass. |
| Structured export/import | Commands 32821 / 32816 → wrappers `0x00482760` / `0x00482770` | Capstone verifies calls to distinct export/import entry points. Import at `0x00477FA0` selects `.ef2/.ief` separately from `.txt`. The export serializer and complete file format were not decoded. |
| Find Next / matching fields | `0x0047C2A0` → `0x0045FD40`, dialog 506 | Native record matching with selectable fields exists. The dialog exposes case/whole-string controls absent from UDM's current Find interface. |
| Grabber template saving | Command 34020 → `0x004E75C0` | Dedicated template workflow exists beyond the ordinary saved-project operation. |
| Grabber project scheduling | Command 34021 → `0x004E9090`, dialog 284 | Dedicated project/schedule configuration and update path. |

The resource decoder follows Microsoft's [menu-template layout](https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-menuitemtemplate). Candidate MFC bindings were treated as hypotheses until selected code targets were inspected. Some tiny wrappers were absent from Ghidra's saved function index; their instructions were inspected separately instead of being counted as successful decompilations.

## Remaining feature register

Priority 1 affects ordinary download compatibility or the core browser/queue workflow. Priority 2 covers substantive parity work. Priority 3 covers secondary preferences. “Missing” means the reviewed UDM implementation has no equivalent workflow; “Partial” names the narrower capability already present. Resource-only evidence establishes that IDM exposes a control, not that every branch works on this PC.

| ID | Feature area | UDM status | Priority | What remains | IDM evidence | UDM location |
|---|---|---|---|---|---|---|
| G01 | Form-generated downloads | Missing | 1 | Ordinary browser handoff does not preserve POST method/body; the local POST-only export fails with HTTP 405. | Extension background.js 2504–2544 and body serializer Bc; controlled UDM fixture. | browser/chromium/background.js:83; native/Bridge.cpp; native/Transfer.cpp:257 |
| G02 | Browser request headers | Partial | 1 | Ordinary handoff forwards selected context only, without the original Authorization or arbitrary required request headers. Manual site logins are a separate implemented path. | Extension requestHeaders handling and native message packing near 2390–2460. | browser/chromium/background.js:91; native/Bridge.cpp |
| G03 | Browser proxy inheritance | Missing | 1 | No per-download propagation of the browser proxy configuration. System/manual desktop proxy settings do not establish equivalence. | Extension proxy.settings observers and proxyInfo packing at 2448, 2548, 2619. | browser/chromium/background.js:91; native/Transfer.cpp HttpSession |
| G04 | Advanced proxy modes | Partial | 1 | No SOCKS4/5 editor, proxy DNS option or separate protocol-specific proxy controls. | Dialogs 140 and 369; official Options documentation. | native/WorkflowsUi.hpp Options; native/Transfer.cpp HttpSession |
| G05 | Failed-file queue retries | Partial | 1 | No equivalent rotation/retry of failed files as queue jobs. Current HTTP request retry/backoff is implemented but different. | Dialog 306; official scheduler failure/retry behavior. | native/Core.cpp Manager::tick and queueRun |
| G06 | Periodic synchronization | Missing | 2 | No queue type that checks remote changes, replaces changed files and retains membership for later synchronization. | Dialog 306; official scheduler documentation. | native/Core.cpp defaultQueue/tick; native/WorkflowsUi.hpp Scheduler |
| G07 | Repeat interval | Missing | 2 | No repeat-every-N-minutes/hours schedule. Daily and one-time windows exist. | Dialogs 306 and 284. | native/Core.cpp inWindow; native/WorkflowsUi.hpp Scheduler |
| G08 | Completion actions | Missing | 2 | No per-file or per-queue run-file/program, disconnect, app-exit or shutdown workflow. | Dialogs 306 and 364; official scheduler documentation. | native/Core.cpp Manager::start; native/Ui.hpp Progress; native/WorkflowsUi.hpp Scheduler |
| G09 | Scheduled-dialog policy | Partial | 2 | Global suppression and minimized queue progress exist; no complete scheduler-specific completion/error suppression policy. | Dialogs 291 and 367; official scheduler documentation. | native/App.cpp completion/event handling; native/Core.cpp QueueOrigin |
| G10 | Queue menus and startup preference | Partial | 2 | No full queue-tree context menus/toolbar chooser or explicit per-queue start-on-IDM-startup equivalent. | Dialogs 306 and 308. | native/App.cpp menus; native/WorkflowsUi.hpp Scheduler |
| G11 | Queue drag ordering | Missing | 2 | No dragging files within or between queues; up/down and membership commands exist. | Official scheduler Files in the queue behavior. | native/WorkflowsUi.hpp Scheduler; native/App.cpp DropTarget |
| G12 | Capture modifier keys | Missing | 2 | No configurable force/bypass combinations for browser capture. | Dialog 229. | browser/chromium/background.js; native/WorkflowsUi.hpp |
| G13 | Address exceptions | Partial | 2 | Host exclusions exist; no complete per-URL wildcard exception editor. | Dialogs 294, 303 and 304. | browser/chromium/background.js; native/WorkflowsUi.hpp Options |
| G14 | Repeated-cancel exclusions | Missing | 2 | No offer to exclude an address/site after a repeatedly cancelled automatic capture. | Dialog 302. | browser/chromium/background.js; native/Bridge.cpp |
| G15 | Video-panel controls | Partial | 1 | No complete per-media-type/minimum-size editor, full/mini mode and independent video-panel exception workflow; live overlay placement needs more site coverage. | Dialogs 340, 370 and 488; formatted extension content.js. | browser/chromium/content.js; browser/chromium/popup.html |
| G16 | Download-all review | Partial | 2 | Selected-link capture exists; missing the complete type-filter, wildcard rename, destination/category and link-description review workflow. | Dialog 228. | browser/chromium/selection.js; browser/chromium/popup.js |
| G17 | Structured history import/export | Partial | 2 | Only text URL lists are implemented. No structured history format/import route comparable to the separate IDM formats. | Commands 32816/32821; wrappers 0x00482770/0x00482760; import function 0x00477fa0 distinguishes .ef2/.ief from .txt. | native/App.cpp CMD_IMPORT/CMD_EXPORT |
| G18 | Export scope | Missing | 2 | No dedicated export-selected/export-queue/export-all chooser. | Dialog 191. | native/App.cpp CMD_EXPORT |
| G19 | Toolbar customization | Missing | 2 | No configurable toolbar button selection/order, size or skin workflow. | Command 33001 -> 0x004581f0 -> 0x00455440 sends the toolbar customization message; commands 34010/34011. | native/App.cpp createToolbar/menu construction |
| G20 | Floating drop target | Missing | 2 | The main window accepts drops; there is no independent floating basket. | Dialog 187; command 32809 -> 0x00456de0 toggles its window visibility. | native/App.cpp DropTarget |
| G21 | Delete completed disk file | Missing | 2 | Removing history does not provide IDM’s explicit companion disk-file deletion option. | Dialog 338. | native/Core.cpp Manager::remove; native/App.cpp CMD_DELETE |
| G22 | Drag completed file out | Missing | 2 | No completed-file drag source for another app/Explorer. | Prior live completion-dialog inspection; dialog 286. | native/App.cpp DropTarget; native/Ui.hpp completion dialog |
| G23 | ZIP contents preview | Missing | 2 | No ZIP contents inspection/download preview dialog. | Dialog 196. | native/Ui.hpp FileInfo; native/WorkflowsUi.hpp |
| G24 | Advanced Find settings | Partial | 2 | Find Next exists; no field selector, match-case or whole-string-only settings. | Dialog 506; Find Next 0x0047c2a0 calls record matcher 0x0045fd40. | native/App.cpp:69; native/MainFeatures.hpp:20 |
| G25 | Category management | Partial | 2 | Rules/custom categories exist; rename/delete and last-used category destination workflows are incomplete. | Category dialogs and Save To controls; official Options documentation. | native/App.cpp CMD_NEWCATEGORY; native/Core.cpp Manager::add; native/WorkflowsUi.hpp |
| G26 | Progress-dialog customization | Partial | 2 | No matching tab visibility, initial view and per-download completion-options configuration. | Dialogs 363–367. | native/Ui.hpp Progress; native/WorkflowsUi.hpp Options |
| G27 | Start with Windows | Missing | 3 | No native Options preference for startup registration. | Dialog 139; official Options documentation. | native/WorkflowsUi.hpp Options; install.ps1 |
| G28 | Event sound selection | Partial | 3 | Completion sound exists; there is no full configurable sound-per-event editor. | Sounds resource and official Options documentation. | native/App.cpp completion; native/WorkflowsUi.hpp Options |
| G29 | Manual-download User-Agent | Missing | 2 | No editable User-Agent for manually added downloads; browser-captured User-Agent is separate. | Dialog 291. | native/Transfer.cpp HttpSession; native/WorkflowsUi.hpp Options |
| G30 | Path-specific site logins | Partial | 2 | Encrypted origin-based saved credentials exist; different paths of the same site cannot select different saved credentials. | Dialogs 145/146; official Options documentation. | native/Core.cpp Manager::add; native/WorkflowsUi.hpp Options |
| G31 | FTP resume and controls | Partial | 2 | Fresh sequential FTP exists; resumable FTP, full passive/proxy configuration and live coverage remain incomplete. | FTP/proxy controls in dialog 140; current transfer implementation. | native/Transfer.cpp FTP path |
| G32 | Grabber templates | Missing | 2 | No reusable template library/wizard. | Dialog 257; command 34020 -> 0x004e75c0. | native/WorkflowsUi.hpp GrabberDialog |
| G33 | Advanced grabber filters/login | Partial | 2 | No full page/file include-exclude rules, manual-login workflow or logout-page protection. | Dialogs 257 and 264. | native/Transfer.cpp explore; native/WorkflowsUi.hpp GrabberDialog |
| G34 | Grabber project schedules | Missing | 2 | No independent explore-only/explore-and-download/synchronize project schedule. | Dialog 284; command 34021 -> 0x004e9090. | native/WorkflowsUi.hpp GrabberDialog; native/Core.cpp projects |
| G35 | Offline site mirroring | Missing | 2 | No original-directory layout plus rewriting downloaded links for offline browsing. | Dialog 259; official Grabber documentation. | native/Transfer.cpp explore |
| G36 | Language selection | Missing | 3 | Native UI remains English without a language catalog/selector. | Dialog 208. | native/App.cpp; native/Ui.hpp; native/WorkflowsUi.hpp |
| G37 | Updater/distribution repair | Partial | 2 | Portable/current-user setup exists; no complete signed update/repair/rollback distribution workflow. | Update menu/notice and installed setup components; UDM packaging. | package.ps1; install.ps1 |
| G38 | Active driver integration | Partial | 2 | UDM’s optional WFP monitor is not equivalent to IDM’s observed redirection, process tagging and stream handling. Production signing/kernel behavior remain unverified. | Hash-matched prior driver decompilation, stream callbacks and process-tag insertion; docs/reverse-engineering-2026-09-20.md. | drivers; native/Network.cpp |
| G39 | Quota warnings | Partial | 3 | Configurable quota periods work; no matching warning-before-stop/exceeded-quota dialog. | Dialogs 194 and 309. | native/Core.cpp Manager::charge; native/WorkflowsUi.hpp Options |
| G40 | Batch-generation review/auth | Partial | 2 | Numeric/letter ranges work; no comparable preview grid/wildcard-size and batch-specific authorization editor. | Dialog 280. | native/WorkflowsUi.hpp batchDialog; native/Core.cpp expand |
| G41 | Dial-up/VPN connection automation | Missing | 3 | No connection/reconnect/hang-up automation workflow. | Dialogs 185/186; official Options documentation. | native/WorkflowsUi.hpp; native/Core.cpp |

## Features that must no longer be called missing

The old 0.12.2 audit predates completed-file editing, Move/Rename, Redownload, configurable double-click, saved columns, category visibility, dark content colors/font/tray choices, Find Next, File Info prefetch, custom temporary storage, 32 connections/server overrides, selectable quota periods, server timestamps, explicit queue membership, selected-link panels and duplicate choices. Those now exist in UDM. Expired-link refresh, verified resume and atomic publication also exist. The [0.16.1 audit](features-0.16.1.md) records their validation and remaining bounds.

Find Next is implemented, but the match-case/whole-string/field selector remains a gap. Plain URL import/export is implemented, but structured history import/export is a separate gap. HTTP request retries are implemented, but rotation/retry of failed jobs within a queue is another separate gap.

## Browser/media and driver limits

The existing successful 1080p YouTube capture remains valid evidence for that one session. Broad site coverage, embedded-player association and overlay placement still require a real-site matrix. Subtitles, alternate audio selection and live-stream workflows remain UDM limitations, but this pass does not establish complete corresponding IDM capabilities; they are not counted as confirmed IDM parity gaps above. Protected-media UI labels do not demonstrate decryption capability.

The previously traced, hash-matched IDM WFP driver includes redirection/process tagging and stream-handling paths; UDM's optional driver is currently a monitor. That is a concrete component difference. It does **not** establish that the driver explains the ISO speed difference or would fix an omitted POST body, missing request headers, or a video-identity error. No new throughput comparison or kernel test was performed.

IDM also exports a file-type/MIME helper and a SQLite library component. Their presence is evidence of component choices, not proof that UDM needs the same DLLs or that those components cause faster downloads.

## Recommended implementation order

1. Preserve and validate ordinary request context end to end: method, bounded body, required headers and proxy context. Use explicit handling for non-idempotent requests and servers that cannot resume them; copying a POST into every range worker is not a correct implementation.
2. Complete failed-job queue retries, then repeat/synchronization scheduling with tests for stop, restart and file replacement.
3. Finish video-panel filters and player association using repeatable browser fixtures plus live supported-site tests.
4. Add structured history export/import, advanced Find, toolbar/drop-target and batch-review workflows.
5. Complete the grabber and production distribution work as separate features.

The queue/synchronization and completion-action comparisons are corroborated by [IDM's scheduler documentation](https://www.internetdownloadmanager.com/support/idm-scheduler/idm_scheduler.html). Capture keys, proxy modes and related settings are described in [IDM Options](https://www.internetdownloadmanager.com/support/options.html); the larger website workflow is documented in [IDM Grabber](https://www.internetdownloadmanager.com/support/idm-grabber/idm_grabber.html).

Licensing, registration, purchase prompts and marketing commands are intentionally excluded from the UDM gap list.

## Reproduction and retained evidence

- Human report: this file; structured register: [feature-reverse-20260924.json](reference/feature-reverse-20260924.json).
- Fresh inventory, resource decoder, extension manifests, disassembly wrappers, Ghidra logs/traces and the POST fixture are under `benchmarks/feature-reverse-20260924`.
- Ghidra opened the existing project read-only. IDM's installed files were not changed. Reference pseudocode and extension source remain under the excluded analysis directory and are not incorporated into UDM implementation files.
- This pass changes reports/analysis artifacts, not the running UDM executable. Existing history and active transfers were not used as fixtures.
