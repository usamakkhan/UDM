# Current parity completion audit

Historical snapshot: the candidate and live-DASH limitations below predate later implementation batches on the same day. Current candidate records are in [the production bundle](production-bundle-20261027.md). Live multi-Period selection, capture, recovery and joined publication now exist in source; native SegmentList/SegmentBase and other limits are recorded in [the mixed-addressing review](browser-live-dash-mixed-addressing-20261028.md). This update does not establish runtime parity or supersede the deferred acceptance requirements below.

The objective remains full IDM behavior parity with UDM branding. Completion is not established. This audit checks the current candidate and selected remaining source boundaries; it is not an exhaustive execution or a parity percentage.

Current increment: 1.15.2 preserves [selected caption metadata in final recordings](live-dash-caption-output-metadata-20261104.md). All five native targets built successfully; runtime acceptance remains unverified.

Prior increment: 1.15.1 validates [live-DASH track metadata](live-dash-track-metadata-20261104.md). All five native targets built successfully; runtime acceptance remains unverified.

Prior increment: 1.15.0 / browser 0.63.85 connects [live DASH caption browser selection](browser-live-dash-captions-20261104.md). Browser syntax and all five native targets passed; end-to-end runtime acceptance remains unverified.

Prior increment: 1.14.0 adds [native live-DASH caption admission and recovery](live-dash-caption-admission-20261104.md). All five native targets built successfully; browser integration and runtime acceptance remain unfinished.

Prior increment: 1.13.1 adds [native recording caption muxing](dash-recording-caption-mux-20261104.md). All five native targets built successfully; live capture admission and runtime acceptance remain unfinished.

Prior increment: 1.13.0 adds [retained DASH caption assembly](dash-retained-caption-assembly-20261104.md). All five native targets built successfully; end-to-end live caption integration and runtime acceptance remain unfinished.

Prior increment: 1.12.3 adds [DASH track-kind preflight](dash-kind-preflight-20261104.md). All five native targets built successfully; live caption integration and runtime acceptance remain unfinished.

Prior increment: 1.12.2 extracts the [shared retained-file MP4 WebVTT decoder](mp4-caption-retained-decoder-20261104.md). All five native targets built successfully; live caption integration and runtime acceptance remain unfinished.

Prior increment: 1.12.1 improves [completed-file action state during relocation](move-control-state-20261104.md). All five native targets built successfully; runtime acceptance remains deferred.

Prior increment: 1.12.0 adds [move speed and timing](move-speed-timing-20261104.md). All five native targets built successfully; runtime and visual acceptance remain deferred.

Prior increment: 1.11.3 adds [move recovery before selected file actions](move-action-recovery-20261104.md). All five native targets built successfully; runtime acceptance remains deferred.

Prior increment: 1.11.2 separates [relocation from transfer scheduling](move-scheduling-20261104.md). All five native targets built successfully; runtime validation remains deferred.

Prior increment: 1.11.1 adds [verification and rollback byte progress](move-verification-progress-20261103.md). All five native targets built successfully; runtime and visual acceptance remain deferred.

Prior increment: 1.11.0 connects the [background relocation dialog](move-progress-ui-20261103.md). All five native production targets built successfully; runtime and visual acceptance remain deferred.

Prior increment: 1.10.0 adds the [relocation progress/cancellation backend](move-progress-backend-20261101.md). All five native production targets built successfully; runtime acceptance remains deferred. Desktop progress UI and asynchronous locking remain unfinished.

Prior increment: 1.9.2 adds [same-volume file-identity verification](move-identity-20261101.md), avoiding full content reads for supported renames. All five native production targets built successfully; runtime acceptance remains deferred. Cross-volume/background relocation remains unfinished.

Prior increment: 1.9.1 adds [current-file verification for Move/Rename](move-current-file-20261101.md). All five native production targets built successfully; runtime acceptance remains deferred. Large-file relocation still runs synchronously and needs responsiveness work.

Prior increment: 1.9.0 adds [case-only filename changes and recovery](filename-case-20261101.md). All five native production binaries built successfully; runtime acceptance remains deferred.

Prior increment: 1.8.0 / browser 0.63.84 adds complete-media support for [per-segment index templates](dash-segment-index-metadata-20261101.md). All five native production targets built successfully; runtime acceptance remains deferred.

## October 29 source update

Later source/build milestones now include live compatible Period transitions and recovery, SegmentList selection/capture, bounded SegmentBase/SIDX resolution, version-bound shared resources, inherited template initialization and recorded final-segment bounds. Finalized DASH completion and remaining-media pacing are documented in [the current production bundle](production-bundle-20261027.md). The artifact and evidence tables below remain the October 28 snapshot rather than a description of the newest candidate.

