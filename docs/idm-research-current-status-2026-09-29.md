# IDM research versus UDM: current gaps

Updated 30 September 2026. **Full IDM parity is not established. Native 0.78.0 and browser 0.53.1 are installed on disk. Video preparation/late-submission correction passed 434 targeted and 35 actual Edge checks; 20 changed files and 49 unchanged native/runtime files verified. Personal browser activation remains pending after computer control was stopped.** [Latest video correction](parity-browser-0.53.1.md) · [Firefox handoff](parity-browser-0.53.0.md) · [Native release scope](parity-0.78.0.md).

**New candidate:** native 0.79.0 adds EF2 import/export, reference Tasks-menu routes, text-link extraction, POST-aware duplicate matching and explicit Resume from stopped queues. 2,462 native and 50 MFC checks passed. Source and installer are delivered; personal activation and a live IDM round trip remain pending. [Candidate and limits](candidate-0.79.0.md).

**Earlier candidate:** native 0.80.0 includes 0.79 plus HTTP/2 negotiation for WinHTTP and the explicit-proxy curl path. 2,465 native checks and 45 actual-transfer assertions passed. Source and installer are delivered; personal activation remains pending. [Evidence and limits](candidate-0.80.0.md).

**Earlier candidate:** native 0.81.0 adds per-session explicit-proxy reuse and HTTP/2 multiplexing. 2,465 native, 45 transport and 18 pool checks passed, including seven HTTP/2 requests on one TCP connection. Source and installer delivered; personal activation remains pending. [Evidence and limits](candidate-0.81.0.md).

**Latest candidate:** native 0.82.0 adds direct Dial-up/VPN credential editing. 2,482 native, 30 scoped credential and 37 MFC checks passed. Source and installer delivered; personal activation and real dialing remain pending. [Evidence and limits](candidate-0.82.0.md).

**Verified staged candidate:** native 0.83 / browser 0.54 adds durable adaptive media receipts and corrected media-refresh recovery. Final native 2,541, receipt 59, MFC 81 and host 11 checks passed, plus actual Edge/Firefox handoffs and recovery. The installer is delivered; source remains staged and personal activation awaits the user's choice. [Release and limits](candidate-0.83.0.md).

**New browser candidate:** 0.55 adds bounded nested audio-only HLS traversal. Eighteen unit checks and 68 actual Edge checks pass; Firefox integration and full browser qualification remain pending. [Scope and evidence](browser-055-nested-audio-candidate.md).


**Staged font menu (1 October):** View > Font now offers Choose and Reset to default Font; 87 native and eight actual app checks pass. Exact reference defaults and visual/DPI parity remain unverified. [Evidence and limits](gui-font-reset-candidate-20261001.md).

**Staged limiter correction (1 October):** Fixed rate retention between the Options save path and Speed Limiter menu; failing baseline followed by 90 native and 12 actual app checks. [Evidence and limits](gui-limiter-shared-save-candidate-20261001.md).

**Staged limiter rollback qualification (1 October):** 18 native fault/recovery checks pass, covering real catalog replacement failure, complete preference rollback, stale Options drafts and download recovery. [Evidence and limits](gui-limiter-rollback-candidate-20261001.md).

**Staged Stop all (1 October):** Separate Pause all and Stop all commands and corrected toolbar routing passed 101 native plus 13 app checks. Multiple-download timing and live reference comparison remain open. [Evidence and limits](gui-stop-all-candidate-20261001.md).

**Staged Stop all batch qualification (1 October):** 118 native checks pass, including two receiving transfers plus one queued, deferred-window state and exact resumed bytes. Scheduled cycles and actual modal workflows remain open. [Evidence and limits](gui-stop-all-multi-candidate-20261001.md).

**Staged selected Stop (1 October):** Selected Stop closes affected progress windows while preserving other transfers; 124 native and 13 app regression checks pass. [Evidence and limits](gui-selected-stop-candidate-20261001.md).

**Staged Downloads layout (1 October):** All 15 first-level labels/positions match the installed IDM menu resource; 148 native and 15 app checks pass. Dynamic queue population and visual/DPI matching remain open. [Evidence and limits](gui-downloads-layout-candidate-20261001.md).

**Staged named queue submenus (1 October):** Start/Stop submenus populate current queues directly with name-bound actions; 155 native and 18 actual app checks pass. Exact live reference population remains unverified. [Evidence and limits](gui-queue-submenus-candidate-20261001.md).

**Staged queue save correction (1 October):** Failed Start/Stop now rolls back queue state and preserves running transfers; 14 fault-recovery, 155 native regression and 18 app checks pass. [Evidence and limits](engine-queue-save-candidate-20261001.md).

**Staged build routing correction (1 October):** 17 build/relink paths now select current engine objects; five selector checks and 59 current-engine restore checks pass. Clean-build reproducibility remains open. [Evidence and limits](build-object-routing-candidate-20261001.md).

**Whole-menu audit (1 October):** Actual staged UDM still differs substantially in Tasks, File, View and Help; only Downloads first-level placement matches. Raw reference/runtime menu trees and concrete next priorities are recorded. [Audit](gui-all-menu-gap-audit-20261001.md).

