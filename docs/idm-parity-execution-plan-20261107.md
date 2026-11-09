# UDM 5.25 versus IDM 6.43 build 10

## Scope and evidence

This is the November 7, 2026 follow-up to the [P0–P6 architecture plan](idm-parity-architecture-plan.md). The target is observable behavior under matching conditions, including the small details that make everyday use feel different. UDM keeps its own name, artwork, licensing and implementation.

The November 8 follow-up installs 5.45, adds current paired transfer measurements, and prioritizes the scanner completion layout, browser handoff and sequential startup work. [5.45 installation, comparison and architecture plan](idm-parity-architecture-audit-5.45.md)

The next pass staged 5.28 and measured the reconnect and same-player panel changes. The 5.29 pass fixed the run-once Scheduler Close failure. [5.26–5.28 results](validation/idm-gap-work-5.26-5.28.md) · [5.29 Scheduler results](validation/scheduler-stop-draft-5.29.md)

| Subject | Identity and evidence |
| --- | --- |
| Source | Commit d54c1cdb790ea43c0c6d4d1128a5c8fa4e9e2675 was the observed tip of both remote branches. UDM 5.25 and Chromium and Firefox 0.68.89. |
| Candidate | Five native executable hashes, 76 recorded source hashes and 143 staged files. This production candidate was built without debug symbols. [Build receipt](validation/scheduler-wake-window-5.25-build.json) · [P4–P6 results](validation/p4-p5-p6-5.25.md) |
| Machine at audit start | Program Files was running UDM 4.74. The saved personal catalog had eight paused and four complete entries. This is a preservation check. |
| Installation attempt | A 60,233,885-byte local installer was built from the frozen native stage and the exact 0.68.89 browser bundles. SHA-256 C96D099637ABB1A250D843FF883AD766FC0BB7336F6697366419DE7738D6C828. Windows canceled elevation before setup started. All four installed native programs still reported 4.74 and both extension manifests on disk were 0.68.39. The current catalog SHA-256 matched its pre-install snapshot. An isolated staged 5.25 process supplied the live UDM evidence. |
| IDM reference | Installed file version 6.43.10.2. Its expired-trial dialog blocked normal controls. A new licensed paired run remains open. The retained GUI audit contains actual same-build control and layout observations, including a 125% dark-theme session. [Retained GUI evidence](gui-parity-2026-10-18.md) |
| Live UDM evidence | Staged 5.25 opened with an empty tagged catalog. Its accessibility tree exposed the default toolbar order, six category children including Images, twelve list headers, horizontal scrolling and Find. Eight controlled HTTP trials completed with correct SHA-256. |
| Prior qualification | The previous HLS/DASH browser acceptance used native 5.23 and browser 0.68.88. The native presentation fixture used 5.24. The 5.25 recovery, queue, and Grabber checks are documented separately. [P1–P3 evidence](validation/p1-p2-p3-5.24.md) |

Evidence codes in the inventory are **L** for live in this pass, **R** for a retained actual IDM observation, **F** for an executable UDM fixture, **S** for source inspection, **D** for official IDM documentation, and **U** for a pending comparison. A row can have several codes. Qualify each workflow on one identified candidate.

### Fresh transfer results

Two UDM 5.25 trials ran per case against an owned HTTP origin. Wall time includes launch through observed file completion. All eight SHA-256 checks passed. Setup compilation overlapped the run, so use it to identify candidate behavior and variation. No new IDM transfer times were collected.

| Case | Wall seconds | Accepted HTTP requests | Refused TCP attempts | Extra MiB sent |
| --- | --- | --- | --- | --- |
| Eight connections, ranged origin | 9.362 / 10.892 | 17 / 19 | 0 / 0 | 5.5 / 6.5 |
| Eight requested, server allows two | 20.660 / 20.659 | 13 / 12 | 58 / 54 | 1.8 / 1.4 |
| Five-second outage | 16.967 / 17.304 | 31 / 24 | 71 / 64 | 10.5 / 3.5 |
| Origin ignores Range | 4.456 / 5.945 | 2 / 2 | 0 / 0 | 0.5 / 0.5 |

The capped origin establishes an actionable cost even when final bytes are correct. Prior 5.23 testing configured two connections against the same two-connection cap and recorded zero refusals. Investigate excess admissions and retry pacing while preserving healthy parallel capacity. Historical IDM comparisons found a finish-phase gap on then-current UDM. Later versions changed staging and publication. Measure the current write, flush, hash, scan and rename costs separately before assigning a cause. [Historical paired benchmark](speed-benchmark-2026-10-18.md) · [Benchmark runbook](../benchmarks/speed/README.md)

### Current presentation and browser measurements

- The 5.25 File Info component fixture passed 140 checks after attaching the pinned local media helpers. One instrumented presentation created the window 62 ms after native dispatch and showed it at 531 ms. Child controls continued to appear until 500 ms. Some initial combo boxes spent more time on creation and font assignment than on population. This observation starts inside the native test and was collected under concurrent activity. Repeat it on a quiet system before assigning the cost to native controls or comparing it with IDM.
- In private Edge profiles, the 0.68.89 browser bundle and staged 5.25 native app passed 58 HLS checks and 10 indexed DASH checks. HLS covered actual menu selections, nested and flat playlists, a cyclic playlist reference, MP4, TS, M4A, alternate audio and subtitles with full decoding. Indexed DASH checked exact selected ranges, audio-only access and rejection of an origin that ignored Range. These owned inputs cannot establish public YouTube ad timing or reference menu behavior.
- The 5.25 Grabber wizard fixture passed 166 checks against the retained reference geometry and control behavior. It exercised navigation through the four configuration steps and the results view. The 5.25 Scheduler dialog fixture timed out after 90 seconds without a result. Treat that as a fixture or interaction problem to isolate, with the separate awake wake-timer and queue completion results still valid.
- Both in-page content scripts mark the trigger busy and use an accessibility announcement while waiting for an uncached format catalog. The separate toolbar popup still shows sending status. Inspect B08 and B12 independently. Disabling accessible status would leave keyboard users without feedback while a cold request is pending.

