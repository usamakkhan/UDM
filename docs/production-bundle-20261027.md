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
