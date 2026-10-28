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