## Architecture and ownership

| Boundary | Owner | Required behavior |
| --- | --- | --- |
| Observation | browser navigation, page observer, capture and site adapters | Scope candidates to browser session, tab, frame, visit and player. Retire stale documents and changed sources. Distinguish main content, ad and unknown provenance. |
| Catalog and panel | browser formats, media, content and popup | Publish a coherent revision of ready choices. Keep focus on the stable row key on refresh. Separate the in-page menu from tools and diagnostics. |
| Handoff | browser background, native bridge, capture recovery and native capture transactions | Bind one user selection to one durable outcome. Reconcile lost acknowledgements using the same receipt. Preserve ownership of non-replayable responses. |
| Admission and presentation | native App, DownloadInfoUi, CaptureAdmission and MediaAdmission | Admit before optional metadata. Present an independent dialog without activating the main list. Apply late details only to the matching offer. |
| Transfer and assembly | native Transfer, CurlHttp, Ftp, Streaming, Adaptive and DashManifest | Own each request and range, entity validation, persisted bytes, stream identity, verification and final publication. |
| Catalog, queues and recovery | native Core, Queue, QueueWake, FileWorkflows and Recovery | Separate eligibility, retry deadlines and user pause. Keep catalog and files recoverable across errors. Fire completion once per eligible cycle. |
| Grabber and integration | native Grabber, GrabberLinks, OfflineSite, Launch, AutomationMain and installer sources | Preserve project identity, output hierarchy and registration ownership. Make failed migration, repair and rollback recoverable. |

### Shared contracts

- **Capture identity.** Reuse the existing offer token and bind it to a stable player, visit, format, representation set, language, container and source. Guard every late response by document and generation. When the chosen representation vanishes, offer an explicit retry or alternative.
- **Size provenance.** Keep unknown, resource length, selected source bytes and verified final bytes distinct. For direct files, bind length to the selected response and byte encoding. For muxed output, require verified staging bytes or proven output layout from complete metadata. Recompute on track, caption, container or assembly changes. Compare the displayed exact size with the final file.
- **Confirmation.** A pending dialog can prefetch into owned staging when enabled. Start consumes that staging session. Later transfers ownership to the chosen queue according to retention policy. Cancel stops its workers and never publishes a final output. A lost acknowledgement recovers the original outcome.
- **Windows and focus.** Measure first actionable paint and foreground ownership independently of window creation. File Info may foreground independently. The main list stays behind the browser. Start and Cancel route focus without raising the list.
- **Rate.** Separate retained bytes from current network rate. An opening prefetch peak represents a real earlier measurement. Verification, merging, scanning and publishing carry explicit phases. Graph gaps and pauses represent missing samples or lack of activity rather than an invented speed.
- **Artifact identity.** Record executable hashes, browser tree hashes, active extension version, media helper versions, setup hash, and process path. A matching version string is only one part of this check.

## Implementation sequence

### Wave 0. Establish a repeatable comparison

1. **DEP-01 — Install the frozen candidate.** Record the setup digest, input receipt and old versions. Snapshot the catalog and preserve the existing data path. Compare the four installed native executables with the candidate, plus the setup helper separately because it runs from setup staging. Verify both installed browser trees and the active extension version in each tested browser. Launch with a tagged private catalog for comparisons. Compare existing download IDs, queue membership, partial byte counts and paused status outside the test. Record installation, active process and browser activation independently.
2. **REF-01 — Establish the licensed reference.** Use an authorized IDM installation in a disposable profile for paired network, scheduler and failure tests. Match theme, monitor scale, browser channel and zoom, connection limits, storage volume, and origin. Use one interceptor per browser profile. Pass a known 1 MiB download through both managers before attempting media. If a workflow cannot be accessed, leave its reference result open and preserve the candidate result independently.
3. **OBS-01 — Add opt-in timing and focus evidence.** Correlate document eligible, first usable catalog, panel visible, click, host admission, File Info first paint, first body byte, exact size proved, Start, last body byte, verification end and publish. Tie browser and native monotonic timelines to a shared run marker. Record transport, foreground owner, count and outcome. Exclude cookies, signed URLs, personal names and filesystem paths. Bound the trace and compare trace-on and trace-off on ten owned trials.

### Wave 1. Make the video workflow feel immediate