**Staged File menu alignment (1 October):** Four reference File commands now match; nine extra actions remain in the context menu. 169 native and 19 app checks pass. Tasks/View/Help remain open. [Evidence and limits](gui-file-menu-candidate-20261001.md).

**Staged View grouping (1 October):** Nine reference first-level positions now match with UDM branding; 185 native and 21 actual app checks pass. Toolbar contents, tray styles and exact visual behavior remain open. [Evidence and limits](gui-view-menu-candidate-20261001.md).

**Staged Tasks alignment (1 October):** Eleven reference Tasks positions match, with recovery retained under Export; 200 native and 22 actual app checks pass. Help and dynamic/visual parity remain open. [Evidence and limits](gui-tasks-menu-candidate-20261001.md).

## Installed versus staged

| Version or candidate | Current evidence | Remaining work |
|---|---|---|
| Installed native 0.78.0 / browser 0.53.1 on disk | 0.78 native acceptance retained. Cross-site video deadlines: 434 targeted and 35 actual Edge checks passed; 20 files verified and all 25 personal download records unchanged. | Personal activation, older interrupted-journal migration, wider actual-site acceptance, and the earlier unexplained HLS timeout remain open. |
| Integrated handoff, atomic admission and durable dialogs | Shipped in 0.76/0.50. Retained native 2,309 checks; fresh 1,163 extension/native-host checks, 31 MFC checks, Edge/Firefox forms, lost replies and public HTTPS ownership. Complete controlled HTTPS recognition: 13 cases / 23 assertions. | Changed-target review is installed in 0.77; legacy review is installed in 0.78, with personal activation and broader lifecycle cases remaining. Focused acceptance does not establish every GUI behavior. |
| Native 0.77 / browser 0.51 captured-link review | Previous release acceptance: 2,381 native checks, 1,171 checks across 44 browser/host scripts, 67 MFC checks, and isolated Edge restart/popup plus lost-reply transfers passed. [Acceptance](parity-0.77.0.md). | Legacy review is added in 0.78; broader live timing remains open. |
| Native 0.75 automation | Separate uninstalled candidate. Running-server calls work; automatic COM launch fails with `0x80040154`. | Resolve cold activation, machine-wide lifecycle and integration with the newer handoff work. |
| Browser 0.53.1 video deadlines and retained HLS catalog work | A separately reproduced late-submission defect is fixed. Real delayed player/settings replies terminate without late jobs; HLS/direct retries and seven decoded outputs passed. Earlier nested HLS acceptance remains retained. | Original audio-only timeout remains unexplained; broader real-site coverage, lost native media acceptance and nested audio masters remain open. [Latest evidence](parity-browser-0.53.1.md). |

The 0.49 candidates and older installers are not interchangeable. Some use the same version string despite different source. The combined [0.78/0.53.1 installer](parity-browser-0.53.1.md) contains the integrated handoff, dialog, recognition, multipart, nested HLS and legacy recovery work. It does not include the separate COM candidate or establish full parity.

Evidence: [durable dialogs](D:/UDM/docs/browser-handoff-presentations-2026-09-29.md), [atomic admission](D:/UDM/docs/browser-handoff-atomic-2026-09-29.md), [COM status](D:/UDM/docs/automation-registration-2026-09-29.md), [Original intermittent HLS timeout](D:/UDM/.media-cache/paired-edge-hls/results.json).

## Missing or incomplete functionality

“Missing” means no complete equivalent was found in the inspected paths. “Partial” means a code path exists with material limits. “Unverified” means evidence is insufficient, not necessarily that the feature is broken. An unsupported UDM case is not automatically a proven IDM advantage.

