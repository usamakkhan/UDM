# Current parity completion audit

The objective remains full IDM behavior parity with UDM branding. Completion is not established. This audit checks the current candidate and selected remaining source boundaries; it is not an exhaustive execution or a parity percentage.

## Artifact state

The latest installer is UDM 1.0.4 with browser 0.63.56, from `setup/dash-refresh-duration-20261028/`. A fresh read verified its recorded SHA-256 and all 147 staged payload hashes, with no mismatches. The installed UDM executable has a different hash. The candidate remains unsigned, uninstalled and unpublished. Exact values are in `validation/parity-completion-audit-20261028.json`.

Compilation records cover the shipping native targets and installer. They do not prove current installed behavior, real transfers, UI fidelity, speed parity, recovery, or domain authentication. Runtime tests are deferred by the user's current instruction.

## Remaining work, separated by evidence

| Area | Current evidence | What remains |
|---|---|---|
| Delivery | Candidate hashes match; installed executable differs | Install and qualify the current candidate when that phase resumes |
| Proxy authentication | Automatic GET/HEAD and rejected-CONNECT HTTPS form support; explicit method selection in source/build | Domain, mixed-method, form, browser-routing and existing NTLM runtime acceptance |
| Speed limits | Short reservations and combined-deadline pacing built | Concurrent throughput, setting changes and cancellation measurements |
| Import/export | Request details, sorting, metadata, selection and request-integrity safeguards built | Current installed workflows and real IDM export round trips |
| Recorded DASH | Declared duration and refresh compatibility built | Old-capture refresh, visual display and actual output acceptance |
| Live/multi-period DASH | `media.js::dash` rejects dynamic MPDs and multiple Periods; native live admission accepts HLS only | Reference parser support established in `dash-live-period-reference-20261028.md`; implement native refresh, Period assembly and recovery, then qualify actual recording behavior |
| Signing/updater | Signing and signed-manifest updater code exist; current setup unsigned | Publisher identity, signed delivery and installed upgrade/rollback qualification |
| ARM64 | Native build selects HostX64/TargetX64 dependencies | Architecture-specific dependencies/build and real ARM64 execution; driver support is separate |
| Driver, physical failures, accessibility | Earlier narrower records exist; no current broad matrix was executed | VPN/HVCI, sleep/power, device/disk faults, mixed DPI and screen-reader acceptance |
| Localization and legacy protocols | English-only remains documented; MMS and old IE-module integration excluded in the standing scope | Do not silently count these as identical or add them without revisiting the scope decision |

Encrypted/DRM media remains explicitly rejected. A source-level rejection is proof of a UDM limit, not proof that the reference can download every corresponding protected stream. No bypass work is implied by the parity goal.

## Corrections to the backlog

The proxy decision document is updated to include the explicit authentication selector and HTTPS form path. The active program no longer points to browser 0.63.50 as the current stage. Its original merge/build order is labeled historical rather than presented as work still to repeat.

No application source was changed in this audit, so the successful candidate was not rebuilt redundantly. The next implementation should be driven by a confirmed reference gap; live/multi-period DASH needs reference and format analysis rather than removal of rejection checks. Deferred runtime acceptance remains necessary before claiming full parity.