4. **VID-01 — Separate readiness from a menu click.** Instrument ad, main-player metadata, hover, keyboard open, and source replacement. Compare when the reference first offers the desired video and when each choice becomes actionable. Reuse prepared catalogs in the existing content/format paths. Warm the visible eligible player under a request budget. Cancel old generations and bound memory. Publish trustworthy ready rows immediately. Load optional choices after the fast path or on explicit request. Keep transient diagnostics in the extension's tools surface. Test that the in-page menu never paints an empty format list for a known ready catalog.
5. **VID-02 — Match compact panel interactions.** Measure player-relative anchor, padding, opening direction, row spacing, alignment, divider thickness, icon and caption baselines, focus ring, hover, pressed state and drag offsets. Test small players, video controls, scroll clipping, iframes, fullscreen, browser zoom and dark pages. Share presentation tokens across Chromium and Firefox. Keep additional Browser Tools reachable through a secondary action if the measured reference hierarchy supports it.
6. **CAP-01 — Remove optional waits from handoff.** Trace cold app launch, warm persistent connection, host reconnection and delayed format resolution. Dispatch the earliest admissible offer. Reserve the native presentation independently of metadata and main-list repaint. Apply asynchronous details only to the matching offer and generation. Reuse receipts for retries. Test two rapid selections, late host replies, tab changes, and browser worker loss during prepare, commit and reconciliation.
7. **MED-01 — Prove exact size without blocking File Info.** Measure click-to-paint and click-to-exact-size independently. Prove direct MP4 length for the selected response. For remuxing, verify the assembled staging file or exact layout from complete metadata. Cover direct and split YouTube, SABR, original X MP4, HLS TS/MP4, indexed DASH, M4A and TTML. Check selected format, final size, codec, resolution, language and timestamps. Decode the whole file. Test in-dialog folder or name change, Cancel, Later, Start and skip File Info. Keep exact-size claims bound to the verified output.
8. **PRO-01 — Show the prefetch handoff accurately.** Record staged bytes before Start, retained bytes after Start, initial progress paint, measured earlier peak, subsequent rates and finishing phase. Reuse the current progress model. Preserve the measured opening burst presentation when it has evidence. Keep the ongoing graph on real interval samples. Test a completed prefetch, fast verification, collapsed details, auto-minimize rules, and tooltips on narrow displays. Require one durable job and one final file.

#### Follow-up for public YouTube session recovery

The 5.26 and 5.28 public 1080p attempts reached UDM. Both stopped before retaining media bytes when the origin required browser attestation. At that point the native stream reader reported a generic exception, so a queued failure could qualify for replay under the queue's file retry policy. UDM 5.30 persists the rejection and uses its existing media refresh flow to replace the session. [Earlier runs](validation/idm-gap-work-5.26-5.28.md) · [5.30 recovery evidence](validation/sabr-attestation-recovery-5.30.md)

1. Give the known stream rejection a distinct result. Propagate it through parallel-worker cancellation and media prefetch. Persist the need for a fresh session for the affected job so automatic and manual retries can explain why the old session is unusable. Preserve the earlier confirmed ranges and keep the exact size unknown until it is proved.
2. Reuse the desktop's existing **Refresh download address** flow. Accept only a replacement from the original video and selection. Require the same quality, representation, language and stream revision before reusing verified data. Clear the recovery state only when the matching replacement commits to the catalog. Keep that state after a mismatch, cancel or failed write.
3. Inject a browser-attestation rejection in the owned stream fixture with no bytes and after a verified segment. Confirm there is no silent replay, another queued job can finish, the recovery state survives restart, and a matching replacement can resume and publish once. Cover a rejected replacement and a later transport outage as separate cases.
4. Repeat public 1080p in an activated browser. Record capture, rejection or accepted response, refresh outcome, first media byte, exact size proof and complete decode independently. Completion still depends on receiving an authorized session that the origin accepts.

5. **Completed in the 5.31 fixture.** Preserve a bounded discard state when the origin repeats a verified selected segment across response boundaries. The exact 5.30 reproduction now completes both selected tracks. The suite also checked header reuse, unexpected continuation, output equality and unfinished unselected side tracks. Public verification still depends on an accepted session. [5.31 cached continuation](validation/sabr-cached-continuation-5.31.md)

6. **Implemented in 5.32.** Start with one request and release the other windows when selected payload arrives. The fixture observed one request on immediate rejection and preserved overlapping requests on healthy streams. It verified exact fresh, resumed and audio-only track outputs and full-cache offline assembly. Controlled first-byte and throughput comparisons remain open. The public 1080p runs with one and eight windows both received media before a later rejection. [5.32 startup results](validation/sabr-startup-probe-5.32.md)

7. **Serial 1080p public completion and exact size from a retained checkpoint verified.** Repeated time-linked replacements preserved the same job and completed a 1,073-second public video on 5.33. UDM 5.34 finished from the same final checkpoint and produced a byte-identical MP4. Both outputs matched their catalog size and SHA-256 and decoded to their expected ends. UDM 5.34 records the rejected window and request cursor without request credentials and holds both session-renewal signals. The 5.35 File Info fixture refreshed the same public checkpoint, displayed the exact assembled size before Start, and published a byte-identical file. Cold exact-size timing, other public selections, installed UI and fewer user interventions remain open. [Publication and renewal results](validation/sabr-session-rejection-5.34.md) · [File Info acceptance](validation/sabr-file-info-prefetch-5.35.md)