| Priority | Area | What is still missing | Evidence needed to close it |
|---|---|---|---|
| P0 | Browser/native ownership | The duplicate-completion case was observed in the older installed protocol. The paired ownership fixes are now deployed and tested within the 0.76 scope. Legacy protocol-0/1 review is installed in 0.78/0.52 and passed actual isolated Edge tests; personal Edge activation is pending. Firefox manual resume is corrected and its actual popup entry is qualified in 0.52.2. Firefox automatic fallback is corrected in 0.53: real unavailable/refused/rejected GET/POST cases keep their original response, and prepared ownership survives lost replies and actual background reloads. Old already-canceled journals require their prior recovery workflow; wider migration coverage remains open. Tokenless ownership still requires the user to inspect both copies. Changed/deleted refresh target review is installed and tested in 0.77/0.51. | One owner through cancel/completion races, lost replies and restart; usable recovery; one accepted native/browser package active in Edge. |
| P1 — R01/R02 | Recognition and capture rules | MIME/Content-Disposition improvements are installed and passed the controlled 13-case HTTPS matrix. No complete general byte-signature classifier or versioned capture-rule evaluator was found. | Controlled 13-case recognition matrix; conflicting names/MIME/body data, redirects, GET/POST, exclusions, stale documents and error-page negatives. |
| P1 | Video panels and current-video association | Wider YouTube sessions/ciphers, reliable ad exclusion, multiple players, iframes and navigation remain incomplete or unqualified. The Dailymotion adapter explicitly accepts recorded videos only. | Actual Edge main-content/ad transitions, quality switches, fullscreen and navigation; each offer belongs to the selected current video and downloads successfully. Repeat in other advertised browsers. |
| P1 | Media and expired links | Live DASH/SABR/YouTube, live subtitles, some DASH index layouts and direct↔SABR refresh transitions remain unsupported or unqualified. Nested video HLS support is installed; nested audio masters remain outside that installed implementation. A separate uninstalled browser 0.55/native 0.83 candidate has actual Edge/Firefox acceptance for declared audio-only nested masters; see browser-055-nested-audio-candidate.md. | Reference support for each claimed comparison; correct decoded video/audio/subtitles, pause/resume and expiry recovery. Nested HLS tolerance is not itself a proven IDM requirement. |
| P1 | Browser request/session transfer | Installed file/blob support is bounded to supported trusted UTF-8 forms and a 1 MiB total POST body. Larger bodies, non-UTF-8/custom producers and adopting an already-issued single-use response without replay remain open. | Exact request bytes/context/output, one-use semantics and browser/native crash recovery. |
| P1 — R07 | Proxy and transport fidelity | Browser PAC/auto-detect, ambient proxy authentication, QUIC, active proxied FTP and FTP gateways remain open. The installed explicit-proxy curl forces HTTP/1.1. Candidate 0.81 includes HTTP/2 negotiation in both transports and verified explicit-proxy reuse/multiplexing through local-DNS SOCKS; broader proxy variants and GOAWAY/reset recovery remain unqualified. | Effective route/DNS/authentication/failover tests and negotiated-protocol measurements. The HTTP2 difference alone does not establish a speed defect. |
| P1 — R08 | Automatic driver handoff | Signed runtime, monitoring and scoped relay components exist. The desktop path exposes Watch/Snapshot; automatic observation → classification → offer → accepted download ownership is not an accepted complete workflow. | Demonstrated capture beyond extension coverage, cancel/crash recovery and VPN/HVCI coexistence. Identical driver control-code numbers would not supply this behavior. |
| P2 — R06 | COM, Explorer and broker integration | Installed COM automation is absent; the candidate cannot cold-start. Explorer integration and equivalents for wider options/maintenance interfaces remain incomplete. | Cold activation from 32/64-bit callers, batch/error flows and owned setup/repair/uninstall. |
| P2 — R12 | Backup and catalog interoperability | Staged native backup/restore now includes dialogs, process handoff, startup recovery and data-folder exclusion; see recovery-ui-candidate-20261001.md and recovery-directories-candidate-20261001.md. Empty-folder restore and identity-checked cancellation cleanup now pass isolated acceptance. Native recovery-busy replies pass app/host and browser-module checks; see recovery-browser-busy-candidate-20261001.md. Save-failure handoff now preserves the running app and browser pipe; see recovery-close-failure-candidate-20261001.md. Release integration and wider recovery/IDM interoperability remain unproven. | Disk-full/power loss, cross-account migration, live-browser recovery transitions, packaging and real IDM round trips. |
| P2 | Grabber/offline sites | JavaScript-rendered exploration/mirroring, browser-cache reuse and automatic session sharing between independent projects remain open. | Rendered/authenticated fixtures, offline navigation, cookie changes and restart/resume. |
| P2 — R11 | Dial-up/VPN workflow | Candidate 0.82 adds direct in-app credentials with Windows saved passwords and protected session-only passwords; actual credential storage and GUI cases passed. Real configured modem/VPN and manual session-only Connect acceptance remain open. | Real connect/error/cancel cases and the intended credential workflow. |
| Release — R09 | Distribution | App/native host/network helper are unsigned. Complete updater, native ARM64 payload and clean-machine install/upgrade/repair/rollback remain absent or unqualified. | Signed reproducible release and clean-machine lifecycle acceptance. The runtime driver already has a valid signature. |

Rechecked source: [capture gate](D:/UDM/browser/chromium/background.js), [video association](D:/UDM/browser/chromium/sites.js:35), [browser proxy restriction](D:/UDM/browser/chromium/chromium-proxy.js:50), [curl configuration](D:/UDM/native/build-curl.ps1), [network desktop path](D:/UDM/native/Network.cpp:45), [broker](D:/UDM/drivers/signed-network/DesktopBroker.hpp), [export](D:/UDM/native/ExportUi.hpp), [installer](D:/UDM/installer/UDM-Setup.iss).

**HLS timing follow-up (1 October):** 32 focused native stop/save trials and 132 existing live-HLS checks passed. The earlier timing failure did not reproduce; the separate browser audio-only timeout remains unresolved. A dedicated reproducer now records stage timings without changing the 1.5-second bound. [Measurements and limits](hls-stop-investigation-20261001.md).

**Edge audio follow-up (1 October):** native 0.82 / browser 0.53.1 passed 58 actual isolated Edge checks, with ten decoded outputs and exactly one native call/job per selection. Both audio-only selections completed. The historical timeout did not reproduce; old logs lack the trace needed to identify its cause. [Timings and limits](edge-audio-acceptance-20261001.md).

## GUI, reliability and speed still need acceptance

