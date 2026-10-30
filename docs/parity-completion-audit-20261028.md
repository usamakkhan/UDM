# Current parity completion audit

Historical snapshot: the candidate and live-DASH limitations below predate later implementation batches on the same day. Current candidate records are in [the production bundle](production-bundle-20261027.md). Live multi-Period selection, capture, recovery and joined publication now exist in source; native SegmentList/SegmentBase and other limits are recorded in [the mixed-addressing review](browser-live-dash-mixed-addressing-20261028.md). This update does not establish runtime parity or supersede the deferred acceptance requirements below.

The objective remains full IDM behavior parity with UDM branding. Completion is not established. This audit checks the current candidate and selected remaining source boundaries; it is not an exhaustive execution or a parity percentage.

## October 29 source update

Later source/build milestones now include live compatible Period transitions and recovery, SegmentList selection/capture, bounded SegmentBase/SIDX resolution, version-bound shared resources, inherited template initialization and recorded final-segment bounds. Finalized DASH completion and remaining-media pacing are documented in [the current production bundle](production-bundle-20261027.md). The artifact and evidence tables below remain the October 28 snapshot rather than a description of the newest candidate.

The source still rejects incomplete low-latency DASH chunk availability, per-segment template indexes, list index-based subsegment selection and unsupported boundary forms. [Full SegmentList media transfer with optional index metadata](dash-list-index-metadata-20261029.md) is now implemented; it does not parse those indexes. [Recorded finite-Period representation-index templates](dash-recorded-template-index-20261029.md) now use the existing SIDX resolver. [Native template-index resolution](dash-native-template-index-20261029.md) and [live browser catalog integration](dash-live-template-index-20261029.md) are now present in source; runtime acceptance remains deferred. Recorded finite-Period templates and native live duration templates now have bounded eptDelta support (see [recorded offsets](dash-recorded-ept-20261029.md) and [native offsets](dash-native-ept-20261029.md)); pdDelta, positive initial gaps and negative media origins remain unsupported. Recorded MP4 WebVTT captions now have browser discovery, native decoding and assembly in source; [Bounded normal-rate edit sequences](mp4-webvtt-edit-sequence-20261029.md), including empty and repeated intervals, are now implemented in source; runtime acceptance, rate-changing edits and broader caption formats remain unfinished. Incompatible codec transitions remain unfinished. Signing, installed delivery, ARM64 and the wider comparison/acceptance matrix are also not established. Compilation and syntax checks do not remove these requirements; runtime testing remains deferred under the current instruction.

## Current artifact state (2026-10-29)

The current recorded candidate is UDM 1.1.0 with browser 0.63.82, from `setup/mp4-webvtt-edit-sequence-20261029/`. Native production compilation and installer packaging passed. The installer is 63,292,238 bytes with SHA-256 `2200771B52A133CE63176FB7AA60979CCF3BBF3C807DEB068FC767009F0B7A9B`. The bundle manifest records 147 payload hashes. These values were read from the current build evidence; this documentation review did not repeat the file-hash pass.

The candidate is unsigned, uninstalled and unpublished. No current installed behavior was inspected in this review. The earlier 1.0.4/browser 0.63.56 comparison is historical evidence in `validation/parity-completion-audit-20261028.json`, not the current delivery state. The latest artifact and source hashes are in [the production bundle manifest](validation/production-bundle-20261027.json) and [caption edit build record](validation/mp4-webvtt-edit-sequence-build-20261029.json).

Compilation does not prove runtime parity, real transfers, GUI fidelity, timing, recovery or domain authentication. Runtime tests remain deferred under the user's instruction.

## Remaining work, separated by evidence

| Area | Current source/build evidence | What remains |
|---|---|---|
| Delivery | Current x64 candidate compiles and packages | Install and qualify when that phase resumes; no current installation claim |
| Proxy authentication | GET/HEAD, rejected-CONNECT HTTPS forms and explicit method selection | Domain/mixed-method/browser-routing and NTLM acceptance |
| Speed limits and catalog responsiveness | Combined pacing, catalog serialization and completion checkpoint improvements built | Concurrent throughput, cancellation and large-catalog measurements |
| Import/export | Request details, sorting, metadata, selection and request-integrity safeguards built | Installed workflows and actual IDM export round trips |
| Recorded DASH | Compatible Period assembly, bounded timing, template/list/representation-index addressing and WVTT captions built | Low-latency incomplete chunks, unsupported boundary forms, index-based list subsegments and incompatible codec transitions |
| Live DASH | Browser selection and native template/list/SegmentBase capture, compatible Period transitions, versioned resources, protected recovery and finalized drain built | Growing indexed resources, incomplete chunks, live captions, incompatible transitions and runtime acceptance |
| MP4 WebVTT | Fragment parsing, assembly and bounded multi-edit/empty-interval mapping built | Rate-changing edits, other subtitle formats and decoder/playback acceptance |
| Scheduler | Role-based built-in type protection and synchronization local-size check built | Same-length local changes are not detected by the size check; installed scheduling, retry and replacement acceptance |
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
