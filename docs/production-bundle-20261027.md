# Complete candidate build

Recent desktop-only candidates did not rebuild all native companions. This
delivery pass builds the desktop, native browser host, monitor, setup helper and
COM automation server together, plus the network companion, and stages both
browser packages from the same source snapshot.

`native/build.ps1 -ProductionOnly` selects all shipping native targets without
building or executing test targets. The root build forwards that option and
copies all native dependency notices to its release directory. Other target
selectors and `-Test` cannot be combined with `-ProductionOnly`.

The installer builder and package wrapper now accept `-ReleaseDirectory`,
`-BrowserDirectory` and `-OutputDirectory`. Defaults retain the existing release
paths. Custom paths are also used by signing and manifest-version checks. The
automation executable now participates in the matching-version gate alongside
the other native companions. The MIME-data license is included in setup.

This candidate uses an isolated, allowlisted staging directory; local catalogs,
test executables, debug symbols and evidence files from the normal release
directory are not copied. Only the required media tools, vendor network runtime,
notices, source archive, assets and shipping binaries enter the candidate.

Packaging retains its existing read-only network-runtime integrity/protocol
check. Runtime behavior, download tests, installer execution and live IDM
comparison remain deferred. No setup is installed or published by this build.
The version remains 1.0.4 with extension 0.63.56; hashes identify this candidate.
It is unsigned and is not a qualified public release.

Build, package and file-hash results are recorded in
[the candidate record](validation/production-bundle-20261027.json).

The 2026-10-28 refresh includes sortable native import folder-override and
description columns. All five native executables and setup were rebuilt; the
candidate record contains the refreshed hashes. Runtime testing remains deferred.

A subsequent 2026-10-28 rebuild adds repeated wildcard replacement in batch URLs,
with numeric and letter paths using the same sequence token at every asterisk.
The candidate record identifies this latest setup; older build records are historical.

The latest 2026-10-28 rebuild also applies metadata-driven HTML selection defaults
while preserving explicit user choices. Its hashes supersede earlier candidates.

The latest refresh includes the native import single-highlight Edit rule. The
initial syntax error was corrected, all production targets rebuilt successfully,
and setup was repackaged. The current record contains the new payload hashes.

The latest refresh adds protected per-file login and Referer overrides to native
import properties, metadata lookup and download admission. Production compilation
and setup packaging passed; runtime authentication testing remains deferred.

The latest refresh displays live type, status and reported byte size in native
import File Properties, suppressing stale metadata during unsaved request edits.
Production compilation and setup packaging passed; runtime checks remain deferred.

The latest setup packages extension 0.63.51, allowing all 500 links retained by
context-menu collection to be submitted. Native binaries are unchanged from the
last successful production build. JavaScript syntax and packaging checks passed.

The latest setup packages extension 0.63.52 with acknowledged batch progress and
bounded submission receipts. Native binaries are unchanged; syntax, mirror,
manifest and setup checks passed. Runtime interruption checks remain deferred.

The latest setup packages extension 0.63.53 with a shared capability lookup within
each browser handoff. Native binaries are unchanged; syntax and packaging passed.
No runtime latency measurements were taken.

The latest setup packages extension 0.63.54 and rebuilt native companions with
browser import Referer support. Source-page context is retained separately, and
request changes suppress stale review metadata. Syntax and build checks passed.

The latest setup packages extension 0.63.55 with per-row explicit selection state
and displayed-order submission. Native binaries are unchanged from the Referer
support build; syntax and setup checks passed. Runtime interactions remain deferred.

## Proxy Negotiate download support (2026-10-28)

All five native production targets and the installer rebuilt successfully for the [Negotiate-only proxy path](proxy-negotiate-downloads-20261028.md). Browser 0.63.55 is unchanged. The new setup is kept in `setup/proxy-negotiate-20261028/` to preserve the prior candidate. See `validation/proxy-negotiate-build-20261028.json` for its hash. Runtime tests remain deferred; this unsigned candidate is not installed or published.

## Live speed-setting responsiveness (2026-10-28)

All five native targets and setup rebuilt successfully with [short limiter reservations](live-limiter-reservations-20261028.md). The candidate retains the proxy change and unchanged browser 0.63.55. Its installer is under `setup/live-limiter-20261028/`; `validation/live-limiter-build-20261028.json` records the hash. Earlier installers remain preserved. Runtime tests are deferred, and this unsigned candidate is not installed or published.

## Combined bandwidth limits (2026-10-28)

All five native targets and setup rebuilt successfully for [concurrent total/per-file reservations](combined-limiter-reservations-20261028.md). The candidate retains the previous live-limit and proxy changes. Its installer is under `setup/combined-limiter-20261028/`; see `validation/combined-limiter-build-20261028.json` for the hash. Browser 0.63.55 is unchanged. Runtime testing, installation and publication remain pending.