8. **Desktop recovery entry points verified in 5.36.** Resume from the main list and Refresh from the progress window open the same timed recovery page. Mixed Resume starts ordinary downloads first, and cancel preserves the remaining held jobs. A separate cold 360p run admitted the correct quality in 1,342 ms before the origin required attestation. It left full 360p completion and cold dialog timing open. [Desktop and cold 360p results](validation/sabr-resume-commands-5.36.md)

   **5.37 File Info follow-up.** Compact File Info constructs its More controls on demand. The native fixture observed 25 child controls before show instead of 41. Cold loopback HLS selections exercised MP4, TS and M4A through Edge and the native host. Each exact byte count and staging hash matched the fully decoded output after Start. Actionable time ranged from 451 to 986 ms. Separate admission-to-UI-dispatch timing from window creation next, then measure a cold public selection and a paired IDM run. [File Info timing and output evidence](validation/file-info-first-paint-5.37.md)

   **5.38 input scheduling.** The native trace found a 0.2 ms median from the pipe notification to UI dispatch and an 80 ms median for the first prefetch startup block on 5.37. Moving startup to a Windows timer let the observer reach the enabled Start button before prefetch in all five traced 5.38 runs. Median click-to-actionable time fell from 568 to 386 ms across ten runs per build. First observed bytes moved from 1,084 to 1,161 ms. All outputs matched the exact pre-Start size and decoded fully. Trace the persistence and worker startup costs next. [Trace and paired UDM fixture results](validation/file-info-input-priority-5.38.md)

   **5.39 media startup persistence.** File Info saves the working folder and pending confirmation together. The worker reuses that checkpoint when its saved record matches. Failed admission rolls back, and interrupted prefetch reopens paused. In three loopback Edge trials per build, median time to the first segment request was 924 ms on 5.39 and 1,587 ms on 5.38. MP4, TS and M4A retained exact-size and full-decode results. Cold public and installed comparisons remain open. [Startup and recovery evidence](validation/prefetch-catalog-startup-5.39.md)

   **5.40 single-click recovery.** Cold public 360p and 1080p selections on 5.39 opened File Info in 597 and 805 ms and showed Refresh after the origin requested attestation. On 5.40, one panel click on the marked recovery page submitted the exact saved video quality. Saving the reviewed offer resumed the retained public 1080p job. It displayed 265,982,413 exact bytes before Start, then published and fully decoded the matching file. Cold exact-size publication and the installed IDM comparison remain open. [Public timing and recovered output](validation/sabr-one-click-refresh-5.40.md)

   **5.41 verified startup.** The first streaming window now commits a timed segment before opening peers. Cold public 360p and 1080p retained 9.985 and 3.000 seconds from zero before a peer requested attestation. The earlier runs retained only initialization. Healthy owned streams still overlap all configured windows. Next verify recovery from the zero-based prefix on a single connection with a fresh browser session. [Startup and recovery evidence](validation/sabr-timed-startup-5.41.md)

   **5.42 single-connection recovery.** The recovery page seeks to the continuous verified prefix, including when later windows have disjoint coverage. A matching replacement after a secondary challenge resumes on one connection. Public 360p advanced from 9.985 to 69.892 seconds. Public 1080p advanced from 3.000 to 65.234 and then 127.200 seconds across two fresh sessions. Each successful replacement reused its entire verified checkpoint. Renewal on the primary connection still limits cold completion. [Serial recovery and checkpoint evidence](validation/sabr-serial-recovery-5.42.md)

   **5.43 capture freshness.** Compare accepted requests from the same visit by request time and prevent late responses from replacing a newer capture. The public control and candidate both reused the checkpoint at 127.200 seconds, reached 189.707 seconds and received a challenge on request four. Both used the browser observer. The loopback test reproduced a separate page observer race when playback reads the response before UDM clones it. [Ordering checks, public control and page observer finding](validation/sabr-fresh-capture-5.43.md)

### Wave 2. Reduce transfer and finishing latency

9. **ENG-01 — Diagnose shared reconnect pressure.** Instrument TCP attempts, accepted responses, peer resets, planned release and route key. Compare two, four, eight and sixteen configured connections against a two-connection cap. Use a bounded budget per effective origin and proxy route if the measurements show competing workers cause the refusals. Keep healthy workers active. After total outage allow a bounded recovery probe and reopen capacity after a validated response. Respect configured and server limits. Test multiple simultaneous jobs and independent routes for fairness. Keep HTTP Retry-After, refusal and invalid ranges distinct.
10. **ENG-02 — Explain sequential and finish-phase cost.** Trace request probe, first response, body write, allocation, last byte, flush, hash, scanner, rename and catalog save. Reproduce with same-volume and cross-volume paths, a single fast stream, parallel ranges, and differing cache conditions. Optimize the dominant current cost. Reuse checksums only when tied to the exact bytes and required validation. Preserve recovery journals, target reservations, overwrite handling and completion-once semantics.
11. **ENG-03 — Qualify route and failure behavior.** Use owned inputs for shifted, ignored, short and truncated ranges, changed validators, caps, slow headers, outages, PAC, authenticated proxy, FTP restart, redirects, and process interruption. Log protocol, byte coverage and retries. Add a physical sleep/hibernate and storage removal matrix in a disposable environment. Keep those results separate from simulated faults.

### Wave 3. Finish desktop, queue and catalog behavior

12. **GUI-01 — Compare one dialog family at a time.** Use the inventory below. Record screenshots, accessible names and control bounds on matching theme, scale and window state. Drive keyboard, default/cancel, menu mnemonics, DPI transitions, saved preferences and focus return. Prefer shared native controls for common behavior and keep Shell icon loading and metadata probes off the paint path.
13. **QUE-01 — Qualify scheduling and completion.** Extend the exact-second wake case across equal and differing saved stop times, disabled stop, midnight crossing, weekday masks, repeat, minute-only legacy values, and DST gap/fold. The 5.25 timer check establishes registration while awake. Run actual suspend/wake in a disposable profile. Test queue ordering, retries, fresh membership, manual stop, scheduled completion, and one recorded action per cycle before enabling any system action.
14. **DATA-01 — Qualify history, portability and migration.** Compare mixed selection, double-click action, delete semantics, duplicate choices, move/rename, missing paths and persisted sorting. Round-trip a real IDM-generated export with synthetic rows, capturing dropped fields explicitly. Reproduce locked state, interrupted restore, rollback and missing partials against fixtures. Keep original catalogs and files recoverable throughout.

### Wave 4. Qualify Site Grabber, Windows integration and release

