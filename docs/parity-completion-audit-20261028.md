# Current UDM parity evidence

Reconciled 2026-11-04. The objective remains IDM behavior and interface parity with UDM branding. Completion is unproven. Historical milestone notes are preserved in [the previous audit](parity-audit-history-through-20261104.md); their statements about what was unfinished apply to their own versions.

## Current state

- Native development version: 1.36.0. Production build in progress; see [the latest build receipt](validation/idm-find-cursor-build-20261104.json). Runtime tests remain deferred.
- Extension source: Chromium and Firefox 0.63.85. Both manifest versions match; the latest browser changes have syntax evidence in [the browser integration receipt](validation/browser-live-dash-captions-build-20261104.json).
- Installed native executable: 1.6.0, directly checked in Program Files on 2026-11-04. The prior installed-extension record is 0.63.82; activation was not checked in this audit.
- Git checkout: content-recognition-phase-a, HEAD d321da5 at the 1.36.0 source snapshot, with uncommitted development changes. A successful local build does not establish a pushed commit or release.
- Historical installer: 1.2.9. Current installers, installation, publication and runtime tests remain deferred under the standing instruction.

## Implementation and verification matrix

| Requirement | Current authoritative source/build evidence | Remaining work |
|---|---|---|
| Desktop list, categories, toolbar and columns | App.cpp, OptionsModel.hpp, CategoryEditor.hpp and ToolbarUi.hpp; current native build | Installed interaction, pixel/DPI/accessibility and reference comparisons |
| Browser handoff and prefetch | Browser/native source and earlier focused records exist | Current installed browser-to-file acceptance, timing and focus behavior |
| File moves | Background worker/dialog, destination reservation, cancellation, verification, rollback, scheduler integration and recovery exist in source | Concurrency, copy/delete failures, aliases, shutdown, interruption and visual acceptance |
| Recorded DASH | Period assembly, bounded template/list/index addressing, MP4 WebVTT and edit-list decoding | Unsupported boundaries, low-latency chunks, index-based subsegment selection and incompatible codec transitions |
| Live DASH captions | Browser discovery/selection, native three-track admission, protected session identity, retained decoding, session-wide caption muxing across silent Periods and explicit empty-caption results are connected through 1.19.0 | Real recording, silent-caption Periods, continuation, timing, joining and playback remain unverified |
| General live DASH | Template/list/SegmentBase capture, version-bound resources and compatible Period joining exist | Growing indexed resources, incomplete chunks, unsupported boundaries and incompatible transitions |
| Subtitle formats | Plain WebVTT and bounded MP4 WebVTT paths, including forward embedded timestamp mapping, exist | Reverse/dwell timed payloads, TTML/other formats and playback acceptance |
| Download scheduling | Retry, synchronization, membership, wake and completion-action source exists | Installed schedule windows, domain/credential cases, replacement and failure acceptance |
| Transfer performance | Earlier benchmarks, pacing work, immutable list snapshots, background Shell icon loading and unchanged-order reuse exist | Current paired measurements against IDM, cancellation, large catalogs and physical storage faults |
| Proxy/authentication | HTTP method/authentication selection, browser routes and FTP gateways exist | Domain/mixed-method/NTLM and installed browser-routing acceptance |
| Catalog import/export | Context-aware duplicate handling, credential/recapture preservation and transaction corrections exist | Actual IDM round trips and installed import/export acceptance |
| Signed delivery and updates | Signing and updater source exists | Publisher identity, signed release and installed upgrade/rollback qualification |
| ARM64 | Current production build is x64 | Architecture-specific dependencies, builds and runtime qualification |
| Localization/legacy integration | English-only, MMS and old IE-module differences were previously recorded as deliberate scope choices | Keep these differences explicit; literal identical behavior cannot be inferred |

## Concrete source boundaries still open

- DashManifest.cpp::planDashAvailableWindow rejects incomplete low-latency chunk availability. Supporting this requires streaming transfer and recoverable chunk handling, rather than simply accepting the manifest flag.
- Shared indexed resources require fixed size/validator bindings. Growing resources need an append-aware identity/checkpoint contract; dropping version checks would invalidate current retained-byte guarantees.
- SegmentList and per-segment index metadata currently allow full-media transfer, not index-based subsegment selection.
- Positive initial gaps, negative duration-boundary gaps, negative media origins and incompatible Period codec transitions remain separate implementation limits.
- MP4 WebVTT forward embedded cue-text mapping is implemented through 1.16.0. Reverse/dwell timed payloads and actual rendering remain open.

The live-caption [integration audit](live-dash-caption-integration-audit-20261104.md) is now a historical implementation breakdown: its discovery/admission/decoder/muxing requirements have source implementations, while its acceptance cases remain outstanding. Do not continue presenting all of those requirements as absent features.