## Export request integrity (2026-10-28)

All five native targets and setup rebuilt successfully for [export request safeguards](export-request-integrity-20261028.md). This candidate retains both limiter changes and the proxy update. Its installer is under `setup/export-request-integrity-20261028/`; see `validation/export-request-integrity-build-20261028.json` for the hash. Browser 0.63.55 is unchanged. Runtime testing, installation and publication remain pending.

## HTTPS form proxy negotiation (2026-10-28)

All five native targets and setup rebuilt successfully for [rejected-CONNECT form support](proxy-post-connect-20261028.md). Earlier limiter and export changes are included. Setup is under `setup/proxy-post-connect-20261028/`; `validation/proxy-post-connect-build-20261028.json` records its hash. Browser 0.63.55 is unchanged. Runtime checks, installation and publication remain pending.

## Explicit proxy authentication choice (2026-10-28)

All five native targets and setup rebuilt successfully for [per-protocol authentication selection](proxy-authentication-choice-20261028.md). Prior proxy, limiter and export changes are included. Setup is under `setup/proxy-auth-choice-20261028/`; `validation/proxy-auth-choice-build-20261028.json` records its hash. Browser 0.63.55 is unchanged. Runtime checks, installation and publication remain pending.

## Recorded DASH duration (2026-10-28)

All five native targets and setup rebuilt successfully for [declared DASH duration](dash-declared-duration-20261028.md). Chromium and Firefox are 0.63.56. Setup is under `setup/dash-duration-20261028/`; `validation/dash-duration-build-20261028.json` records its hash. Earlier proxy, limiter and export changes are included. Runtime checks, installation and publication remain pending.

## DASH duration refresh compatibility (2026-10-28)

All five native targets and setup rebuilt successfully for [refresh compatibility and transactional duration updates](dash-refresh-duration-20261028.md). Setup is under `setup/dash-refresh-duration-20261028/`; `validation/dash-refresh-duration-build-20261028.json` records its hash. Extension 0.63.56 is unchanged. Runtime checks, installation and publication remain pending.

## Recorded audio-only DASH (2026-10-28)

Setup compiled successfully with extension 0.63.57 and the existing native 1.0.4 production payload. [Audio-only DASH](dash-audio-only-20261028.md) now uses the M4A workflow. The installer is under `setup/dash-audio-only-20261028/`; `validation/dash-audio-only-build-20261028.json` records its hash. Runtime checks, installation and publication remain pending.

## DASH Period parser (2026-10-28)

Setup compiled successfully with extension 0.63.58 and the existing native 1.0.4 production payload. [Period parsing groundwork](dash-period-parser-20261028.md) separates clocks and catalogs; multi-Period handoff remains gated pending native assembly. See `validation/dash-period-parser-build-20261028.json`. Runtime checks, installation and publication remain pending.

## DASH Period selection (2026-10-28)

Setup compiled successfully with extension 0.63.59 and the existing native 1.0.4 production payload. [Cross-Period track matching](dash-period-selection-20261028.md) constructs selections for native integration; multi-Period handoff remains gated. See `validation/dash-period-selection-build-20261028.json`. Runtime checks, installation and publication remain pending.

## Native DASH Period contract (2026-10-28)

All five production targets and setup compiled successfully with the [native Period validator and recovery identity helper](dash-period-native-20261028.md). Browser 0.63.59 is unchanged. Multi-Period admission remains explicitly disabled pending transfer and assembly integration. See `validation/dash-period-native-build-20261028.json`. Runtime checks, installation and publication remain pending.

## Adaptive recovery selection binding (2026-10-28)

All five production targets and setup compiled successfully for [journal selection binding](adaptive-selection-journal-20261028.md). Browser 0.63.59 is unchanged. Multi-Period transfer and assembly remain disabled and unfinished. See `validation/adaptive-selection-journal-build-20261028.json`. Runtime checks, installation and publication remain pending.

## Period-aware adaptive transfer layout (2026-10-28)

All five production targets and setup compiled successfully for [the shared transfer layout](adaptive-period-layout-20261028.md). Browser 0.63.59 is unchanged. Multi-Period assembly and handoff remain unfinished. See `validation/adaptive-period-layout-build-20261028.json`. Runtime checks, installation and publication remain pending.

## Native DASH Period assembly (2026-10-28)

All five production targets and setup compiled successfully for [Period assembly and joining](dash-period-assembly-20261028.md). Browser 0.63.59 is unchanged. Nested capture/refresh and browser handoff remain unfinished; admission is still gated. See `validation/dash-period-assembly-build-20261028.json`. Runtime checks, installation and publication remain pending.

## Recorded multi-Period DASH handoff (2026-10-28)

