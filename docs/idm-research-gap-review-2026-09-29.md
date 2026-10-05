# IDM research → UDM: gap review, 29 September 2026 UTC

> Toolbar follow-up: [UDM 0.72.0](parity-0.72.0.md) delivers the native Available/Current editor, separators and external skins from R05/G19. F011 remains partial for outstanding visual/interaction qualification. The historical findings below describe the original 0.70 audit; use the linked releases for subsequent work.

> Subsequent implementation: [UDM 0.71.0](parity-0.71.0.md) adds the supported remote ZIP preview workflow from R03/G23. The findings below retain their original installed-0.70 scope; the other gaps remain open.

**UDM still has substantive gaps. The previous “88 implemented” count is not a parity score, and some rows cover less behavior than their feature names suggest.** This review compares the expanded IDM research with the installed UDM 0.70.0 / browser 0.47.0 sources. It adds concrete omissions that were obscured by the old checklist.

## Scope and verification

Reviewed the report from the chat **“Reverse engineer IDM More”**, its file-recognition and capture-rule decoders, driver and HTTP2 evidence, the September 24 gap register, current source, and release acceptance records.

- All **49 files in the 0.70 deployment receipt** still match their recorded hashes.
- Installed IDM still matches SHA-256 `03cc62e9adb77a380f9dc12f67ccaaee5106f12844aa73ce32c914ddd16d607c`.
- The newer research also analyzes **build 11**, while the installed reference is **build 10**. Build 11 findings are identified below; they must not be presented as observed build 10 behavior.
- A new **six-case source-execution probe** confirms the filename-based capture limitation in both Chromium and Firefox bundles. It uses a mocked browser, stops before handoff, and is not a live browser or IDM test.
- Authenticode was rechecked: **UDM.exe, Udm.Network.exe and the current installer are NotSigned; WinDivert64.sys is Valid**. A signed driver does not make the application or helper publisher-signed.
- No application source, installed executable, driver configuration, browser session or user download was changed in this review.

[Hash/anchor evidence](research-gap-review-2026-09-29/review-evidence.json), [capture probe](research-gap-review-2026-09-29/capture-recognition.json), [signature results](research-gap-review-2026-09-29/signatures.json), [reproduction script](research-gap-review-2026-09-29/capture-recognition-probe.cjs).

## Newly identified or previously understated gaps

“Missing” means the inspected UDM path does not provide the behavior. “Partial” means some relevant behavior exists. “Unverified” means the available evidence cannot establish equivalence.

