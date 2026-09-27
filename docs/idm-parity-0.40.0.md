# UDM 0.40.0: current 103-workflow IDM comparison

This replaces the stale status column in the September 23 audit. It keeps the same 103 workflow IDs so progress can be traced. It is a finite inventory, not proof that these are all IDM features or a parity percentage. Native code and prior release evidence were reviewed; see [this release verification](parity-0.40.0.md) for fresh results and limits.

**Implemented** means a concrete code path exists. It does not mean every native control was clicked against IDM in this session. **Partial** identifies a specific missing behavior. **Different** is a known behavior difference. **Unverified** requires an acceptance test; **Missing** has no complete workflow. **Deferred by user** means explicitly outside the currently agreed scope (English-only). Equal-looking dialogs alone do not close a row.

The original reference inventory, including 105 installed dialog templates, is retained in [the historical audit](idm-feature-audit-2026-09-23.md). Primary reference: [IDM Options](https://support.internetdownloadmanager.com/support/options.html), [IDM user guide](https://www.internetdownloadmanager.com/support/index.html).

| Status | Workflows |
|---|---:|
| Implemented | 88 |
| Partial | 12 |
| Deferred by user | 1 |
| Unverified | 2 |

## Acceptance inventory

| ID / workflow | Current state | Behavior and remaining acceptance | Code / evidence |
|---|---|---|---|
| F001 Completed-file double-click | Implemented | Completed double-click defaults to Properties; Open is a separate command. | [App.cpp](../native/App.cpp) |
| F002 Properties from File and context menus | Implemented | Properties available from menus and keyboard actions. | [App.cpp](../native/App.cpp) |
| F003 Completed properties editing | Implemented | Rename/move, metadata edits and rollback preserve history/file integrity. | [FileWorkflows.cpp](../native/FileWorkflows.cpp) |
| F004 Paused-file properties | Implemented | Paused properties edit URL, destination, metadata and transfer settings. | [PropertiesUi.hpp](../native/PropertiesUi.hpp) |
| F005 Properties source page and authentication | Implemented | Source page, Referer, actual stream links and login editors exist. | [PropertiesUi.hpp](../native/PropertiesUi.hpp) |
| F006 Action enablement | Implemented | Shared action-state rules reject unsafe mixed/active selections. | [App.cpp](../native/App.cpp) |
| F007 Sorting every displayed column | Implemented | Column-specific stable sorting preserves selected records. | [App.cpp](../native/App.cpp) |
| F008 Keyboard context menu | Implemented | Menu key and Shift+F10 route to selected-download actions. | [App.cpp](../native/App.cpp) |
| F009 Column resizing and reordering | Implemented | Column widths, order and sort persist. | [MainFeatures.hpp](../native/MainFeatures.hpp) |
| F010 Column visibility chooser | Implemented | Columns dialog offers visibility, width and ordering. | [MainFeatures.hpp](../native/MainFeatures.hpp) |
| F011 Toolbar customization | Implemented | Toolbar order, visibility, size and text/icon modes; external skins remain outside this row. | [DesktopUi.hpp](../native/DesktopUi.hpp) |
| F012 Find-file filtering | Implemented | Filename, description and address search with case/whole-string controls. | [DesktopUi.hpp](../native/DesktopUi.hpp) |
| F013 Open downloaded file and folder | Implemented | Open and Open folder commands retain missing-file checks. | [App.cpp](../native/App.cpp) |
| F014 Select and remove history records | Implemented | Remove records retains files. | [App.cpp](../native/App.cpp) |
| F015 Remove all completed records | Implemented | Remove completed records retains files. | [App.cpp](../native/App.cpp) |
| F016 Delete files from disk workflow | Implemented | Separate reviewed Recycle Bin workflow; no permanent-delete fallback. | [FileWorkflows.cpp](../native/FileWorkflows.cpp) |
| F017 Duplicate-link choices | Implemented | Ask, reuse/resume, numbered copy and explicit verified replacement. | [Duplicates.cpp](../native/Duplicates.cpp) |
| F018 Category folders and rules | Implemented | Category destinations and extension/host rules. | [FileWorkflows.cpp](../native/FileWorkflows.cpp) |
| F019 Category edit/delete/tree menus | Implemented | Category create, edit, rename and delete workflows. | [FileWorkflows.cpp](../native/FileWorkflows.cpp) |
| F020 Hide category pane | Implemented | Category pane visibility persists. | [App.cpp](../native/App.cpp) |
| F021 Add URL with credentials | Implemented | Add URL includes credentials and URL history. | [Ui.hpp](../native/Ui.hpp) |
| F022 File Info start/later/cancel | Implemented | File Info provides Start, Later, destination, category, description and queue. | [Ui.hpp](../native/Ui.hpp) |
| F023 Pre-download size/type preview | Implemented | File Info independently queries HTTP headers or FTP metadata when no transfer is active. Shows size and server MIME type (Windows file type fallback), supports More > Refresh details and draft login, and skips POST/media/partial jobs. The actual native dialog issued HEAD with prefetch disabled and preserved zero saved bytes; visual/DPI acceptance remains in F095. | [DownloadPreview.cpp](../native/DownloadPreview.cpp) |
| F024 ZIP contents preview | Implemented | Bounded ZIP/ZIP64 central-directory preview; no execution or extraction. | [ZipPreview.hpp](../native/ZipPreview.hpp) |
| F025 Download while File Info stays open | Implemented | Prefetch receives data before confirmation and prevents premature publication. | [Core.cpp](../native/Core.cpp) |
| F026 Clipboard capture | Implemented | Opt-in extension/host-filtered capture offers either a suggestion or automatic File Info. Modal native acceptance remains pending. | [ClipboardPolicy.hpp](../native/ClipboardPolicy.hpp) |
| F027 Drop links onto main window | Implemented | OLE text URL drop routes through File Info. | [App.cpp](../native/App.cpp) |
| F028 Floating drop basket | Implemented | Floating drop basket with commands and saved placement. | [BasketUi.hpp](../native/BasketUi.hpp) |
| F029 Batch numeric/letter sequences | Implemented | Numeric/letter batch expansion, selection review and authorization. | [SelectionUi.hpp](../native/SelectionUi.hpp) |
| F030 Import/export URL lists | Implemented | TXT and rich catalog import/export; optional encrypted credentials. This is not full application settings backup. | [DesktopUi.hpp](../native/DesktopUi.hpp) |
| F031 Command-line integration | Implemented | Native command-line parsing and forwarding have isolated launch tests. | [Launch.hpp](../native/Launch.hpp) |
| F032 Status, bytes, ETA and resume capability | Implemented | Live status, byte counts, resume capability, speed and ETA. | [App.cpp](../native/App.cpp) |
| F033 Segment progress map | Implemented | Range map and individual connection progress. | [ProgressUi.hpp](../native/ProgressUi.hpp) |
| F034 Pause/resume and reopen progress | Implemented | Pause/resume and reopen progress through manager state. | [App.cpp](../native/App.cpp) |
| F035 Close progress stops transfer | Implemented | Progress Close/Cancel pauses; explicit Hide keeps downloading. | [ProgressUi.hpp](../native/ProgressUi.hpp) |
| F036 Per-file speed limiter | Implemented | Temporary and remembered per-file speed limits. | [ProgressUi.hpp](../native/ProgressUi.hpp) |
| F037 Aggregate speed limiter | Implemented | Configurable aggregate or per-download global cap; a stricter per-file cap wins. Existing aggregate behavior is preserved by default. | [Core.cpp](../native/Core.cpp) |
| F038 Completion dialog | Implemented | Modeless completion: Open, Open with, Open folder, Close, Minimize and suppression. | [Ui.hpp](../native/Ui.hpp) |
| F039 Drag completed file to another app | Implemented | Outgoing file drag source exists on completion workflows; native visual recheck is pending. | [DragUi.hpp](../native/DragUi.hpp) |
| F040 Completion actions | Implemented | Per-download actions use cancellable, one-shot countdowns and optional wait-for-others. | [ProgressUi.hpp](../native/ProgressUi.hpp) |
| F041 Progress customization | Implemented | Start mode, detail expansion and optional progress tabs. | [ProgressUi.hpp](../native/ProgressUi.hpp) |
| F042 Per-event sounds | Implemented | Complete, failed and paused event sound selection/playback. | [WorkflowsUi.hpp](../native/WorkflowsUi.hpp) |
| F043 Expired-address recovery | Implemented | Prior real Microsoft ISO refresh resumed retained data and matched the published hash. | [RecoveryChecks.hpp](../native/RecoveryChecks.hpp) |
| F044 Refresh from original page | Implemented | Browser fresh-link handoff supports a pending refresh with review. | [RecoveryChecks.hpp](../native/RecoveryChecks.hpp) |
| F045 Ambiguous replacement link | Implemented | Candidate review precedes applying an ambiguous fresh address. | [RecoveryChecks.hpp](../native/RecoveryChecks.hpp) |
| F046 Safe partial-file reuse | Implemented | Validators/ranges/length checks protect partial-file reuse. | [RecoveryChecks.hpp](../native/RecoveryChecks.hpp) |
| F047 HTTP segmented download and dynamic splitting | Implemented | Parallel HTTP ranges, bounded retries and dynamic splitting; speed parity is a separate row. | [TransferChecks.hpp](../native/TransferChecks.hpp) |
| F048 Connection count and per-server overrides | Implemented | 1â€“32 connections and per-server exceptions. | [Core.cpp](../native/Core.cpp) |
| F049 Stalled-request cancellation and retries | Implemented | Asynchronous WinHTTP cancellation and bounded server-aware retry. | [TransferChecks.hpp](../native/TransferChecks.hpp) |
| F050 Restart persistence | Implemented | Atomic checkpoints/backups and interrupted-job recovery. | [Core.cpp](../native/Core.cpp) |
| F051 Authenticated downloads | Implemented | Basic/Digest challenge workflow, explicit/browsed headers and encrypted saved logins. Full authentication scheme coverage is not implied. | [AuthenticationChecks.hpp](../native/AuthenticationChecks.hpp) |
| F052 FTP transfer and resume | Implemented | Direct FTP supports validated resume, up to 32 fixed segments, cancellation, retries, active/passive modes and SHA-256 publication. Passive FTP tunnels control and data through SOCKS4/4a/5 or an HTTP CONNECT proxy. Tested with fault injection and independent pyftpdlib. Remaining FTP proxy modes are in F079; size/time metadata is not a cryptographic identity. | [Ftp.cpp](../native/Ftp.cpp), [0.40 verification](parity-0.40.0.md) |
| F053 Expected SHA-256 checking | Implemented | Expected SHA-256 prevents publication of mismatched output. | [TransferChecks.hpp](../native/TransferChecks.hpp) |
| F054 Disk-full and interrupted publication recovery | Partial | Checkpoint contention, rollback and publication failure tests exist; real disk exhaustion/removable-device UI recovery remains unqualified. | [CheckpointChecks.hpp](../native/CheckpointChecks.hpp) |
| F055 Named queues and concurrency | Implemented | Named queues with per-queue/global concurrency. | [Queue.cpp](../native/Queue.cpp) |
| F056 Daily/weekday/overnight scheduling | Implemented | Day masks and overnight windows; DST qualification remains outside current tests. | [Queue.cpp](../native/Queue.cpp) |
| F057 One-time schedule | Implemented | Dated one-time schedules can be rearmed and survive restart. | [Queue.cpp](../native/Queue.cpp) |
| F058 Periodic synchronization queues | Implemented | Completed HTTP(S) update detection, verified replacement and retained prior version. | [Queue.cpp](../native/Queue.cpp) |
| F059 Repeat every N hours/minutes | Implemented | Repeat interval bounded by schedule window. | [Queue.cpp](../native/Queue.cpp) |
| F060 Queue completion actions | Implemented | Queue actions/countdowns cancel when new work arrives. | [SchedulerUi.hpp](../native/SchedulerUi.hpp) |
| F061 Separate queued membership from queue label | Implemented | Pending membership is separate from a history queue label. | [Queue.cpp](../native/Queue.cpp) |
| F062 Queue ordering | Implemented | Buttons and drag/drop reorder/move pending queue members. | [SchedulerUi.hpp](../native/SchedulerUi.hpp) |
| F063 Queue menus and toolbar chooser | Implemented | Queue menus and toolbar/tray commands. | [App.cpp](../native/App.cpp) |
| F064 Suppress dialogs for scheduled work | Implemented | Scheduled/repeating/action queues can suppress per-file prompts. | [Queue.cpp](../native/Queue.cpp) |
| F065 Wake computer for scheduled downloads | Partial | Opt-in wake timers now cover one-time and daily/weekday/overnight queue starts, repetition and synchronization. Real Windows timer registration/signaling and app timed downloads/restart are tested while awake. Physical sleep/hibernate wake remains unqualified and depends on Windows policy/hardware; UDM must remain running. IDM documents the same running-app/wake-policy prerequisite. | [QueueWake.cpp](../native/QueueWake.cpp) |
| F066 Chromium/Firefox native messaging | Implemented | Chromium and Firefox native messaging with persistent framing. | [Bridge.cpp](../native/Bridge.cpp) |
| F067 Automatic ordinary download interception | Partial | Ordinary capture and bounded POST handoff work with negotiated limits and encrypted request storage. Native 0.39.0/browser 0.29.0 add durable worker-loss recovery, accepted-job reconciliation and a late-Add release barrier. Real Chrome/Edge worker termination and Chrome/Edge/Firefox form transfers pass. Multipart, larger or incomplete bodies stay in the browser. Ambiguous native crash/disk states require review; arbitrary browser configurations remain unverified. | [0.39 verification](parity-0.39.0.md), [CaptureReceipts.hpp](../native/CaptureReceipts.hpp), [capture-recovery.js](../browser/chromium/capture-recovery.js) |
| F068 Single-link context menu | Implemented | Link/media context menu sends the current browser context. | [../browser/chromium/background.js](../native/../browser/chromium/background.js) |
| F069 Download all/selected page links | Implemented | All/selected links have review and durable submission state. | [../browser/chromium/selection.js](../native/../browser/chromium/selection.js) |
| F070 Capture force/bypass keys | Implemented | Configurable force/bypass keys, trusted gesture matching and bounded lifetime. | [BrowserSettingsUi.hpp](../native/BrowserSettingsUi.hpp) |
| F071 Capture address exceptions | Implemented | Host/URL-pattern exceptions and captured-request exclusions. | [BrowserSettingsUi.hpp](../native/BrowserSettingsUi.hpp) |
| F072 Cross-site video panels | Partial | Geometry/fullscreen/iframe/SPA controls and fixtures exist. Universal real-site panel coverage is not established. | [../browser/chromium/content.js](../native/../browser/chromium/content.js) |
| F073 Panel preferences | Implemented | Compact/hover/corner/menu-width/per-site controls and reset/hide actions. | [BrowserSettingsUi.hpp](../native/BrowserSettingsUi.hpp) |
| F074 Current-video quality association | Partial | Current-video association, format filtering and captured SABR have live successes; unsupported ciphered sessions remain. | [../browser/chromium/background.js](../native/../browser/chromium/background.js) |
| F075 Direct media and recorded HLS/static DASH | Partial | Clear recorded HLS/static DASH supported, including single-file MP4 SegmentBase/SIDX with exact parallel ranges (browser 0.28.0). Chrome/Edge UI and Firefox handoff pass real native downloads. Nested/external indexes, live/protected formats and arbitrary manifests remain unsupported. | [0.28 verification](browser-parity-0.28.0.md) |
| F076 Native video/audio assembly | Implemented | Direct video/audio and supported adaptive streams assembled locally; no yt-dlp resolver. | [Transfer.cpp](../native/Transfer.cpp) |
| F077 Live streams, subtitles, alternate tracks | Partial | Selected audio/language, audio-only M4A, self-contained HLS WebVTT and standalone DASH WebVTT subtitles in MP4 are implemented and tested end to end. External audio-only downloads omit video requests. Live recording, multiple in-band audio, other subtitle encodings and the separate YouTube audio/subtitle path remain open. | [parity-0.37.0.md](parity-0.37.0.md) |
| F078 Network driver | Partial | Signed WinDivert runtime, authenticated elevated helper and desktop monitoring are integrated and live-tested with Test Mode off. Scoped IPv4/IPv6 TCP redirection, HTTP framing/multipart ranges and crash cleanup are tested. Legacy protocol interpretation, automatic driver-level download handoff and broader HVCI/VPN compatibility remain open. | [parity-0.36.0.md](parity-0.36.0.md) |
| F079 Proxy support | Partial | Separate HTTP/HTTPS/FTP overrides with independent encrypted logins and bypass lists; omitted protocols inherit the default. Redirects select the destination protocol route. HTTP/HTTPS support Windows/PAC, HTTP and SOCKS4/4a/5. Passive FTP supports SOCKS and HTTP CONNECT for both control/data, including parallel transfer, resume, remote destination DNS, cancellation and no direct fallback. Active FTP through proxies, FTP HTTP-gateway/PAC modes and browser-specific proxy inheritance remain open. | [ProxyPolicy.hpp](../native/ProxyPolicy.hpp), [0.40 verification](parity-0.40.0.md) |
| F080 Saved site logins | Implemented | HTTPS site/folder logins select the longest matching path at request time, including redirects; explicit per-file auth overrides them. | [WorkflowsUi.hpp](../native/WorkflowsUi.hpp) |
| F081 Download quota | Implemented | Global quota amount and 1â€“168 hour windows. | [Core.cpp](../native/Core.cpp) |
| F082 Temporary-files location | Implemented | New jobs use selected temporary storage; existing parts retain their locations. | [Core.cpp](../native/Core.cpp) |
| F083 Virus scanner integration | Implemented | Executable/arguments, quoted file tokens, monitored results, timeout/restart recovery, per-file rescan and completion gating. Recognized Microsoft Defender and ClamAV executables supply editable parameter presets and documented exit-code interpretation. Actual UDM downloaded a harmless file, ran installed Defender, persisted its result and preserved the file hash. New dialog visually reviewed. ClamAV exit mapping is fixture-tested, not a real ClamAV installation qualification; unknown/custom commands retain generic results. | [Scanner.cpp](../native/Scanner.cpp), [0.40 verification](parity-0.40.0.md) |
| F084 Dial-up/VPN connection automation | Implemented | Windows phone-book setup/properties/connect, opt-in shared automatic connection, retry/cancel and queue gating. Live success against a real modem/VPN remains unverified. | [DialUp.cpp](../native/DialUp.cpp) |
| F085 Manual-download User-Agent setting | Implemented | Manual User-Agent setting with captured browser-header precedence. | [Core.cpp](../native/Core.cpp) |
| F086 Tray controls | Implemented | Tray restore/add/queue/limiter/basket actions. | [App.cpp](../native/App.cpp) |
| F087 Start with Windows preference | Implemented | Start-with-Windows option and rollback on registration failure. | [Startup.hpp](../native/Startup.hpp) |
| F088 Language selection | Deferred by user | English-only accepted by the user. No full localization catalog or language selector; this is not claimed as IDM-equivalent language support. | [App.cpp](../native/App.cpp) |
| F089 Signed installer, updater and repair | Partial | Local 0.40.0 package contains extension 0.29.0 and the existing signed network runtime. Native release activation and browser handoff have local checks. Application/installer publisher signing, updater and clean-machine install/repair/uninstall acceptance remain open. | [0.40 verification](parity-0.40.0.md) |
| F090 Saved projects and extension filters | Implemented | Saved bounded same-origin HTML exploration and selectable file rules. | [Transfer.cpp](../native/Transfer.cpp) |
| F091 Mirroring and offline browsing | Partial | Bounded static offline ZIP, rewritten links/assets and cached resume exist. Rendered/authenticated session mirroring and arbitrary hierarchy preservation remain different. | [OfflineSite.cpp](../native/OfflineSite.cpp) |
| F092 Templates and advanced exploration filters | Partial | Multistep templates and filters exist; authenticated rendered exploration and exhaustive site rules are not covered. | [GrabberUi.hpp](../native/GrabberUi.hpp) |
| F093 Project-specific scheduling and tree actions | Implemented | Saved projects, project tree actions and queue scheduling. | [GrabberUi.hpp](../native/GrabberUi.hpp) |
| F094 Same-server real-world IDM speed parity | Unverified | One prior two-minute ISO pair was near equal with different routes. No universal speed equivalence or winner is established. | [../docs/parity-0.26.0.md](../native/../docs/parity-0.26.0.md) |
| F095 Mixed-DPI/accessibility behavior | Unverified | Network dialog start/events/stop and preserved main-window history verified through native UI on this PC. Mixed-DPI, screen-reader and high-contrast matrix remains open. | [parity-0.36.0.md](parity-0.36.0.md) |
| F096 Configurable completed-file double-click | Implemented | Open versus Properties completed-double-click preference. | [App.cpp](../native/App.cpp) |
| F097 Redownload command | Implemented | Redownload creates a fresh job without silently replacing the saved file. | [FileWorkflows.cpp](../native/FileWorkflows.cpp) |
| F098 Dark-mode preference | Implemented | Content dark theme and custom controls; standard Windows control details remain different. | [ThemeControls.hpp](../native/ThemeControls.hpp) |
| F099 Font customization | Implemented | Font face, size and weight preferences. | [MainFeatures.hpp](../native/MainFeatures.hpp) |
| F100 Tray-icon appearance choices | Implemented | Tray icon appearance choices. | [App.cpp](../native/App.cpp) |
| F101 Find Next navigation | Implemented | Find Next/F3 navigation. | [DesktopUi.hpp](../native/DesktopUi.hpp) |
| F102 Use server file creation date | Implemented | Opt-in application of valid server timestamps. | [Core.cpp](../native/Core.cpp) |
| F103 Selected-text download panel | Implemented | Selected-text download panel has all/off/site-specific preferences and submission review. | [../browser/chromium/content.js](../native/../browser/chromium/content.js) |

## Release gates beyond the historical inventory

- Front end: compare every implemented command and dialog against live IDM, including empty/mixed selections, keyboard traversal, screen reader, high contrast and multiple DPI levels. Complete skin handling and standard-control styling; localization remains deferred by the user.
- Back end: Remaining FTP proxy modes, site coverage and the missing media workflows above need independent implementations and fixture/live tests.
- Browser: test supported features in each advertised browser, real site/session variants, expired links, advertisements, navigation, and unavailable native host. A shared source bundle is not a compatibility test.
- Distribution: signed installers, upgrades, repair and rollback require a real distribution identity and test matrix. Local test signing is not public trust.
- Performance: repeated, sequential same-source trials with matched route, settings and duration; retain per-second bytes and verified output hashes.

English-only is the agreed scope. Full parity within that scope can be claimed only after the open behaviors and acceptance gates are resolved. Existing strengths such as SHA-256 verification are not substitutes for missing reference behavior.