All five native targets and setup compiled successfully with extension 0.63.60. [The recorded Period path](dash-period-handoff-20261028.md) now connects browser selection/index expansion, scoped credentials, native admission, refresh, recovery, assembly and publication. Live DASH and incompatible transitions remain unfinished. See `validation/dash-period-handoff-build-20261028.json`. Runtime checks, installation and publication remain pending.

## Stored DASH catalog matching (2026-10-28)

Setup compiled successfully with extension 0.63.61 and the existing native production payload. [Track-value matching](dash-period-catalog-20261028.md) fixes reference-identity deduplication after storage. See `validation/dash-period-catalog-build-20261028.json`. Runtime checks, installation and publication remain pending.

## Native DASH manifest reader (2026-10-28)

All five production targets and setup compiled successfully with the [native MPD reader](native-dash-manifest-20261028.md). Browser 0.63.61 is unchanged. Live addressing, refresh and recording integration remain unfinished. See `validation/native-dash-manifest-build-20261028.json`. Runtime checks, installation and publication remain pending.

## Native DASH recording interval (2026-10-28)

The native DASH groundwork now includes manifest refresh, track continuity, resource receipts, recording recovery and staging assembly. The [capture interval contract](native-dash-capture-interval-20261028.md) persists the requested start and stop and requires each track to cover that interval before assembly. Browser 0.63.61 is unchanged. Live DASH is still unavailable through job dispatch: credential scoping, polling, Period transitions and final publication need integration. Runtime testing remains deferred at the user's request. The candidate is uninstalled and unpublished; see `validation/native-dash-capture-interval-build-20261028.json` for compilation and packaging records.

## Native DASH media credentials (2026-10-28)

The [resource credential API](native-dash-resource-credentials-20261028.md) now requires explicit origin scopes for initialization and media requests, including alternate URLs and redirects. An unlisted host receives no supplied or implicit browser/site credentials. Live job construction of these scopes and the remaining live DASH workflow are still unfinished. Browser 0.63.61 is unchanged, runtime testing remains deferred, and candidates remain uninstalled and unpublished.

## Native DASH stop finalization (2026-10-28)

The [stop finalization operation](native-dash-stop-finalization-20261028.md) freezes the complete common audio/video interval and retains excluded tail records and files. Capture cannot append to a sealed recording. Explicit requested end coverage remains mandatory. Live job and browser integration are still unfinished; runtime testing remains deferred. Build and packaging evidence is recorded in `validation/native-dash-stop-finalization-build-20261028.json`.

## Native DASH job dispatch (2026-10-28)

The [native live DASH worker](native-dash-job-worker-20261028.md) connects job dispatch, stable track selection, polling, capture, Stop finalization, assembly and verified publication across destination volumes. Browser admission is still explicitly disabled pending extension selection and request-scope integration. Period transitions, static-MPD resume bootstrap and the other limitations in the worker document remain unfinished. Runtime tests are deferred; candidates are uninstalled and unpublished. See `validation/native-dash-job-worker-build-20261028.json` for build status.

## DASH ended-stream resume (2026-10-28)

The [resume bootstrap](native-dash-ended-resume-20261028.md) now preserves initial manifest and track identity so a new recording can resume after its MPD becomes static. Final-boundary retries retain their requested end, and persisted XML cannot masquerade as a fresh direct clock sample. Browser handoff and Period transitions remain unfinished. Runtime tests are deferred, with compilation and packaging recorded in `validation/native-dash-ended-resume-build-20261028.json`.

## Browser live DASH handoff (2026-10-28)

Extension 0.63.62 adds [stable-ID live DASH catalog selection and handoff](browser-live-dash-handoff-20261028.md). The desktop advertises `live-dash` and admits its bounded exact-origin credential contract. The previous browser-admission gate is superseded for clear single-Period SegmentTemplate MP4/audio selections. Period transitions and the documented unsupported modes remain unfinished. Syntax checks passed; runtime testing is deferred. See `validation/browser-live-dash-handoff-build-20261028.json` for native build and packaging evidence.

## Concurrent live DASH tracks (2026-10-28)

The [track-worker update](native-dash-track-workers-20261028.md) downloads selected tracks concurrently while retaining per-track order, serialized checkpoints and shared bandwidth/quota accounting. A one-connection setting remains sequential, and empty polling windows launch no workers. Browser 0.63.62 is unchanged. Runtime testing is deferred; see `validation/native-dash-track-workers-build-20261028.json` for compilation and packaging results.

## Live DASH metadata accounting (2026-10-28)

The [metadata accounting update](native-dash-metadata-accounting-20261028.md) includes completed manifest and clock-response bodies in shared job limits and transferred-byte counters without increasing saved-media progress. Application waits preserve network timeout allowance but do not reset clock or manifest freshness. Browser 0.63.62 is unchanged and runtime testing remains deferred. See `validation/native-dash-metadata-accounting-build-20261028.json` for build results.

## Native DASH Period transition planning (2026-10-28)