The source still rejects incomplete low-latency DASH chunk availability, per-segment index-based subsegment selection, list index-based subsegment selection and unsupported boundary forms. [Full SegmentList media transfer with optional index metadata](dash-list-index-metadata-20261029.md) is now implemented; it does not parse those indexes. [Recorded finite-Period representation-index templates](dash-recorded-template-index-20261029.md) now use the existing SIDX resolver. [Native template-index resolution](dash-native-template-index-20261029.md) and [live browser catalog integration](dash-live-template-index-20261029.md) are now present in source; runtime acceptance remains deferred. Recorded finite-Period templates and native live duration templates now have bounded eptDelta support (see [recorded offsets](dash-recorded-ept-20261029.md) and [native offsets](dash-native-ept-20261029.md)); Negative pdDelta (ending gaps), positive initial gaps and negative media origins remain unsupported; non-negative pdDelta declarations now have native/browser support with existing Period clipping (docs/dash-duration-overlap-20261101.md). Recorded MP4 WebVTT captions now have browser discovery, native decoding and assembly in source; [Bounded normal-rate edit sequences](mp4-webvtt-edit-sequence-20261029.md), including empty and repeated intervals, are now implemented in source; [Whole-number playback edits from 1x through 16x](mp4-caption-integer-rate-20261030.md) are now implemented in source. Finite dwell edits are now implemented in source; see docs/mp4-caption-dwell-20261030.md. Fractional forward edits are now implemented in source; see docs/mp4-caption-fractional-rate-20261030.md. Finite reverse edits are now implemented in source; see docs/mp4-caption-reverse-20261030.md. Plain WebVTT cue-text timestamp mapping is now in source (docs/webvtt-text-timing-20261031.md). Runtime acceptance, MP4 embedded cue timestamp remapping and broader caption formats remain unfinished. Incompatible codec transitions remain unfinished. Signing, installed delivery, ARM64 and the wider comparison/acceptance matrix are also not established. Compilation and syntax checks do not remove these requirements; runtime testing remains deferred under the current instruction.

## Native development state (2026-10-30)

Native 1.7.0 and browser 0.63.83 add non-negative DASH duration-overlap support; browser syntax checks and all five production targets passed. Native 1.6.8 distinguishes catalog duplicates by transfer context and recognizes import-numbered copies; all five production targets compiled successfully. Native 1.6.7 preserves form/media recapture context on completed catalog imports and checks it before Download Again; all five production targets compiled successfully. Native 1.6.6 commits selected-download recovery before Site Grabber starts a batch and preserves journals during deferred saves; all five production targets compiled successfully. Native 1.6.5 adds transactional standalone offline archive admission while preserving enclosing project transactions; all five production targets compiled successfully. Native 1.6.4 makes Download Again creation and inherited-property persistence a single catalog transaction; all five production targets compiled successfully. Native 1.6.3 adds recovery before Move/Rename and transactional move-history recovery; all five production targets compiled successfully. Native 1.6.2 compiled successfully across all five production targets with scoped recovery before catalog removal and recycling; the prior 1.6.1 build passed all five production targets with plain WebVTT cue-text timestamp mapping; the prior 1.6.0 build passed all five production targets with reverse caption edits; the prior 1.5.0 build passed all five production targets with fractional edits; the prior 1.4.0 build passed all five production targets with finite dwell edits. Native 1.3.0 previously compiled successfully across all five production targets. The previous 1.2.15 build passed all five production targets and added rollback of both replacement-history records when saving recovery fails. Resume now limits publication/replacement recovery to the selected download and its tracks. Publication recovery now includes saved private audio/video child tracks, without changing replacement-history lookup. Version 1.2.11 extends pending-recovery protection to direct, recorded-media, live DASH and offline output; 1.2.12 fixes the private audio/video track publication regression introduced by the uninstalled 1.2.9 candidate. Tests remain deferred. No installer was generated, following the latest instruction; installed UDM is 1.6.0 following the 2026-10-31 direct update, and the last historical setup is 1.2.9. See [native build evidence](validation/dash-duration-overlap-build-20261101.json) and [next major release notes](next-major-release.md).

## Current artifact state (2026-10-29)

The current recorded candidate is UDM 1.2.9 with browser 0.63.82, from setup/publication-responsive-20261030/. All five native production targets compiled and installer packaging passed. The bundle manifest records 147 payload hashes. See [current build evidence](validation/publication-responsive-build-20261030.json).