- **GUI:** Complete dialog-by-dialog visual and action comparison remains unfinished: button states, empty/mixed selections, keyboard traversal, modal opening, panel placement and toolbar behavior. Properties, credentials, File Info, duplicate, progress and completion dialogs already exist. The retained 67 MFC checks qualify specific recovery flows, not the entire interface. A staged menu-update defect was reproduced and fixed: 17 native selection/menu checks and six actual app handoff checks passed; see gui-menu-state-candidate-20261001.md. Mixed Stop/Resume dispatch now filters eligible records; 41 native menu/transfer checks pass in gui-mixed-selection-candidate-20261001.md. The reference Arrange files menu is now staged with 69 native and ten app checks; see gui-arrange-menu-candidate-20261001.md. Speed Limiter On/Off/Settings and remembered rate are staged with 80 native and twelve app checks in gui-limiter-menu-candidate-20261001.md.
- **Accessibility/DPI:** System-DPI awareness exists; mixed-monitor transitions, screen reader/high contrast and standard-control dark styling remain unqualified or different. Geometry checks at several scales are narrower evidence.
- **Physical recovery/scheduling:** Real disk exhaustion, removable-device loss, physical sleep/hibernate wake, DST boundaries and actual system completion actions remain unqualified. Fault injection and awake timer checks do not replace those tests.
- **Speed:** The older two-minute ISO pair used different IPv4/IPv6 routes and cannot establish a winner. Repeated sequential same-source/same-route measurements still need stopwatch boundaries, received-byte samples and output hashes. The research's build 11 HTTP2 recovery branch is a lead, not proof of higher speed.

See [workflow inventory](D:/UDM/docs/idm-parity-0.74.0.md) and [HTTP2 branch comparison](D:/UDM/benchmarks/idm-complete-20260927/selected-evidence/http2-comparison.json).

## Do not rebuild these as if they were missing

Remote ZIP preview (R03, 0.71), toolbar Available/Current customization, separators and skins (R05, 0.72), queue-on-app-start and export scopes (R10/R12, 0.73), and repeated-cancel exclusion offers/editor (R04, 0.74) already have implementation and retained tests. Their wider acceptance limits remain.

Completed-file Download Properties, per-download username/password, queues, speed limits, parallel transfers, ordinary link refresh, recorded HLS/static DASH, clear live HLS, static offline sites and browser sign-in handoff also exist. These need their remaining cases completed, not blanket “missing” labels.

## What is still missing from the IDM research itself

The research inventories **988 file entries, 465 unique contents and 59 native binaries**. It retains **88,975 inferred pseudocode exports**, **38 unresolved decompilations**, and **1,801,253 executable-section bytes outside recognized functions**; those bytes may include padding or data. This is extensive static analysis, not recovered original source or complete behavioral understanding.

Unresolved areas include:

1. Full meaning of the file-recognition database's 3,224 rule records and complete native/driver effects of its 81 capture rules.
2. Full message layouts and runtime behavior behind the 11 mapped driver control codes.
3. Indirect calls, unrecognized executable regions and decompiler assumptions.
4. Live setup/driver/browser/network behavior under load; the expanded research pass was static.
5. Complete local-state/auxiliary-format interpretation, browser-package signature semantics and exhaustive visual resource review.

Installed IDM is **6.43 build 10**. Some research is from **build 11**; its changed behavior must not be attributed to the installed reference without testing. [Research and explicit limits](D:/UDM/docs/idm-complete-static-analysis-2026-09-27.md).

## Audit verification and next order

The preceding research audit matched 100 of 101 deployment-receipt entries; the only difference was the already-updated research status document. The personal catalog remained byte-identical with 25 records. Fifteen source anchors, 22 selected evidence files and the presentation candidate's compiled binaries were fingerprinted. UDM app, native host and network helper signatures are NotSigned; WinDivert64.sys is Valid. Retained tests were reviewed, not rerun. That audit changed documentation/evidence only. The later 0.77 deployment verified 37 updated files, preserved all 25 personal records byte-for-byte, and confirmed personal Edge 0.51 activation and connection.

1. Activate browser 0.53.1 in personal browsers when control is authorized, resolve the earlier HLS timeout, and qualify migration of older interrupted journals. New Firefox automatic fallback and actual-background-reload recovery now have live acceptance; preserve those limits.
2. Expand real-site video/player/ad association and independently specified capture rules. Paired integration, controlled recognition, packaging and Edge activation are complete within the recorded release scope.
3. Close browser request/proxy gaps and the automatic driver-to-download workflow.
4. Complete automation/Explorer, backup/interop, rendered Grabber, native GUI/accessibility, physical recovery and matched-route speed qualification.
5. Ship through a reproducible signed release lifecycle.

The historical checklist records 86 implemented, 14 partial, two unverified and one user-deferred rows. They have unequal scope and are not exhaustive, so they cannot justify a parity percentage. English-only remains agreed; trial, registration and purchase screens remain outside the requested UDM scope.

[Audit metadata](D:/UDM/docs/research-feature-audit-20260929/audit-evidence.json) · [Signature check](D:/UDM/docs/research-feature-audit-20260929/signatures.json) · [Reproduction script](D:/UDM/docs/research-feature-audit-20260929/audit.py)