The [next-Period planner](native-dash-period-transition-20261028.md) resolves an adjacent Period while preserving selected format metadata and rejecting gaps, overlaps or ambiguous matches. Durable multi-Period session capture and joined publication remain unfinished; browser live multi-Period admission stays restricted. Runtime testing is deferred and Browser 0.63.62 is unchanged. Build evidence is in `validation/native-dash-period-transition-build-20261028.json`.

## Live DASH revision recovery (2026-10-28)

The [revision checkpoint](native-dash-revision-checkpoint-20261028.md) persists the latest accepted manifest before its media requests. Resume retains publication-order checks and learned Period ends across restarts while preserving the original presentation identity. Automatic multi-Period capture remains unfinished. Browser 0.63.62 is unchanged and runtime testing is deferred; see `validation/native-dash-revision-checkpoint-build-20261028.json` for build evidence.

## Live DASH assembled-output recovery (2026-10-28)

The [assembly recovery receipt](native-dash-assembly-recovery-20261028.md) allows a sealed recording to reuse its completed local output after verifying the recording binding, interval, byte count and SHA-256. New output is flushed before its receipt commits; failed verification falls back to retained-media assembly. Multi-Period session coordination remains unfinished. Browser 0.63.62 is unchanged and runtime tests are deferred. Build results are in `validation/native-dash-assembly-recovery-build-20261028.json`.

## DASH recording session assembly (2026-10-28)

The [session assembler](native-dash-session-assembly-20261028.md) joins compatible sealed Period outputs after checking presentation identity, contiguous capture boundaries, locked-file hashes and stream signatures. The existing live worker uses the one-Period path; durable multi-Period capture advancement remains unfinished. Browser 0.63.62 is unchanged. Runtime testing is deferred; see `validation/native-dash-session-assembly-build-20261028.json` for compilation and packaging evidence.

## DASH recording session journal (2026-10-28)

The [persistent session journal](native-dash-session-journal-20261028.md) now supplies the worker's current Period and final assembly order, checkpoints selected identities and capture boundaries, and commits saving only after sealing. It adopts reconciled legacy recordings and rejects missing or changed recorded state. Automatic capture advancement remains unfinished; Browser 0.63.62 is unchanged. Runtime testing is deferred, with build results in `validation/native-dash-session-journal-build-20261028.json`.

## Automatic DASH Period advancement (2026-10-28)

The [advancement worker](native-dash-period-advancement-20261028.md) now seals a captured Period, prepares the next bootstrap/index, commits the session switch and continues capture. Aggregate progress, advancing-phase recovery and Stop at an empty new tail are connected to ordered session assembly. This supersedes the earlier lack of native automatic advancement for initially single-Period live selections; initial multi-Period browser selection and other documented modes remain unfinished. Browser 0.63.62 is unchanged. Runtime behavior is unverified; compilation and packaging are recorded in `validation/native-dash-period-advancement-build-20261028.json`.

## Initial multi-Period live DASH selection (2026-10-28)

Extension 0.63.63 adds [native-clock-based initial Period selection](browser-live-dash-period-selection-20261028.md), with a fresh manifest query and another check before handoff. It supersedes the earlier initial multi-Period catalog rejection while preserving quality/audio choices and the native capture session. Syntax checks passed; runtime browser and recording tests remain deferred. Compilation and packaging are recorded in `validation/browser-live-dash-period-selection-build-20261028.json`.

## Joined DASH output recovery (2026-10-28)

The [joined-output receipt](native-dash-session-output-recovery-20261028.md) allows a verified completed multi-Period file to survive an interrupted final save without repeating the final concat. It binds the full ordered recording state and output selection, independently of each Period receipt. This supersedes the earlier lack of a whole-session output checkpoint. Browser 0.63.63 is unchanged. Runtime testing remains deferred.

## DASH Stop reconciliation (2026-10-28)

The [Stop path](live-dash-stop-reconciliation-20261028.md) now recovers current media receipts before deciding whether a newly started Period is empty. It avoids excluding committed segments based on the pre-cancellation snapshot, updates the session journal and refreshes saved progress. Browser 0.63.63 is unchanged. Runtime testing remains deferred.

## Mixed live DASH catalogs (2026-10-28)

Extension 0.63.64 [retains supported qualities](browser-live-dash-mixed-addressing-20261028.md) when another representation has unsupported list/index addressing. Unsupported-only audio produces an explicit error rather than a muted video plan. Native live list/index transfer remains unfinished; the review identifies range-bound resources as the next implementation prerequisite. JavaScript syntax checks passed; runtime tests remain deferred. The existing UDM 1.1.0 native binaries are reused.

## DASH ranged-resource foundation (2026-10-28)