15. **SITE-01 — Compare the full Grabber journey.** Pair the four configuration steps and results/actions on the same owned site. Exercise templates, authorization, logout exclusions, off-site links, filters, MIME mismatch, redirects, depth, relative folders, pause/reopen, selected output and queue handoff. Record static crawler and browser-rendered capture as separate sources. Stop the origin before opening archived assets, child/back links, video, audio and captions. [IDM Grabber wizard](https://www.internetdownloadmanager.com/support/idm-grabber/grabber_wizard.html)
16. **WIN-01 — Verify real Windows entry points.** Compare Explorer's classic and Windows 11 overflow menus, Internet Shortcut paths with spaces and encoding variants, URL scheme, silent and queued CLI, COM from 32- and 64-bit clients and browser host restart. Exercise upgrade, repair and removal in disposable profiles. Preserve unrelated file associations and register only owned entries.
17. **REL-01 — Close deployment and signing gates.** Build setup from the exact verified payload. Record pre/post hashes, browser activation, setup and migration results, and rollback outcome. Verify publisher signing and the update manifest using the release key. Test clean install, upgrade of custom catalog, repair, locked host recovery and removal. Keep the optional driver on a separate matrix for signing, VPN, HVCI, sleep and network changes.

## Granular comparison inventory

Each row is a testable difference or a pending comparison. **Critical** means correctness, ownership or accessibility can block the workflow. **High** means a routine workflow is materially slower or harder. **Detail** covers small differences that still require a disposition. Work through the common path first, then its keyboard, failure and migration branches. Compare actual values only under matching settings.

### Browser integration and media

| ID | Behavior to compare | Evidence and current UDM position | Acceptance |
| --- | --- | --- | --- |
| B01 High | First in-page button appearance | R/D/S/U. Current source prepares catalogs. A new paired timing distribution is missing. | Time from first eligible main-player evidence to first usable action. Repeat cold and warm. |
| B02 Critical | Pre-roll, ad skip and main video association | User observations, F/S/U. Simulated ad and page identity checks exist. | Before/after skip and natural ad end. Zero ad downloads from a main-video choice. |
| B03 Critical | Correct target with multiple videos | F/S/U. X poster and player matching exist. | Select two feed items, a quoted video and a recycled player. Saved video identity matches each selection. |
| B04 Detail | Anchor and first paint geometry | R/S/U. UDM has its own panel placement code. | Compare alignment, height, width, corner radius, spacing and caption baseline at 100%, 125%, 150% and 200%. |
| B05 Detail | Drag position and dismiss | S/U. Per-site panel offsets exist. | Drag, drag cancellation, reposition, resize, scroll, reload, and repeated visit. Document what is remembered. |
| B06 Detail | Open direction and clipping | S/U. Current menu geometry handles a visible player. | Top/bottom edge, narrow window, iframe bounds, fullscreen entry/exit and zoom. Every row remains reachable. |
| B07 Detail | Icon, text, hover and pressed state | R/S/U. UDM branding remains deliberate. | Compare visual hierarchy, pointer, feedback, contrasting backgrounds and focus ring. Use original UDM artwork. |
| B08 High | Format menu immediate contents | R/S/U. Prepared catalog path and deferred discovery exist. | Compare single choice and multiple qualities, partial readiness, loading text, empty state, ordering and first actionable row. |
| B09 Detail | Choice labels, units and grouping | R/S/U. UDM formats include codec, audio and caption variants. | Check quality, resolution, container, size units, long title wrapping, separators, selected language and duplicate labels. |
| B10 High | Keyboard entry and exit | F/S/U. Source has menu focus logic. | Tab, arrow navigation, Home/End, Enter, Escape, outside click and restoration to the trigger. Refresh preserves the focused choice. |
| B11 Detail | Secondary tools hierarchy | S/U. Browser Tools and refresh are available. | Compare where advanced tools, help, site controls and diagnostics appear. Ordinary download stays one selection away. |
| B12 High | Sending indicator and fallback | S/U. Toolbar popup still contains sending feedback. In-page and toolbar routes need separate evidence. | Compare warm, cold, host unavailable and slow metadata. Use a single acknowledged action and actionable errors. |
| B13 Critical | Worker restart and lost native acknowledgement | F/S/U. Existing receipt suites cover narrow cases. | Stop worker before and after commit in Chrome, Edge and Firefox. Keep one owner and one record. |
| B14 Critical | Capture ownership for ordinary files | D/F/S/U. UDM has staged admission, direct and takeover paths. | Link click, redirect, attachment, viewer, manual add, browser bypass, ignored type and unhandled scheme. Exactly one final file. |
| B15 Critical | POST, blob and expiring browser response | F/S/U. Bounded browser response and context protections exist. | Verify original body provenance and recovery. An already submitted non-idempotent request must never replay silently. |
| B16 High | Browser enablement and exception scope | D/S/U. Site access and host rules exist. | Domain, wildcard subdomain, private window, iframes and extension disabled. Inspect match precedence and policy refresh. |
| B17 High | Browser context menu parity | D/S/U. Compare actual browser menus separately from Explorer. | Link, selection, all links, video, page and empty selection. Check label, ordering, enabled state and target identity. |
| B18 Critical | YouTube 360p/1080p/AV1 and audio | R/F/S/U. Earlier exact-size/live acceptance covered selected cases. | Confirm full decode and exact pre-Start bytes where proven. Match identity and selected tracks for combined, split and streaming paths. |
| B19 Critical | X original MP4 and HLS | R/F/S/U. Original URL selection and HLS paths have coverage. | Replay 1080p and alternate rendition, captions, selected audio, TS and MP4. Verify byte size and EOF playback. |
| B20 Critical | DASH indexes and Period joins | F/S/U. Controlled flat/indexed output and resume exist. | Multiple Periods, uneven lengths, indexes, signed resource change and unsupported join. Check full coverage and no duplicate frames. |
| B21 High | Subtitle and language behavior | F/S/U. Current TTML validation and HLS subtitle gating exist. | Manual and automatic TTML, WebVTT, alternate audio and unsupported track. Verify language, cue text and timing. |
| B22 High | Unsupported and protected streams | D/S/U. Reference documentation distinguishes unsupported/protected media. | Correct absence or actionable explanation. Never pass HTML, an ad or a different quality off as the chosen file. |
| B23 High | Native host packaging and activation | S/U. Matching browser files are an install prerequisite. | Check running extension version, correct executable path, reconnect after idle/restart and repair of an owned host registration. |

Reference descriptions establish the exposed panel, ordinary capture routes, and supported controls. The paired run must establish behavior and timing for this machine. [IDM video panel](https://www.internetdownloadmanager.com/register/new_faq/video5.html) · [IDM panel exceptions](https://internetdownloadmanager.com/register/new_faq/video18.html) · [IDM starting downloads](https://www.internetdownloadmanager.com/support/using_idm/starting.html)

### Desktop and everyday detail

| ID | Behavior to compare | Evidence and current UDM position | Acceptance |
| --- | --- | --- | --- |
| G01 Detail | Title, icon, menu bar and borders | L/R. UDM shows UDM 5.25, IDM retains product branding. | Document intentional branding, window dimensions, minimum size, menu order and non-client padding. |
| G02 High | Startup, restore and redraw | R/F/S/U. Startup caching and repaint work exist. | Warm and cold launch, tray restore and large history. Measure first actionable frame, first visible list, scroll latency and idle CPU separately. |
| G03 Detail | Toolbar order and labels | L/R. Clean UDM exposed eleven controls from Add URL through Tell a Friend. | Same order, keyboard reachability and split arrows. Compare disabled/pressed appearance and tooltip delay. |
| G04 Detail | Toolbar layouts and skins | R/F/S/U. UDM has saved skins and a gallery action. | Icon-only, text-only, 3D, reset, missing skin, customize persistence and high DPI. |
| G05 Detail | Category defaults and expansion | L/R. UDM has Images, a retained default difference. | Decide disposition for Images without rerouting existing profiles. Compare expand/collapse, empty custom categories and last selected node. |
| G06 Detail | Tree and list divider | L/R/U. UDM exposes a category close control. | Initial splitter width, minimums, hide/show, cursor, scrollbars and persistence after restart. |
| G07 Detail | Columns and drag sizing | L/R/F. Clean UDM exposed twelve headers including Q, Last Try Date, Referer and Parent web page. | Compare default visible order and widths, horizontal overflow, sort arrow, header right-click and customization persistence. |
| G08 High | Row text and sorting | R/F/S/U. Duration fallback and whole-byte rate sort keys exist. | Test unknown and zero size, missing time, transfer rates, equal keys, Unicode, locale and descending ties. |
| G09 Detail | Row icons, selection and focus | R/S/U. Current list uses native views and lazy icons. | Compare active/inactive selection, keyboard range select, focus rect, double-click, icon fallback and scroll anchor. |
| G10 High | Action enablement | R/F/S/U. Source tracks action availability by job status. | Empty selection, mixed completed/paused/active, missing target and failed publication. Compare toolbar, menu and context actions together. |
| G11 Detail | Find and quick filter | R/F/S/U. Find and optional quick filter exist. | Search focus, next/previous wrap, case options, hidden columns, no-match message and keyboard return. |
| G12 High | Add URL and clipboard | R/F/S/U. Native Add URL and clipboard capture exist. | URL present/absent, selected text, duplicates, invalid scheme, history combobox, OK/Cancel and focus. |
| G13 High | Batch builder and selection | R/F/S/U. Number/letter expansion and preview are implemented. | Match From/To/width, wildcard validation, First/Second/Last samples, authentication scope, sort and selected rows. |
| G14 Detail | Basket and drag/drop | R/S/U. UDM supports the drop target. | Empty/valid/multiple URL drops, icon hover and drag affordance, dock position, disabled state and multi-monitor restore. |
| G15 High | File Info opening path | R/F/S/U. 5.24 component evidence supports independent presentation. | Compare live browser and desktop entry separately. File Info appears before optional network metadata with main UDM behind the browser. |
| G16 Detail | File Info layout and tab order | R/F/S/U. UDM has separate fields and filename preview. | Compare group boundaries, labels, icon, control widths, Save To, description, size, address, browse, default button and Escape. |
| G17 Critical | Confirmation and duplicates | F/S/U. UDM has transactions, reservation and duplicate choices. | Start, Later, Cancel, Enter/Escape, folder change and duplicate target while prefetching. Verify catalog and both destination paths. |
| G18 Detail | Size updates and clipboard | R/F/S/U. Provenance is implemented for selected paths. | Unknown-to-known layout without clipping or focus loss. Tooltip/copy text and rounding must preserve exact bytes when required. |
| G19 High | Progress window lifecycle | R/F/S/U. Manual and queued presentation rules exist. | Initial focus, taskbar, minimized/tray/suppressed states, Stop, close and rapid completion. Compare no main-list foreground flash. |
| G20 Detail | Progress fields and connection map | R/F/S/U. Recent source exposes DASH ranges and tracked resources. | Compare status wording, timer tick, units, speed, ETA, resume badge, connection row count, splitter and details toggle. |
| G21 High | Limiter and graph | R/F/S/U. Limiter controls and recorded graph measurements need current runtime comparison. | Current and queued limits, pause/resume, opening retained bytes, rate sample gaps, narrow viewport and accessible tooltip dismissal. |
| G22 Detail | Completion dialog | R/F/S/U. Dedicated reference runner exists. | Compare labels, ordering, Open, Open with, Open folder, copy, Don't show again, suppression and missing target. |
| G23 High | Properties, address refresh and preview | R/F/S/U. Address refresh, ZIP preview and audio metadata exist. | Edit, save/cancel, changed signed URL, wrong file, metadata read failure, filename preview and missing file. |
| G24 Detail | Options tabs, size and default controls | R/F/S/U. Nine-page native fixture exists. | Compare tab order, dialog size, OK/Help, Escape and Enter, checkbox defaults, browse buttons and persistence. |
| G25 High | Options behaviors by page | D/R/S/U. General, file types, connection, save folders, downloads and other controls exist. | Compare per-site exceptions, proxy/auth, file matching, scanner, limiter, duplicate and dialog preferences using safe fixtures. |
| G26 Detail | Theme, DPI and accessibility | R/F/S/U. Native controls support baseline accessibility. | Light/dark, high contrast, 100–200% scale, mixed monitor transition, keyboard only, screen reader, focus and tab wrap. |
| G27 Detail | Tray, notifications, tips and Help | R/F/S/U. Existing source includes trays, tooltips and help. | Compare reveal, icon styles, system tray menu order, startup tip dismissal, sound and notification suppression. |
| G28 Detail | Language and About | R/S/U. English-only and UDM license UI are current scope differences. | Document intentional localization scope and branding. Check overflow, version display, accessible links and keyboard. |

[IDM main window](https://www.internetdownloadmanager.com/support/using_idm/using_idm.html) · [IDM options](https://www.internetdownloadmanager.com/support/options.html) · [Retained control inventory](gui-parity-2026-10-18.md)

### Transfer, queues, storage and integration

| ID | Behavior to compare | Evidence and current UDM position | Acceptance |
| --- | --- | --- | --- |
| E01 High | Startup to first byte | L/R/U. Earlier and current trial methods measure different versions and loads. | Separate process start, probe, first validated response and first body byte. Report cold and warm. |
| E02 High | Range assignment and tail | L/R/S/U. Current 5.25 ranged trials split 15–17 times and served 5.5–6.5 MiB extra. | On matching origin, compare workers, split timing, overlap, duplicate tail and goodput. No missing range. |
| E03 High | Refused connection budget | L/S/U. The capped origin saw 54–58 refusals in 5.25. | Compare current and revised policy with two accepted connections and eight requested. Measure refusal count and wall time. |
| E04 High | Recovery after outage | L/R/S/U. 5.25 recovered both outages. | Time from origin recovery to first useful byte. Count refused attempts, retained bytes and extra requests. |
| E05 Critical | Resume and entity change | F/S/U. Validators and durable checkpoints exist. | Pause/resume, changed ETag, short or ignored Range, lost partial and process restart. Resume only when identity and coverage are proven. |
| E06 High | Sequential, allocation and finish | L/R/S/U. Current sequential trials took 4.456–5.945 seconds. | Measure transport vs disk and finalization. Repeat without background compression. Trace sync, hash, scan, rename and catalog steps. |
| E07 High | HTTP protocol and proxy routes | R/F/S/U. UDM chooses direct and proxy transport paths. | HTTP/1.1, HTTP/2, HTTP/3 where supported, PAC, manual proxy and per-site policy. Record negotiated route and effect on parallelism. |
| E08 Critical | Authentication and FTP | D/F/S/U. UDM has FTP, site login and Windows authentication paths. | Active/passive configuration as supported, FTP restart, certificate failure, proxy auth and same-origin credential rules. |
| E09 High | Limiter and multiple jobs | D/F/S/U. Current source separates global and job caps. | Compare measured sustained cap, start transients, priority and fairness across routes and queues. |
| E10 Critical | File publication | F/S/U. 5.25 passed 38 publication recovery checks. | Same/cross volume, target exists, external modification, disk full and access denied. Preserve prior target and exact completion record. |
| Q01 Detail | Queue names, Q column and default membership | D/R/F/S/U. Normal and synchronization queues exist. | Add via each entry point, completion removal, manual add/remove, tree badge, stable order and queue filtering. |
| Q02 Detail | Scheduler controls | R/F/S/U. The 5.29 fixture passed 692 checks after fixing the disabled run-once stop draft. | Compare tabs, spin controls, defaults, day labels, focus, Apply/Close behavior and error handling on the installed reference. |
| Q03 High | Manual queue start and stop | L/D/F/U. 5.25 shortcut CLI started the Main queue and completed two owned files. | Compare UI and CLI start, stop, start all, reorder, remove and user-paused files. No surprise resume. |
| Q04 Critical | Windows wake and calendar boundaries | L/F/S/U. 5.25 armed the exact selected second on an awake machine. | Overnight, missed start, repeat, DST, system clock change, sleep and manual stop. Verify actual wake on hardware separately. |
| Q05 High | Retry order and synchronization | D/F/S/U. UDM has bounded retry and replacement recovery paths. | Failed entry ordering, server delay, concurrency, changed and unchanged remotes, queue reopening and safe replacement. |
| Q06 Critical | Completion actions | D/F/S/U. Fifteen 5.25 queue completion checks passed. | Record once, cancellation during countdown, failed files, schedule/manual distinctions, and no unwanted shutdown. |
| Q07 High | Download Later | D/F/S/U. Queue selection and prefetch retention paths exist. | Default queue, explicit queue, existing queued copy, invalid queue, cancel and restart. Verify exactly one outcome. |
| D01 High | Category and destination rules | D/R/F/S/U. Category folders and saved rules exist. | Extension/site/parent matching, user overrides, last folder, unavailable drive, UNC and collision. |
| D02 Critical | History and file deletion scope | R/F/S/U. Current source offers recycle/remove workflows. | History-only, disk deletion, mixed selections and missing file. Compare wording, focus and preservation of unrelated files. |
| D03 High | Export and import round trip | R/F/S/U. IDM and text representations exist. | Use reference-generated synthetic export. Compare URLs, description, referer, encoding, optional fields and duplicate policy. |
| D04 Critical | Catalog and partial recovery | F/S/U. 5.25 passed 59 restore checks, range resume and interruption rollback. | Preserve hashes, bytes and queue assignment through restart, inaccessible paths, rollback and legacy records. |
| D05 High | ZIP and audio metadata | R/F/S/U. Remote ZIP and ID3 paths exist. | Compare partial preview, corrupt/truncated input, UTF-8 and legacy names, ID3v1/v2, metadata edit and cancellation. |
| S01 Detail | Grabber wizard step geometry | R/D/F/S/U. The current native fixture passed 166 checks against retained reference measurements. | Titles, Back/Next/default focus, templates, auth expansion, layout and validation at matched scale. |
| S02 High | Crawl and file-filter priority | D/F/S/U. UDM has controlled static and rendered checks. | Path/file include/exclude, MIME, query, redirects, depth, duplicates, navigation and authentication boundaries. |
| S03 High | Result table and queue handoff | R/D/F/S/U. Existing result, selection and action paths. | Check tree/list, selection, output folder, direct start, enqueue and reopen state. Record partial failure and retry. |
| S04 Critical | Offline validity | F/U. 5.25 passed native archive and Edge playback after origin shutdown. | Compare reference link conversion, retained relative tree, unresolved links, byte integrity, child/back and full media playback. |
| W01 High | Explorer and URL scheme | L/F/S/U. Both direct CLI entry points worked on 5.25. | Real right-click registration and discoverability, quoted/Unicode path and ownership on upgrade or removal. |
| W02 High | Automation and shell | F/S/U. Native automation exists. | COM cold and running activation, x86/x64 clients, tagged input, malformed arguments and safe uninstall. |
| W03 Critical | Installer preservation and repair | F/S/U. Setup helper passed custom-path and rollback checks. | Actual same-machine and clean-VM upgrade, locked app/host, rollback, repair and retained personal catalog. |
| W04 High | Signed updater | F/S/U. Update validation exists. | Verify signed exact package with the release key. Reject tampering, wrong version and rollback paths. Verify user-visible state. |
| W05 High | Optional network monitor | S/U. Monitor and browser capture have separate owners. | Compare VPN, sleep, adapter changes and driver availability. Keep monitoring results separate from transfer and capture behavior. |

[IDM Scheduler](https://www.internetdownloadmanager.com/support/schedulerD.html) · [IDM download features](https://www.internetdownloadmanager.com/features2.html) · [5.25 isolated qualification](validation/p4-p5-p6-5.25.md)

## Run protocol and acceptance gates

1. Freeze the versions and hashes, then record OS build, DPI, scaling, browser build, profile type, content route, machine load and storage. Capture the license state without storing license data.
2. Run fresh and migrated profiles separately. Warm up each app once, discard that timing, then alternate reference/candidate order. Collect at least five pairs per cold and warm interactive scenario. Extend high-variance cases to ten or more. Publish individual observations, median and spread. Use a tail percentile only when sample size supports it.
3. Record a short paired video when the licensed reference is available. Use one monitor and the same layout. Capture from the same input event to the first visible frame at a known frame rate. Mark dialog paint, foreground app, ad transition, and exact-size display separately. Pair video frames with trace timestamps. Mask unrelated browser tabs and existing downloads.
4. Check direct-file and media integrity before discussing speed. Require exact selected payload or decoded stream identity and a full decode, with final SHA-256 for controlled fixtures. Every size labeled exact must match final bytes. Record any unsupported or unknown outcomes explicitly.
5. Compare healthy throughput, recovery time and extra bytes separately. For ENG-01 first target at least 50% fewer refusals than a repeated 5.25 baseline on the same two-connection case. Keep the paired median wall time within 5% on the healthy and capped cases, within measured variance, and preserve exact bytes. If these trial thresholds conflict, collect more pairs and review the admission policy before changing defaults.
6. Use provisional UI guardrails until the licensed reference is measured. For a warm ready-format selection on the same device, target a first actionable File Info paint within 500 ms in local fixtures. Track player detection, native dispatch and paint separately. No duplicate jobs, wrong media, clipped primary controls or accidental main-list activation qualify for a latency waiver.
7. A detail closes when its screenshots or interaction trace and fixture show the disposition. Intentional differences get a reason and user-visible effect. A pending reference remains open even if the candidate behaves correctly. Publish no blanket parity percentage.

### What to do next

| Order | Work | Depends on | Done when |
| --- | --- | --- | --- |
| 1 | DEP-01 and REF-01 | Verified local setup and an authorized IDM session | Installed bytes and running versions verified. The two managers each finish the same owned reference file. |
| 2 | OBS-01 and the four highest-impact video pairs | DEP-01 for installed evidence. REF-01 for paired claims. | Trace covers first eligible video, menu ready, selection, File Info, first byte, exact size, Start and publish. |
| 3 | ENG-01 capped-origin experiment | Frozen 5.25 baseline. Can start without IDM. | Refusals fall without a regression in file integrity, healthy throughput or recovery. |
| 4 | VID-01, VID-02 and CAP-01 | OBS-01 and at least one reproduced gap | One measured bottleneck removed at a time. Stable choice, correct foreground window and truthful size survive. |
| 5 | MED-01 and PRO-01 | Reliable selection and handoff | Verified final size and full playback across the selected quality/container matrix. Prefetch is reused. |
| 6 | GUI-01, QUE-01, DATA-01 and SITE-01 | Baseline and independent fixture catalogs | Each workflow closes its matching UI, persistence and recovery rows. |
| 7 | WIN-01 and REL-01 | Qualified candidate and disposable Windows profiles | Registration, repair, rollback, publisher signing and preservation evidence complete. |

Advance desktop and both browser versions for each separately delivered functional improvement. Documentation and qualification carry the actual candidate number. Keep source, build, install, browser activation and publication records separate. [Version policy](versioning.md) · [Release ledger](next-major-release.md)