Offline Help and column-width persistence now have current-source acceptance: 200 rebuilt native checks and 33 fresh actual-app checks passed at 120 DPI. Four offline Help routes/F1 work; column widths no longer shrink on reopening. Staged only; remaining Help workflows and broader visual/DPI parity remain open. [Acceptance](gui-help-topics-candidate-20261001.md).


Tip of the Day is staged with reference control positions, eight UDM tips, navigation and persistent startup preference. Final build passed 18 tip lifecycle checks plus 33 GUI/recovery checks; internal recovery launches suppress tips. Artwork, exact runtime reference semantics and remaining Help actions are still open. [Acceptance](gui-tip-day-candidate-20261001.md).


Tree context targeting now distinguishes the keyboard sentinel from negative monitor coordinates and handles empty selections without a diagnostic modal. Current-source native acceptance: 209 checks passed, including nine new real-control cases. Candidate only; physical multi-monitor rendering and full context-menu parity remain open. [Details](gui-tree-context-candidate-20261001.md).


Find dialog now follows reference resource 506 control geometry and labels with a default Find button. Seventeen actual-app search/F3/Cancel checks passed. Staged only; exact font/rendered/DPI equivalence remains unverified. [Acceptance](gui-find-dialog-candidate-20261001.md).


Completed queue members can now be removed/reordered without losing their saved file. Removal atomically clears pending synchronization, and sync startup respects membership. Current-source native suite: 221 checks passed, including rollback and retained/removed sync cases. FileWorkflows is now a fourth required engine overlay object. Staged only; full context-menu and synchronization parity remain open. [Acceptance](gui-completed-membership-candidate-20261001.md).


Completed direct files can now enroll through a synchronization-only queue picker; their saved bytes and Complete status remain intact. Final acceptance: 230 native and eight actual-dialog checks. Multi-selection still uses per-record saves; atomic batch validation/save is the next related gap. Candidate remains uninstalled. [Details](gui-completed-enrollment-candidate-20261001.md).


The preceding per-record queue-save gap is now closed for main-window Move/Remove from queue: whole-selection validation, one save, and rollback of every record share QueueMembership.hpp. Final current-source acceptance: 238 native plus ten actual-app checks. Physical power-loss and broader parity remain open; candidate uninstalled. [Details](gui-membership-batch-candidate-20261001.md).


Scheduler now excludes completed nonmembers and permits removal/reordering of inactive completed members. Fifteen actual-app checks passed, including reopen persistence and saved-file preservation. New overlay headers: WorkflowsUi.hpp (unchanged routing copy) and SchedulerUi.hpp. Candidate uninstalled; broader Scheduler parity remains open. [Details](gui-scheduler-membership-candidate-20261001.md).


Queue drag-and-drop now shares membership validation, supports eligible completed synchronization members, rejects invalid anchors and restores order/data on failed save. Current-source acceptance: 243 native checks plus 15 actual Scheduler checks with queued mouse gestures. Physical/OLE drop and full GUI parity remain open. Candidate uninstalled. [Details](gui-queue-drop-candidate-20261001.md).

Category editor follow-up: staged effective built-in extension display, first replacement/empty overrides, and folder-only site-rule preservation pass 250 native and 10 actual-app checks. See [category acceptance](gui-category-types-candidate-20261001.md). Built-in rename/delete and full parity remain open.

Category control follow-up: staged reference dialog dimensions/positions, site restriction switch, and atomic remembered-path preference passed 254 native and 22 actual-app checks. See [category controls](gui-category-controls-candidate-20261001.md). Built-in rename/delete, arbitrary site wildcard fidelity and full parity remain open.

Predefined category lifecycle: staged rename/delete for six predefined file-type categories now passes 269 native and 37 actual-app checks, including restart and file preservation. All 23 engine objects rebuilt against shared active-category helpers. See [acceptance](gui-category-lifecycle-candidate-20261001.md). Other remains reserved; full parity remains unproven.

Site-restricted category routing: reproduced and fixed builtin/global type matching outside selected sites, empty-list fallback, and Options type editing escaping restrictions. Staged acceptance: 12 focused, 269 native and 27 actual-app checks; see [details](gui-category-restrictions-candidate-20261001.md). Arbitrary wildcard/full parity remains unverified.

Update-check workflow: staged asynchronous Help action passed 15 model, 271 native, 21 dialog and five real-endpoint main-app checks. Latest published v0.78.0 was correctly compared with running candidate 0.83.0. See [acceptance](gui-update-check-candidate-20261001.md). Automatic/signed installer replacement, rollback and full parity remain unfinished.

Help product actions: staged 13-entry reference Help order, project Home/Support and explicit Tell a Friend workflow pass 274 native, ten share-dialog and seven main-app checks. Browser/network tools remain under Options > More. See [acceptance](gui-help-product-candidate-20261001.md). Default email/browser integration and full GUI parity remain unqualified.