The native resource store adds [range-bound identity, exact HTTP 206 validation and receipt-length checks](native-dash-range-resource-20261028.md). Existing template callers keep whole-resource behavior. Live list/index planning, descriptor persistence and shared resource-version validation remain unfinished, so this change does not enable unsupported catalog entries. Extension 0.63.64 is unchanged; runtime tests remain deferred.

## DASH range persistence (2026-10-28)

[Media and initialization ranges](native-dash-range-recording-20261028.md) now flow through planned descriptors, pending recording checkpoints and offline reconciliation. Refresh cannot change a recorded slice or initialization range within the Period. This completes descriptor persistence from the prior foundation; native list/index planning, shared file-version consistency and browser admission remain unfinished. Runtime testing remains deferred and extension 0.63.64 is unchanged.

## Native SegmentList planning (2026-10-28)

The [native list planner](native-dash-list-planning-20261028.md) resolves inherited resources, initialization ranges, bounded timeline expansion and live availability. Both selected-track admission and browser catalogs remain gated until shared resource-version validation is connected. SegmentBase/index parsing is still separate. This supersedes the earlier missing list-planner implementation, not the remaining end-to-end acceptance requirements. Runtime tests remain deferred; extension 0.63.64 is unchanged.

## DASH resource-version binding (2026-10-29)

The [resource store](native-dash-resource-version-20261029.md) can bind a strong ETag and total file length to an exact URL and include them in protected cache identities. Conditional requests, response validators, range totals and whole-file lengths are enforced. Shared-source version discovery and recording persistence still need integration before live list admission is enabled. Runtime tests remain deferred; extension 0.63.64 is unchanged.

## DASH recording version persistence (2026-10-29)

[Version descriptors now persist](native-dash-version-recording-20261029.md) through media and initialization checkpoints and offline reconciliation. Capture rejects a refresh that replaces a saved version, and recording-wide checks reject conflicting versions for one exact URL across tracks. This supersedes the missing descriptor persistence above. Automatic discovery and assignment still remain before live list admission; runtime tests remain deferred and extension 0.63.64 is unchanged.

## Native live SegmentList capture (2026-10-29)

[Bounded resource discovery](native-dash-list-capture-20261029.md) now supplies size and strong ETag descriptors to listed capture windows. Native list admission is enabled, with saved-version reuse, protected checkpointing and conditional transfers. Browser live-list catalog/capability integration remains pending; SegmentBase index parsing remains separate. This supersedes the prior native admission gate and missing automatic version assignment. Runtime tests remain deferred; extension 0.63.64 is unchanged.

## Browser live SegmentList selection (2026-10-29)

Extension 0.63.65 [adds live list video/audio choices](browser-live-dash-list-20261029.md) and checks the selected tracks for the updated desktop capability before handoff. Origin lookup URLs are bounded without sending the full segment list. This completes the browser/native source integration missing above; native strong-validator requirements still apply. SegmentBase parsing and runtime acceptance remain unfinished. JavaScript syntax checks passed; runtime testing remains deferred.

## Native DASH index decoding (2026-10-29)

The [native SIDX decoder](native-dash-index-parser-20261029.md) adds bounded 32/64-bit index parsing with byte ranges, media clocks and separate external-index address spaces. This supplies the decoding prerequisite for SegmentBase; recursive retrieval, continuity validation and manifest/browser integration remain unfinished. Runtime tests remain deferred; extension 0.63.65 is unchanged.

## Native DASH index-tree resolution (2026-10-29)

[Hierarchical index traversal](native-dash-index-tree-20261029.md) now checks identity, exact clock continuity, overlap and external media placement under bounded fetch budgets. A version-bound resource-store adapter supplies verified binary bytes and rate/quota accounting. This supersedes the missing recursive reader and continuity checks above; SegmentBase manifest/browser integration and runtime acceptance remain pending. Extension 0.63.65 is unchanged.

## Native SegmentBase materialization (2026-10-29)

[Inherited SegmentBase resolution](native-dash-base-materialization-20261029.md) now connects index and initialization locators to the version-bound reader and produces media-list descriptors with exact integer clock conversion. Unmaterialized planning and live selected-track admission remain gated until capture clock/identity integration is complete. Browser SegmentBase support and runtime acceptance remain unfinished. Extension 0.63.65 is unchanged.

## Native live SegmentBase capture (2026-10-29)

[Indexed capture checkpoints](native-dash-base-capture-20261029.md) now preserve exact clocks, media ranges and source versions through startup, refresh, restart and Period transitions. Native selection is enabled while unresolved capture remains rejected. This supersedes the prior native clock-integration gate. Browser SegmentBase catalog/capability work and runtime acceptance remain pending; extension 0.63.65 is unchanged.

## Browser live SegmentBase selection (2026-10-29)

[Extension 0.63.66](browser-live-dash-base-20261029.md) adds indexed live video/audio choices, inherited resource locators and a selected-track native capability check. This completes browser/native source integration for the immutable indexed resource path described above. Runtime acceptance remains deferred; mutable growing resources and incomplete low-latency chunks remain unsupported.