## Evidence limits and next work

This reconciliation inspected current sources, manifest versions, build hashes and the installed executable version. It did not run runtime tests or operate the installed GUI. Existing green builds prove compilation only; they do not prove full workflow or IDM parity.

The next implementation work should address a remaining source, interaction or performance boundary identified above. Current runtime qualification also remains mandatory before claiming completion, but is deferred by the user's instruction. The goal remains active and incomplete.

Cue-current-time metadata is preserved in the MP4 decoder ([1.15.3 notes](mp4-caption-current-time-20261104.md)); forward timestamp mapping followed in 1.16.0.

Forward MP4 cue-text mapping is now implemented ([1.16.0 notes](mp4-caption-inline-timing-20261104.md)); reverse/dwell timed payloads and runtime rendering acceptance remain unfinished.

Caption assembly reuse now carries a caption-processing revision for both Period and joined-session outputs (1.16.1). Older caption output is rebuilt from retained verified fragments; audio/video-only reuse retains its original binding. Runtime recovery acceptance remains deferred. See [implementation record](dash-caption-assembly-recovery-20261104.md).

Direct-track YouTube assembly now compares selected source/output track codecs, dimensions, audio format and available per-track durations before confirmation/publication (1.17.0). This is a mux consistency guard; complete playback and packet-level acceptance remain deferred. See [implementation record](media-assembly-verification-20261104.md).

Tray queue controls now expose named Start/Stop submenus, with disabled inactive Stop entries and an editable chooser beyond the bounded popup command range (1.18.0). Current main-menu dynamic queue population was already present; its old GUI audit description was corrected. Installed/visual acceptance remains deferred. See [implementation record](tray-queue-controls-20261104.md).

Live-DASH caption assembly now builds one recording-relative cue timeline and muxes captions after audio/video Period joining (1.19.0). Silent Periods no longer require a subtitle stream in intermediate files. Entirely silent captures retain an explicit zero-cue result without fabricated captions. Caption configuration/coverage, output-receipt binding and media stream checks remain enforced; live timing, continuation, cancellation and decoded playback are still unverified. See [implementation record](live-dash-silent-captions-20261104.md).

Download-list refresh now snapshots only its display inputs under the manager lock, reuses unchanged immutable projections, and sorts/decodes headers/looks up shell icons/paints after releasing that lock (1.20.0). Selection and focus retain ID-based restoration. Current installed version remains 1.6.0 by direct read. Measured responsiveness and large-catalog/active-transfer/browser-handoff acceptance remain deferred. See [implementation record](download-list-snapshot-refresh-20261104.md).

File-type icon lookup now runs on a dedicated COM-initialized background worker with bounded current-generation requests, viewport loading and generation-based invalidation (1.21.0). UI/image-list updates remain on the window thread, using immediate category fallbacks. Loader shutdown does not join a blocked Shell call and keeps worker-owned state alive. Measured startup/interaction gains, Shell failure cases, DPI transitions and shutdown behavior remain unverified. See [implementation record](background-file-icons-20261104.md).

Download-list sorting now reuses the last order only when the column/direction, candidate sequence and comparator keys match (1.22.0). Candidate/catalog reordering and job replacement invalidate reuse to preserve stable ties. Row rendering and asynchronous icons continue independently. Large-catalog timing and live sort/filter/selection acceptance remain deferred. See [implementation record](download-list-order-reuse-20261104.md).

Main File Name sorting now calls Windows StrCmpLogicalW (1.23.0), traced from the installed IDM specimen whose hash matches the retained analysis. Raw-name order validation avoids UTF-16 conversion on cache hits, and Windows settings broadcasts invalidate cached order. Other columns retain their prior comparators. Actual Unicode/number/case/locale and equal-key tie comparisons remain unverified. See [implementation and static reference evidence](idm-logical-filename-order-20261104.md).

Version 1.24.0 adds the statically confirmed empty-last ascending rule for optional text, plus bounded UTF-16 Save Path and Parent Web Page comparison keys. Full stored/displayed metadata is retained. See [implementation and limits](idm-optional-text-order-20261104.md). Runtime and locale parity remain unverified.

Version 1.25.0 follows the traced CStringA/multibyte comparator path for Description and Referer, using temporary system-ANSI keys while retaining full Unicode data and raw cache keys. CRT comparison errors restart the whole sort under a consistent byte comparator. Exact runtime locale/code-page parity and interaction remain unverified. See [implementation and limits](idm-multibyte-text-order-20261104.md).