Toolbar Tell a friend: staged twelve-action default, explicit command mapping, original UDM link icon and preserved customized layouts. 87 focused toolbar checks passed in three fresh profiles; 274 accumulated native checks passed. First layout failure corrected; one earlier fixture timeout remains unexplained and retained. No deployment or full-parity claim. See [acceptance](gui-toolbar-share-candidate-20261001.md).

Queue membership display: reproduced and fixed stale members in queue-filtered download lists and Q-column values/sorting. Add/Move menu and dialog labels now follow selection membership. 285 native and 13 actual-app checks pass, including restart. Staged only. Distinct queue-state glyphs and full context-menu parity remain open. See [acceptance](gui-queue-membership-view-candidate-20261001.md).

Queue indicators: staged original native artwork now distinguishes download/synchronization, main/additional, and configured scheduled starts in the Q column and tree. 348 native and 15 actual-app checks pass; eight states rendered at 16/20/32 pixels and on light/dark backgrounds. Main queue membership is visible; nonmembers clear. Full accessibility/hover, default synchronization-queue provisioning and exact reference visual parity remain open. [Acceptance](gui-queue-indicators-candidate-20261001.md).

Default synchronization queue: staged one-time initialization now creates/reuses a stopped synchronization primary queue, preserves custom names/settings/downloads, avoids collisions and honors later explicit deletion. Primary icon identity uses retained role metadata. 17 migration, 349 native and 22 actual-app checks pass. No deployment; reference naming/protection policy and full parity remain unqualified. [Acceptance](gui-default-sync-queue-candidate-20261001.md).

Queue deletion: fixed incompatible completed-file reassignment, stale pending/manual cycle state, and unsafe synchronization-to-download mode conversion. 15 deletion, 17 migration, 349 native and 26 actual-app checks pass, including locked-save rollback and restart. Exact reference deletion/protection policy and project/template references remain unqualified. Staged only. [Acceptance](queue-deletion-candidate-20261001.md).


## 1 October — full candidate regression and synchronization completion

The staged native GUI candidate now passes the full native suite: **2,545 checks, zero failures**. The complete browser 0.55 script suite passes **1,326 reported checks across 48 scripts**; its nine real native-host protocol checks also pass after the engine fix. The relinked native GUI passes 349 checks, publication/reload policy passes 16, and monitor protocol plus read-only runtime verification passes 15.

The full run exposed a real bug: completing a synchronization replacement cleared its queue membership, so later cycles skipped it. Publication and the replacement recovery journal now retain eligible explicitly enrolled current versions and remove archived versions from membership. Ordinary, explicitly removed, media/POST and FTP completion remain outside synchronization membership. Existing regression fixtures were updated for explicit completed-file enrollment, reserved Other category semantics, and the two default queues; original failures are retained.

No installed binary or personal catalog changed. The monitor fixture needed its existing network package available beside the binary; it did not load the driver. A separate publication fixture initially omitted its destination directory; correcting that setup yielded 16 passing checks with unchanged engine code. Candidate release/version assembly and broader IDM parity remain unfinished. [Detailed acceptance](D:/UDM-Workspace/candidates/native-084-backup/control/sync-membership-acceptance.json).


## 1 October — combined native 0.84 / browser 0.55 candidate

A new combined source tree and clean native build are retained at `D:\UDM-Workspace\candidates\release-084-055`. All 23 engine translation units were built afresh. Native version metadata is shared between About, browser diagnostics, update checks and executable resources; all three distributable executables report 0.84.0. Incremental builds now consider every local native header, including new queue-policy headers. The installer license destination was corrected.

The combined binaries passed **2,545 native checks**, **27 actual desktop checks**, **68 isolated Edge checks with ten fully decoded media downloads**, **seven real host cold-start/reconnect checks**, **nine framing checks**, **15 monitor protocol/runtime checks**, and **15 update-version checks**. The temporary Edge host registration was removed. An initial UI fixture failed to dismiss About through WM_COMMAND; sending BM_CLICK to its actual OK button passed with unchanged binaries.