The 1.2.9 candidate is unsigned, uninstalled and unpublished. Installed UDM remains 1.2.8. Setup completed with exit 0; all 146 deployed payload files, installed versions, merged Explorer registration and browser-host registrations were verified. Runtime/UI acceptance remains deferred. See [installation receipt](validation/install-1.2.8-20261030.json).

Compilation does not prove runtime parity, real transfers, GUI fidelity, timing, recovery or domain authentication. Runtime tests remain deferred under the user's instruction.

A fresh [live DASH caption integration audit](live-dash-caption-integration-audit-20261104.md) traces the remaining browser/native/assembly boundaries. Live subtitles require MP4 WebVTT selection, identity, decoding and output mapping; exposing a browser option alone cannot complete this feature.

## Remaining work, separated by evidence

| Area | Current source/build evidence | What remains |
|---|---|---|
| Delivery | Current x64 native builds compile; historical setup is 1.2.9 | 1.6.0 direct update completed with four executable hashes and Windows version verified; broader installed qualification remains |
| Proxy authentication | GET/HEAD, rejected-CONNECT HTTPS forms and explicit method selection | Domain/mixed-method/browser-routing and NTLM acceptance |
| Speed limits and catalog responsiveness | Combined pacing, catalog serialization and completion checkpoint improvements built | Concurrent throughput, cancellation and large-catalog measurements |
| Import/export | Request details, sorting, metadata, selection and request-integrity safeguards built | Installed workflows and actual IDM export round trips |
| Recorded DASH | Compatible Period assembly, bounded timing, template/list/representation-index addressing and WVTT captions built | Low-latency incomplete chunks, unsupported boundary forms, index-based list subsegments and incompatible codec transitions |
| Live DASH | Browser selection and native template/list/SegmentBase capture, compatible Period transitions, versioned resources, protected recovery and finalized drain built | Growing indexed resources, incomplete chunks, live captions, incompatible transitions and runtime acceptance |
| MP4 WebVTT | Fragment parsing, assembly, bounded forward/fractional/dwell/reverse edits and empty-interval mapping in source | Embedded cue timestamp remapping, other subtitle formats and decoder/playback acceptance |
| Scheduler | Role-based built-in type protection, synchronization local-size check, missing-file recovery, enrollment and retry admission built | 1.2.6 source adds cancellable SHA-256 verification for records with a saved digest; legacy unhashed records and installed scheduling, retry and replacement acceptance remain |
| Signing/updater | Signing pipeline and signed-manifest updater source exist | Public publisher identity, signed delivery and installed upgrade/rollback qualification |
| ARM64 | Build currently targets x64 | Architecture-specific dependencies, build and execution; driver availability is separate |
| Driver, physical failures, accessibility | Earlier narrower records exist | Current VPN/HVCI, sleep/power, disk/device faults, mixed DPI and screen-reader matrix |
| Localization and legacy protocols | English-only; MMS and old IE-module integration excluded by standing scope decisions | Deliberate differences must remain visible; they are not identical behavior |

Encrypted/DRM media remains rejected. That UDM limit does not establish what the reference supports; no bypass work is implied.

## Desktop reference recheck (2026-10-29)

[IDM's main-window documentation](https://help.internetdownloadmanager.com/support/using_idm/using_idm.html) describes automatic category assignment by file type/site, per-category destinations, editable categories, list sorting and customizable toolbar/columns. The review found existing UDM implementations rather than a demonstrated missing feature:

| Reference workflow | Current source inspected | Evidence limit |
|---|---|---|
| File type and site category routing | `OptionsModel.hpp::downloadCategory`, `categoryForPreferences`, `siteRestrictedCategory` | Matching semantics and redirect/media-origin routing still need comparisons |
| Category folders and destination memory | `categoryFolder`, `rememberCategoryDestination`, `CategoryEditor.hpp` | UI and filesystem behavior not executed |
| Toolbar button layout and skins | `ToolbarUi.hpp::currentToolbarLayout`, `saveToolbarButtons`, `loadToolbarImages` | Source presence, not pixel/interaction parity |
| Categories visibility, find and columns | `App.cpp` command/menu handlers | No installed GUI acceptance performed |

No application source was changed during this recheck. The latest successful build therefore remains the candidate; rebuilding identical sources would add no coverage. Next work should target an evidenced implementation boundary in the table, or a concrete defect found while tracing one of these workflows, rather than recreate existing controls.

## Corrections to the backlog

The proxy decision document is updated to include the explicit authentication selector and HTTPS form path. The active program no longer points to browser 0.63.50 as the current stage. Its original merge/build order is labeled historical rather than presented as work still to repeat.

The October 28 artifact audit remains historical. The current inventory above supersedes its source-gap descriptions. Deferred runtime acceptance remains necessary before claiming full parity.