## Live template initialization locators (2026-10-29)

[Inherited Initialization children](dash-template-initialization-20261029.md) now supply literal whole/ranged initialization resources to native live template capture, including version-bound ranged recovery. Extension 0.63.67 adds selection metadata and desktop capability negotiation. The recorded browser parser also includes inherited child initialization through the existing segment contract. Runtime acceptance remains pending.

## Recorded DASH final-segment bounds (2026-10-29)

[Extension 0.63.68](recorded-dash-final-segment-20261029.md) now stops fixed-duration and timeline plans at their inherited final segment number, matching the existing native bound. It adds exact-integer and timeline-continuity guards. Native binaries are unchanged from the prior successful build; runtime acceptance remains deferred.

## Finalized DASH completion (2026-10-29)

[Finite static addressing](dash-finalized-end-20261029.md) now supplies a final recording boundary when a stream becomes static without an explicit Period duration. Every selected track must have a bounded end, and committed media must cover the common interval before saving. Still-dynamic streams continue refreshing. Extension 0.63.68 is unchanged; runtime acceptance remains deferred.

## Downloading finalized DASH remainders (2026-10-29)

[Static remainder capture](dash-finalized-drain-20261029.md) now adopts a finite ending immediately and fetches bounded windows without live polling sleeps. Empty finalized coverage uses bounded retries instead of resetting the failure counter indefinitely. Dynamic recording and transfer rate accounting remain unchanged. Extension 0.63.68 is unchanged; runtime acceptance remains deferred.

## Standalone DASH caption clocks (2026-10-29)

[Extension 0.63.69](dash-sidecar-caption-clock-20261029.md) now gives side-loaded WebVTT the neutral Period-relative clock in multi-Period selections, avoiding an extra media timestamp offset. Segmented MP4 captions remain unfinished; runtime alignment acceptance remains deferred. Native production binaries are unchanged.

## MP4 WebVTT sample decoder (2026-10-29)

[A bounded native payload reader](mp4-webvtt-samples-20261029.md) now parses typed WVTT sample cues. Container initialization/fragment mapping and download integration remain pending; browser admission is unchanged at 0.63.69. The decoder is compiled but not yet used by downloads. Runtime testing remains deferred.

## MP4 WebVTT initialization (2026-10-29)

[Track-bound initialization metadata](mp4-webvtt-initialization-20261029.md) now supplies the caption clock, sample-description identity, configuration and fragment defaults. Fragment mapping and download assembly remain pending. Runtime testing remains deferred; extension 0.63.69 is unchanged.

## MP4 WebVTT fragment mapping (2026-10-29)

[Fragment sample mapping](mp4-webvtt-fragments-20261029.md) now combines track defaults, sample runs and decode/composition clocks, with bounded media-data extents and integer timing. Initialization verifies self-contained data references. Download integration, browser admission and runtime acceptance remain pending; extension 0.63.69 is unchanged.

## Native MP4 WebVTT assembly (2026-10-29)

[Caption download assembly](mp4-webvtt-assembly-20261029.md) now invokes initialization and fragment decoding for explicitly marked recorded DASH subtitle tracks. Single- and multi-Period paths produce timed WebVTT input for muxing, retain cue metadata, and bind the selection through existing recovery identity. Browser discovery/admission and runtime acceptance remain pending; extension 0.63.69 is unchanged.

## MP4 WebVTT browser handoff (2026-10-29)

[Extension 0.63.70](dash-mp4-webvtt-browser-20261029.md) now discovers recorded DASH MP4 WebVTT captions, preserves their clocks, and checks the desktop caption capability before handoff. Native decoding and assembly are connected. Runtime download/rendering acceptance remains deferred; the package is not installed or published.

## Caption cue metadata and line endings (2026-10-29)

[WebVTT cue fidelity](webvtt-cue-fidelity-20261029.md) now retains plaintext cue identifiers/settings and normalizes CRLF, CR and LF across plaintext parsing, mapped headers and MP4 cue serialization. Final styling fidelity and plaintext STYLE/REGION preservation remain unproven or incomplete. Extension 0.63.70 is unchanged; runtime tests remain deferred.

## WebVTT style and region blocks (2026-10-29)

[Caption configuration preservation](webvtt-configuration-20261029.md) now carries consistent STYLE/REGION blocks through recorded and live plaintext assembly. Mapped headers preserve block separation; conflicting definitions are rejected. Final muxed rendering remains unverified, and runtime tests remain deferred. Extension 0.63.70 is unchanged.

## Main-list text sorting (2026-10-29)

[Text sort-key preparation](list-text-sort-20261029.md) now computes lowercase filename, queue, description, path and recovery keys once per visible row, rather than repeatedly inside comparisons. Stable sort semantics and refresh freshness remain unchanged. Runtime speed measurements remain deferred; extension 0.63.70 is unchanged.