The paired installer compiled successfully, but it is **not deployment-ready or installed**. A read-only upgrade audit found that current Edge registration is per-user and points to `D:\UDM\release\Udm.NativeHost.exe`, while the candidate installer only writes machine-wide keys. Microsoft documents that Edge searches user keys first; therefore installing this package can leave the browser connected to the old host. Upgrade/uninstall registration ownership must be implemented and isolate-tested before activation. Existing app, host, monitor and the personal catalog remain byte-identical. Full IDM parity remains open. [Candidate acceptance](D:/UDM-Workspace/candidates/release-084-055/control/release-candidate-acceptance.json) · [Upgrade gap](D:/UDM-Workspace/candidates/release-084-055/control/installer-upgrade-gap.json) · [Microsoft registry search order](https://learn.microsoft.com/en-us/microsoft-edge/extensions/developer-guide/native-messaging#step-3-copy-the-native-messaging-host-manifest-file-to-your-system).


## 1 October — installer browser registration upgrade

The 0.84/0.55 installer now registers the host for the current user and both machine registry views. It writes UTF-8 manifests beside its own installation, checks all writes, and restores prior registration values and manifest bytes after failure. Uninstall removes only default values that still point to this installation, leaving a replacement owner and unrelated values intact.

**100 compiled shared-code fixture checks and five independent manifest/cleanup checks passed.** The fixture exercised successful upgrade, another installation's uninstall, ownership changes, partial/final write failures, existing/new manifest rollback, storage failure, unsupported registry values, retries and Unicode paths. All test registry keys were removed. The three production scopes were redirected under a unique HKCU fixture namespace, so this does not qualify elevated HKLM permissions or multi-user setup. The production installer compiled successfully; the 131 application/browser/runtime payload files are byte-identical to the accepted build. [Registration acceptance](D:/UDM-Workspace/candidates/release-084-055/control/registration-upgrade-acceptance.json).

Activation remains pending because the active application uses a custom portable data location resolving to `D:\UDM\user-data` (25 downloads), while the default local profile has a distinct 13-download catalog. A new installation directory must preserve that resolved location without reinterpreting its relative path. Neither catalog, installed binaries nor actual Edge registration changed. [Data-location gap](D:/UDM-Workspace/candidates/release-084-055/control/installer-data-location-gap.json). Full IDM parity is still unestablished.


## 1 October — installer preserves the existing catalog location

The staged 0.84/0.55 installer now resolves the previous installation's portable data path before publishing its browser registrations. It preserves matching destination configuration, rejects conflicting catalogs, and creates an absolute custom data location only when needed. Failure rollback removes only its unchanged owned configuration; existing history and download files are neither copied nor moved. Default-profile installations remain profile-relative.

The helper passed **47 CLI checks**, **24 deterministic race/failure checks**, **127 compiled installer integration checks**, and **10 actual app migration/restart checks**. The real app reopened the original three-record fixture twice and preserved paused states and files. A read-only inspection of current registrations selected `D:\UDM\user-data`; both personal catalogs and installed executables remain byte-identical. The 131 existing package inputs are unchanged; the new helper is embedded for setup only. The updated production installer compiled successfully and remains staged, not installed.

Elevated installation/uninstallation, alternate administrator identity, multi-user behavior and physical interruption qualification remain open. Full IDM parity is not established. [Migration acceptance](D:/UDM-Workspace/candidates/release-084-055/control/data-migration-acceptance.json) · [Combined candidate](D:/UDM-Workspace/candidates/release-084-055/control/release-candidate-acceptance.json).


## 1 October — preserve replacement manifests during failed setup

A new compiled fixture reproduced two rollback defects: a failure deleted a replacement of a newly created manifest and overwrote a replacement of a preexisting manifest. The installer now restores/removes only files whose contents still match its published UTF-8 manifest, and only after its own successful publication. Restoration of prior bytes uses a staged rename. The unchanged compiled code passes **155 registration/manifest checks** and **134 full data/registration integration checks**, with private registry cleanup independently verified. All 132 payload files, including the setup-only helper, remain byte-identical, as do installed executables and both personal catalogs. The production installer was rebuilt and remains staged.

An initially suspected expandable-registry-string defect was disproved by 137 passing pre-edit checks: Inno preserves that existing type. No production change was made for it. The retained fixture compile/setup errors are documented in the acceptance record.

Manifest comparison and restoration remain separate filesystem operations; an additional change between them and fully simultaneous installers are not qualified. Elevated lifecycle, alternate administrator identity and multi-user validation are also open. Full IDM parity remains unfinished. [Ownership acceptance](D:/UDM-Workspace/candidates/release-084-055/control/manifest-ownership-acceptance.json) · [Inno string-type behavior](https://jrsoftware.org/ishelp/topic_isxfunc_regwritestringvalue.htm).


## 1 October — completed Properties queue membership

The separate candidate now lets Advanced download properties edit a completed file's queue membership. The same engine validation used by main-window queue actions is applied before saving the edited record. It rejects ordinary-queue enrollment, unsupported transfers, missing saved files and deleted queues; a failed catalog write restores the complete original record. Queue removal clears pending synchronization, while a description-only edit preserves it. Downloaded file bytes are unchanged.

The failing baseline had nine failed assertions. The final candidate passes **22 focused engine checks**, **9 actual dialog checks**, **27 desktop/restart checks**, and **2545 full native checks**. Cancel/OK staging and restart persistence were tested.

The intermediate full run failed two existing live-HLS stop/save timing bounds, with publication taking 2672 ms and 2188 ms. No live-HLS correction or deadline relaxation was made; the final full-run evidence is retained separately. Those intermittent timing failures remain unresolved.

The candidate executable is separate from both the installed app and the paired installer. Installed executables, personal catalogs and all 132 accepted package inputs remain byte-identical. Release assembly is pending. This completes the scoped Properties workflow correction, not complete visual or functional IDM parity.

[Acceptance and exact hashes](D:/UDM-Workspace/candidates/release-084-055/control/completed-properties-acceptance.json)


## 1 October — live Stop and save phase tracing

32 targeted stop/save trials and 32 independently decoded outputs passed. Ordinary trials ranged 265–1079 ms (median 320.5); two-worker loaded trials ranged 313–594 ms (median 383). The slowest traced trial spent 1000 of 1079 ms in finalization. The normal build passed 132 live-HLS checks without diagnostic output. Historical full-suite timing failures did not reproduce and remain unresolved; no performance fix is claimed. Installed files and accepted package inputs are unchanged. [Phase evidence and limitations](live-hls-phase-trace-20261001.md).


## 1 October — Properties correction included in paired installer

The tested completed-file Properties correction is now assembled into all paired native components and the 0.84/0.55 installer. Fresh integration acceptance passed 115 checks; all 132 inputs and protected personal files verified. The prior separate-candidate packaging gap is closed. No personal installation or browser activation occurred, and lifecycle/concurrency/live-HLS/full-parity gaps remain open. [Package acceptance](properties-release-assembly-20261001.md).


## 1 October — keyboard focus traversal

Tab/Shift+Tab now navigate the main-window controls, with toolbar participation and hidden/disabled-control handling. Ten focused MFC checks and 27 optimized desktop checks passed; the paired installer was rebuilt, changing only UDM.exe among 132 inputs. Personal installation is unchanged. No complete IDM shortcut/focus-order claim is made. [Evidence and remaining limitations](gui-keyboard-navigation-20261001.md).


## 1 October — toolbar asset recovery

Reproduced and fixed real-app startup termination from missing/corrupt toolbar PNGs using embedded copies of UDM icons. Three startup cases, 27 desktop checks without external assets, and twelve byte-exact resource checks pass. Installer rebuilt; installed app unchanged. [Evidence and scope](toolbar-asset-recovery-20261001.md).


## 1 October — header Columns menu

Added the documented header customization route, including explicit keyboard-context forwarding. 16 actual-app checks and 27 desktop regressions pass; installer rebuilt and installed files unchanged. [Evidence and scope](gui-header-columns-20261001.md).


## 1 October — column save transaction

Fixed canceled column edits leaking into the visible layout and later saved state after a write failure. 17 fault/recovery, 16 column workflow and 27 desktop checks pass. Installer rebuilt; personal files unchanged. [Evidence and scope](gui-column-save-recovery-20261001.md).


## 1 October — progress limiter control recovery

Fixed visible controls diverging from retained caps after failed saves and temporary limits appearing remembered on reopening. 17 focused dialog and 27 desktop checks pass; installer rebuilt and installed files unchanged. [Evidence and limits](gui-progress-limiter-recovery-20261001.md).


## 1 October — real progress-control transfers

Twenty-nine actual-transfer checks pass for Pause, active Cancel/window-close, Hide, reopen, process restart, range resume and completion controls. Three 8 MiB outputs independently match source bytes and SHA-256. The existing candidate and installed files are unchanged; this is not a speed-parity result. [Evidence and limits](gui-progress-real-transfers-20261001.md).


## 1 October — expired-link refresh metadata

Real HTTP 403 recovery preserves partial data and resumes to an exact 8 MiB output. Fixed stale redirected URL and old authentication context surviving address refresh; rollback/retry/restart checks pass. All four native components rebuilt; 140 focused and component checks pass. Installer delivered; installed files unchanged. [Evidence and scope](gui-refresh-link-recovery-20261001.md).


## 1 October — category icon refresh

Fixed stale download-list icons after category edits. Nine actual MFC checks and 27 desktop regressions pass; the paired installer is rebuilt. Personal files are unchanged. [Evidence and scope](gui-category-row-refresh-20261001.md).


## 1 October — display scaling

Added per-monitor scaling for the main window, dialogs and floating basket. The final candidate passes 63 focused and desktop checks; installer rebuilt, personal files unchanged. Physical mixed-monitor movement remains unqualified. [Evidence and limits](gui-display-scaling-20261001.md).


## 1 October — uninstall manifest ownership

Fixed uninstall deleting replacement browser manifests and registrations. 358 compiled registration/migration checks pass; production installer rebuilt with all 132 payload inputs unchanged. Personal files and actual Edge registration remain unchanged. Concurrent replacement and elevated lifecycle are still unqualified. [Evidence and scope](installer-uninstall-ownership-20261001.md).


## 1 October — installer transaction lock

Added shared lifetime and recursive transaction locks for cooperating setup/uninstall instances. 400 contention, lifecycle, ownership and migration checks pass. Installer rebuilt; all application inputs and personal files unchanged. Cross-session/elevated and non-cooperating concurrency remain unqualified. [Evidence and scope](installer-transaction-lock-20261001.md).

## 2 October — consolidated backend and synchronization probes

The workspace now contains the validated native 0.84.0 backend consolidation. A subsequent local HTTP regression reproduced false successful synchronization from incomplete response bodies, unnecessary replacement records from malformed ranges, and failed checks for valid empty resources. The engine now validates range probes before accepting their metadata or reserving a replacement. Saved bytes, retry outcomes, cancellation, completion events, and restart persistence are covered.

A fresh complete backend run passes **2,874 checks**: 2,545 native and 329 focused checks, including 83 synchronization probe assertions. This updates source and isolated test builds; no installer activation was performed. Full IDM parity and the previously documented browser, driver, lifecycle, and physical-recovery qualifications remain open. [Probe correction and evidence](backend-synchronization-probes-20261002.md) · [Backend consolidation](backend-completion-20261002.md).