Version 1.26.0 implements reference-derived Status completion groups, unrounded progress ordering across unfinished states and zero-progress error-label ties. HTTP 404 failures now display Not Found and their result participates in immutable row snapshots. Byte-ratio/media-percent score scaling follows the traced reference callback; exact media-clock, mixed-transport, floating-point and live behavior remain unverified. See [implementation and limits](idm-status-order-20261104.md).

Status display through 1.27.0 now retains valid progress on failed rows, uses source coverage instead of rounded permille, and follows the reference float/truncated-hundredths path with a cached Windows decimal separator. Locale broadcasts invalidate the separator and row paint cache. Unknown/live mappings, UDM-specific assembly/quota phases, Unicode separators, stale-cell behavior and floating-point boundaries still require qualification. See [implementation and limits](idm-status-display-20261104.md).

Version 1.28.0 removes the artificial one-day media ETA cap and guards all ETA integer conversions. Main/Scheduler list cells use integer rates above 10 B/s; the Time Left comparator retains lower-rate numeric estimates, zero unavailable keys and completed-first ascending ties. The reference 10,000-hour display bound is retained separately from calculation. Runtime clock/rate, paused-transfer, overflow, ordering and visual parity remain unverified. See [implementation and limits](idm-time-left-20261104.md).

Version 1.29.0 adds the reference Size comparator's duration ordering within zero/unknown-byte-size media, plus guarded whole-byte Transfer Rate keys shared by list rendering and ETA. Cache validation follows those actual comparator keys. Exact reference duration/object-rate mappings, secondary rate fallback, malformed/legacy Last Try Date behavior and all runtime/visual comparisons remain unverified. See [implementation and limits](idm-size-rate-order-20261104.md).

Version 1.30.0 fixes forward inline MP4 caption offsets that were truncated to whole media ticks before edit-rate scaling. Rational delta seconds/remainders now combine with checked sample/edit, movie and presentation clocks before final millisecond rounding; caption assembly revision 4 invalidates older caption output reuse. Reverse/dwell timed payloads still need cue-tree temporal-state preservation and remain unsupported. No parser/player/runtime or IDM comparison was performed. See [implementation and limits](mp4-caption-subtick-clock-20261104.md).

Version 1.31.0 limits navigation building's own manager-lock scope to a lightweight projection, moves structural comparison/serialization/control messages afterward, and updates queue schedule/type icons in place. Structural rebuilds restore expansion, selection/ancestor fallback and a vertical item anchor by stable keys. Actual latency, concurrent mutation, UI/DPI/keyboard behavior and IDM comparison remain unverified; other UI lock scopes are not claimed fixed. See [implementation and limits](navigation-tree-snapshot-20261104.md).

Version 1.32.0 batches toolbar/context availability from one UI selection and shared global catalog pass, removes the context menu's outer manager lock, and resolves detached file/FTP checks before control updates. Ordinary command updates skip unused preferences/selection reads. Synchronous UI-thread file probes, legacy refresh-address predicate locking, operation-time races and runtime responsiveness/IDM comparisons remain unqualified. See [implementation and limits](command-availability-snapshot-20261104.md).

Version 1.33.0 captures queue Stop eligibility in a shared catalog pass, uses that snapshot for queue menu creation and MFC child-item updates, and refreshes dynamic submenus on opening. Queue parents no longer inherit their first child's unavailable Stop state. Fresh execution checks remain; runtime scalability, nested menu interaction and IDM comparison are unverified. See [implementation and limits](queue-menu-snapshot-20261104.md).

Version 1.34.0 follows the retained IDM Find address helper's full-string-or-literal-site whole-match rule and Windows uppercase folding. Find Next prepares one query and resolves per-row owned search metadata outside the manager lock. Current selection-based cursor behavior remains; exact reference cursor-reset rules, malformed legacy addresses, Unicode/runtime and UI performance comparisons are unverified. See [implementation and static reference evidence](idm-find-site-20261104.md).

Version 1.35.0 saves accepted Find text/options through a targeted Manager update, restores them at main-window creation, and normalizes malformed saved fields during catalog loading. Cancel and save exceptions preserve the previous active query; storage/restart acceptance, exact reference cursor/reset behavior and GUI comparison remain unverified. See [implementation and static reference evidence](idm-find-state-20261104.md).

Version 1.36.0 introduces a separate next-row Find cursor, reference-style wrap/full-pass failure handling, query/scope reset, selection restoration after list changes and partial-visibility result scrolling. The earlier selection-based starting rule is superseded for stable views. Exact legacy callback/reset equivalence, quick-search coupling, concurrency and GUI/scroll comparisons remain unverified. See [implementation and static reference evidence](idm-find-cursor-20261104.md).