## Main-list derived sorting (2026-10-29)

[Derived sort keys](list-derived-sort-20261029.md) now prepare size, status/progress and ETA values once per visible row; date keys use hash lookup, and comparisons avoid shared-pointer copies. Runtime speed measurements remain deferred. Extension 0.63.70 is unchanged.

## Toolbar online discovery (2026-10-29)

[Toolbar discovery](toolbar-discovery-20261029.md) now opens the explicitly labeled official IDM gallery, with a separate local UDM toolbar-folder action. This supersedes the earlier local-folder-only source limitation. Existing .tbi loading remains responsible for validation; per-skin compatibility and installed UI behavior are not qualified. Extension 0.63.70 is unchanged.

## Toolbar package subfolders (2026-10-29)

[Toolbar subfolder support](toolbar-subfolder-20261029.md) addresses a source incompatibility found in the official Pure Flat and Kavian ZIP metadata. Relative BMP subfolders are accepted with bounded path components and rejection of traversal/reparse points. Five packages were inspected as format evidence; none were installed or bundled. Runtime rendering remains unverified. Extension 0.63.70 is unchanged.

## Toolbar menu names (2026-10-29)

[Declared toolbar names](toolbar-names-20261029.md) are now read from bounded metadata for every discovered skin, without decoding all images on menu opening. Invalid metadata has a filename fallback; full image validation remains on selection. Runtime menu behavior remains unverified.

## DASH duration-template offsets (2026-10-29)

[Duration-template offset handling](dash-duration-offset-20261029.md) now uses the declared sample-clock origin in native planning and recorded browser URL expansion. Explicit timelines and SegmentList timing remain unchanged; boundary-adjustment support and runtime acceptance remain pending. Browser version is 0.63.71.

## Recorded DASH Period windows (2026-10-29)

[Period-window expansion](dash-period-window-20261029.md) now filters explicit timeline repeats before generating recorded browser URLs, preserving original numbering and retaining boundary overlaps. Runtime output acceptance remains deferred. Extension version is 0.63.72; native binaries are unchanged.

## Single-Period DASH assembly gap (2026-10-29)

[Assembly contract review](dash-single-period-contract-review-20261029.md) confirms that filtering segment references does not by itself add declared-clock clipping to the legacy single-Period merge path. The display-only duration hint is excluded from recovery identity. A bound timing contract and assembly integration remain required; the 0.63.72 candidate is unchanged.

## Single-Period DASH timing integration (2026-10-29)

[Known-duration single-Period catalogs](dash-single-period-20261029.md) now use the recovery-bound Period assembly contract and capability gate. The source preserves optional embedded audio when no separate audio is selected. Browser version is 0.63.73; runtime acceptance remains deferred.

## Media receipt selection identity (2026-10-29)

[Recorded media receipts](media-receipt-selection-20261029.md) now bind timing and selected track/segment structure instead of relying only on page and format labels. Ambiguous older receipts and unresolved conflicting selections require recovery review. Browser version is 0.63.74; native binaries are unchanged and runtime testing remains deferred.

## Accepted media receipt revalidation (2026-10-29)

[Receipt revalidation](media-receipt-revalidation-20261029.md) now asks the running desktop whether an accepted download still has a history entry, both on explicit review and duplicate recovery. Historical acceptance is retained when the current session cannot verify it. Extension version is 0.63.75; runtime testing remains deferred.

## Large media admission (2026-10-29)

[Media admission limits](media-admission-limit-20261029.md) now agree with the existing native request transport budget. The desktop advertises its limit and the extension checks complete request size before reserving a new receipt. Adaptive plan limits remain unchanged. Browser version is 0.63.76; runtime testing remains deferred.

## Catalog serialization locking (2026-10-29)

[Catalog serialization](catalog-serialization-20261029.md) now runs outside the manager mutex for periodic saves, using an already detached snapshot and preserving write-sequence ordering. Snapshot copying and outer-lock callers remain unchanged; runtime responsiveness and concurrency checks are deferred.

## Durable completion checkpoint (2026-10-29)

[Completion checkpoint publication](catalog-completion-checkpoint-20261029.md) now follows successful catalog writes. Completion checks reuse the cached JSON and require the saved record to match the current job, including child records. Runtime concurrency and failure acceptance remain deferred.

## Recorded DASH earliest-presentation offsets (October 29)

Extension 0.63.77 adds bounded recorded eptDelta support: nominal coverage changes the chosen segment range while template URI values retain their original numbering/clock. The finite Period assembly path handles output boundaries. See [scope and limitations](dash-recorded-ept-20261029.md). Runtime acceptance remains deferred; this candidate is not installed or published.

## Native DASH earliest-presentation offsets (October 29)