| ID | Finding | Current UDM evidence and practical consequence | Reference / acceptance needed |
|---|---|---|---|
| R01 | **Partial: file recognition** | Automatic capture in [background.js](../browser/chromium/background.js) derives the extension from the browser filename/path. The new probe, with PDF capture enabled, accepts `report.pdf` but skips `download` and `download.bin` despite `application/pdf`, in both bundles. [DownloadPreview.cpp](../native/DownloadPreview.cpp) normalizes server MIME; it does not implement a general byte-signature classifier. Media recognition is a small MIME/path classifier in [media.js](../browser/chromium/media.js). | Research decoded 557 MIME strings, 523 extensions, 1,042 signature blobs and 3,224 rule records in [IDMFType](../benchmarks/idm-complete-20260927/filetype-database/summary.json). This supports a richer recognition architecture; it does not prove IDM captures every example above. Add independent recognition and compare ambiguous MIME/name/signature cases against IDM. |
| R02 | **Partial: site-specific capture rules** | UDM has exclusions, request context, YouTube handling and a Dailymotion adapter. No equivalent general versioned request-rule evaluator was found. Filename/path extraction, method-dependent capture and suppression remain separate hardcoded paths. [sites.js](../browser/chromium/sites.js) explicitly admits ambiguity between playlists and ads in one frame. | Research recovered 81 active rules and traced a shared configuration path into browser and driver consumers. [Rule effects](../benchmarks/idm-complete-20260927/capture-config/browser-rule-effects.json). Full native effects remain unknown. Build independently specified rules with positive, negative, ad and stale-navigation fixtures; do not equate possession of decoded rules with working parity. |
| R03 | **Missing: ZIP contents before downloading** | [App.cpp](../native/App.cpp) enables ZIP preview only for an existing **Complete** local ZIP. [ZipPreview.hpp](../native/ZipPreview.hpp) reads a local file. File Info has no remote archive-preview path. The old F024 “Implemented” label therefore covers post-download inspection only. | IDM documents a Preview button before download in [Starting Downloads](https://www.internetdownloadmanager.com/support/using_idm/starting.html). Need bounded remote central-directory retrieval, authentication/redirect/range handling, ZIP64, cancellation and a File Info entry point. |
| R04 | **Missing: repeated-cancel exclusion offer** | [DownloadInfoUi.hpp](../native/DownloadInfoUi.hpp) cancels by stopping preview/prefetch and pausing the record. No successive-site cancellation counter or exclusion-offer workflow was found in native or browser source. Manually editable exceptions already exist. Historical G14 is still open and was not separately represented by the 103-row inventory. | IDM says two consecutive cancelled downloads from the same site can trigger the offer: [official FAQ](https://www.internetdownloadmanager.com/register/new_faq/sites2_1.html). Need an optional reviewed offer, correct site attribution and persistence; cancellation must not silently add an exclusion. |
| R05 | **Partial: toolbar and GUI fidelity** | Installed [App.cpp](../native/App.cpp) uses custom individual buttons; [DesktopUi.hpp](../native/DesktopUi.hpp) uses a checkbox list. There is no installed external TBI skin loader, separator editor or reference-style Available/Current customization workflow. Existing size/order/visibility settings work. Staged ToolbarModel/ToolbarImages/ToolbarUi headers are **not integrated, compiled or installed**. | Research contains toolbar descriptors and normal/hot/high-DPI bitmap variants. Finish the independent toolbar work, then test rendering, keyboard operation, queue dropdowns, persistence and mixed-DPI behavior. F011 must remain partial for full toolbar parity. |
| R06 | **Missing: external COM automation and shell integration equivalents** | UDM has native messaging and command-line integration. No comparable registered COM automation server/type library or shell extension was found in native/installer code. Existing OLE drag/drop and Windows Open commands do not implement these interfaces. | Research recovered SendLinkToIDM/SendLinkToIDM2/SendLinksArray parameter declarations, options-broker and maintenance interfaces. See [COM findings](idm-complete-static-analysis-2026-09-27.md). Define UDM-owned interfaces and supported caller workflows before implementation; undocumented wire compatibility is not established. |
| R07 | **Unverified/different: HTTP2 negotiation and fallback** | Explicit-proxy [CurlHttp.hpp](../native/CurlHttp.hpp) forces HTTP/1.1; [build-curl.ps1](../native/build-curl.ps1) disables nghttp2. The WinHTTP path has no explicit HTTP2-specific recovery branch or protocol comparison evidence. This is a transport difference, not proof of a speed defect. | Build 11 research identifies an additional conditional `Cancel HTTP2` restart path after error 0x80090304. [Branch evidence](../benchmarks/idm-complete-20260927/selected-evidence/http2-comparison.json). Record negotiated protocols and reproduce relevant failures before choosing an independent recovery design. Do not retry unsafe POSTs indiscriminately. |
| R08 | **Partial: driver-to-download pipeline** | The signed WinDivert runtime, authenticated broker, monitoring and scoped relay components exist. Desktop [Network.cpp](../native/Network.cpp) exposes Watch/Snapshot operations. Automatic classification → download offer → desktop handoff from that network path remains absent from the accepted workflow. Installing a driver alone does not fill this gap. | Research maps 11 IDM driver control codes and capture tag 0x12, but not the full message semantics. [Driver interface](../benchmarks/idm-complete-20260927/selected-evidence/driver-interface.json). Need end-to-end functional tests, cancellation/ownership recovery and compatibility qualification, not identical control-code numbers. |
| R09 | **Partial: installation, repair and architecture coverage** | The app/helper/installer lack publisher signatures; no complete updater and clean-machine upgrade/repair/rollback qualification exists. [Installer](../installer/UDM-Setup.iss) is x64-oriented; native ARM64 components equivalent to the researched architecture-specific payload are absent. | Research reconstructs a two-stage installer, locked-file handling, registration, driver integration and ARM64 selection. These are behaviors to reproduce independently. The setup's filename or its extracted payload is not a runtime installation test. |
| R10 | **Missing/different: explicit queue-on-app-start choice** | Queue menus, schedules and persisted enabled state exist. The inspected [SchedulerUi.hpp](../native/SchedulerUi.hpp) has no separate equivalent to “Start download on IDM startup.” That control is present in reference dialog 306. Existing queue resume/start semantics need a direct behavioral comparison. | Historical G10 remains partial for this preference despite F063 covering queue menus. Test manual stop, app restart, scheduled windows and multiple queues separately. |
| R11 | **Partial: direct dial-up credential editing** | [DialUp.cpp](../native/DialUp.cpp) obtains credentials from Windows phone-book/EAP storage. Windows setup/properties and connection automation exist; an equivalent in-app credential editor was not found. Live real modem/VPN success is also unqualified. | Keep F084 qualified; do not infer complete dialog parity from successful backend mocks. Preserve Windows credential storage and test actual connection/error/cancel flows. |
| R12 | **Partial: export workflow and interoperability** | [DesktopUi.hpp](../native/DesktopUi.hpp) offers a checkbox grid seeded from the selected records, or all records when selection is empty. Rich UDM catalogs exist. The dedicated all/queue/selected scope chooser from G18 is not present, and native IDM .ef2/.ief compatibility is not established. | G17's old “TXT only” statement is obsolete, but G18 is only partly addressed. Catalog export is also not a complete application/settings backup. |

## Important open work already recorded

| Area | Remaining behavior or evidence |
|---|---|
| Video capture and quality selection | Wider actual-site/session/browser coverage; robust multi-player/ad attribution; Dailymotion adapter currently rejects non-recorded streams. A successful public Edge 360p test is narrow evidence, not universal YouTube compatibility. |
| Media and refreshed links | Live DASH/SABR/YouTube and live subtitles; remaining DASH index layouts; direct↔SABR refresh changes and public-session rejection recovery. DRM playback is not evidence that an unprotected download is available; protected-content bypass is not an acceptance target. |
| Browser download handoff | Text multipart forms work; file/blob bodies, non-UTF-8/custom producers, one-use response handling, larger bodies and broad browser/site qualification remain open. |
| Proxy behavior | Browser PAC/auto-detect routes remain unsupported even though native PAC exists. QUIC, ambient proxy authentication, active proxied FTP and FTP gateways remain open. |
| Grabber and offline sites | Hierarchical offline ZIP is implemented in 0.70. JavaScript-rendered exploration, browser-cache reuse, sharing sessions between independent jobs and broader real-site acceptance remain open. |
| Native GUI | Many dialogs have control-geometry checks but no complete visual/click/keyboard comparison. Mixed-monitor DPI, accessibility/high contrast and standard-control dark styling remain unqualified or different. The manifest is still system-DPI-aware. |
| Reliability and scheduling | Actual disk exhaustion/removable-device recovery, physical sleep/hibernate wake, DST boundaries and real system completion actions need qualification. |
| Performance | The old two-minute pair transferred 396.422 MiB in IDM and 398.656 MiB in UDM, but used different IPv4/IPv6 routes. It cannot prove equal speed. Need repeated matched-route/source trials, stopwatch boundaries, per-second byte samples and output integrity. [Prior comparison](parity-0.26.0.md). |

The latest [103-workflow inventory](idm-parity-0.70.0.md) remains useful for tracing code and tests. It must be read with the corrections above: **F011 is partial for full toolbar behavior; F024 is partial because remote ZIP preview is missing; G14 remains missing; G10 and G18 are not fully covered by broader implemented rows.** No revised percentage is justified.

## Reconciliation with the older 41-gap research

The September 24 report targets UDM **0.16.1**. Its 21 Missing / 20 Partial counts must not be reused as current status.

- **Code now addresses the main old omission:** G02 headers; G05–G07 queue retries/synchronization/repetition; G09 scheduled dialogs; G11 queue ordering; G12–G13 keys/exceptions; G16 link review; G20–G22 basket/recycle/file drag; G24–G30 Find/categories/progress/startup/sounds/User-Agent/path logins; G32 templates; G34 project schedules; G39–G40 quota/batch review. This is a reconciliation of implementation evidence, not fresh acceptance of every workflow.
- **Implemented with material qualifications:** G08 completion actions are not fully exercised against real system actions; G15 panel controls still need native/site acceptance; G17 has UDM structured catalogs but no proven IDM catalog interoperability.
- **Still partial or missing:** G01 forms, G03–G04 proxies, G10 startup choice, G14 cancellation offer, G18 export scopes, G19 toolbar, G23 remote ZIP preview, G31 FTP proxy modes, G33 rendered Grabber behavior, G35 rendered offline sites, G37 distribution, G38 driver handoff, G41 dial-up credentials/live qualification.
- **Deferred by the user:** G36 languages; English-only remains agreed. Registration, trial and purchase screens are not product requirements.

## What remains missing in the research itself

The [expanded static report](idm-complete-static-analysis-2026-09-27.md) inventories 988 entries / 465 unique contents / 59 native binaries and reports 88,975 recovered pseudocode functions. That is coverage of retained artifacts, not human understanding or runtime equivalence.

1. **38 decompilation failures** remain, with assembly retained. Indirect targets, inferred parameter types and exception/no-return behavior still require analysis.
2. **1,801,253 executable-section bytes outside recognized functions** remain to classify. They may include padding and data, not necessarily missing code.
3. The 38-byte file-recognition rules are decoded but not fully interpreted. Capture flags have partial browser meanings; complete native/driver semantics are unknown.
4. Driver request layouts, effective runtime authorization, setup side effects, recovery under load and live browser/native transport behavior were not exercised in that static pass.
5. Auxiliary stream formats, browser-package signature validation, graphical review and some CHM details remain incomplete. Local download/history formats were not decoded; the state pass recorded metadata only.
6. Build 11's Googlevideo manifest branch and new converter export are leads, not proven explanations for video compatibility or speed.

## Recommended implementation order

1. Close recognition and workflow omissions with measurable fixtures: MIME/name recognition, remote ZIP preview and repeated-cancel exclusions.
2. Finish and install toolbar work; complete the dialog comparison, including the explicit queue/export choices.
3. Improve Edge video association and form/proxy handoff with real-site acceptance and failure/recovery tests.
4. Connect driver observations to a narrowly specified handoff workflow where it adds coverage.
5. Investigate HTTP2 only with protocol/error measurements; run the repeated matched-source benchmark.
6. Finish publisher signing, updater and clean-machine install/repair qualification.

This review changes the gap assessment, not the shipped version. The staged toolbar implementation remains separate until it is integrated and tested.