The native resolver now separates offset-adjusted segment coverage from template URL timing. Nonzero offsets bind recovery identity and are checked across refreshes. Extension 0.63.78 gates the live handoff on desktop support. See [implementation scope](dash-native-ept-20261029.md). Runtime acceptance remains deferred; no installation or publication is performed.

## DASH SegmentList index metadata (October 29)

Extension 0.63.79 and the native resolver accept validated optional SegmentURL index metadata when transferring complete declared media segments. Index-based subsegment selection remains unfinished. See [scope and limitations](dash-list-index-metadata-20261029.md). Runtime acceptance remains deferred, with no installation or publication.

## Recorded DASH representation-index templates (October 29)

Extension 0.63.80 connects finite-Period SegmentTemplate index locators to existing SIDX resolution and corrects SegmentList assembly clock extraction. See [scope and remaining limits](dash-recorded-template-index-20261029.md). Runtime acceptance remains deferred; this candidate is uninstalled and unpublished.

## Native DASH representation-index templates (October 29)

The native resolver now normalizes representation-wide template indexes into the existing indexed-media capture and recovery path. The live browser catalog connection remains pending. See [scope and limits](dash-native-template-index-20261029.md). Extension 0.63.80 is unchanged; runtime acceptance remains deferred.

## Live browser DASH representation-index integration (October 29)

Extension 0.63.81 connects live index-template catalog selection and capability negotiation to the native resolver. This supersedes the preceding live-browser integration gap. See [scope and limits](dash-live-template-index-20261029.md). Runtime acceptance remains deferred; the candidate is uninstalled and unpublished.

## Recorded DASH SegmentList Period windows (October 29)

Extension 0.63.82 validates list/timeline correspondence and filters timed recorded lists to overlapping segments for finite Period assembly. Untimed legacy lists retain full-list behavior. See [scope and limitations](dash-recorded-list-window-20261029.md). Runtime acceptance remains deferred; the candidate is uninstalled and unpublished.

## Scheduler queue-role correction (October 29)

Scheduler now uses stored built-in queue roles instead of literal names to control queue-type switching, with matching persistence enforcement. A fresh reference-to-source review distinguishes implemented Scheduler workflows from deferred behavior checks. See [review and change scope](scheduler-queue-role-20261029.md). Runtime acceptance remains deferred; extension 0.63.82 is unchanged.


## Synchronization local length check — 2026-10-29

Synchronization now requires the saved destination length to match catalog metadata before reporting Up to date. A mismatch uses the existing replacement workflow and retains the previous file. Converted offline page source/local lengths remain separate. Same-length edits are not detected by this check. All five production targets and installer packaging passed; runtime tests remain deferred. See [implementation notes](sync-local-size-20261029.md) and [build evidence](validation/sync-local-size-build-20261029.json). Extension 0.63.82 is unchanged; nothing was installed or published.


## MP4 WebVTT edit-list timing — 2026-10-29

MP4 captions now accept a single normal-rate edit in elst version 0 or 1. Assembly clips and shifts media cues before applying DASH PTO, with finite duration measured in the movie timescale. Multiple, empty and rate-changing edits remain unsupported. All five production targets and installer packaging passed; runtime tests remain deferred. See [implementation notes](mp4-webvtt-edit-20261029.md) and [build evidence](validation/mp4-webvtt-edit-build-20261029.json). Extension 0.63.82 is unchanged; nothing was installed or published.


## MP4 WebVTT edit sequences — 2026-10-29

MP4 caption assembly now maps up to 64 normal-rate edits, including empty delays and repeated/reordered sections. Expansion respects cue/text budgets and output is sorted in movie time. Rate-changing edits remain unsupported. All five production targets and installer packaging passed; runtime tests remain deferred. See [implementation notes](mp4-webvtt-edit-sequence-20261029.md) and [build evidence](validation/mp4-webvtt-edit-sequence-build-20261029.json). Extension 0.63.82 is unchanged; nothing was installed or published.


## Server filename category routing — 2026-10-29

Use server name now recalculates automatic category/folder selection using the accepted filename and final response host. Manual category choices and custom folders are preserved. All five production targets and installer packaging passed for current version 1.2.0 after the repository version changed during the build. Runtime tests remain deferred. See [implementation notes](server-name-category-20261029.md) and [build evidence](validation/server-name-category-build-20261029.json). Extension 0.63.82 is unchanged; nothing was installed or published by this batch.


## Synchronization missing-file recovery — 2026-10-29

Synchronization now restores missing destinations through duplicate restart, retaining the original history ID and using the returned job for transfer. Existing-file replacement still keeps the previous version. All five production targets and installer packaging passed for 1.2.0; runtime tests remain deferred. See [implementation notes](sync-missing-recovery-20261029.md) and [build evidence](validation/sync-missing-recovery-build-20261029.json). Extension 0.63.82 is unchanged; nothing was installed or published.
