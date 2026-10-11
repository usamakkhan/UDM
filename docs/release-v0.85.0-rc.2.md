# UDM 0.85.0 RC2 — browser 0.63.0

RC2 packages the [changes since RC1](changelog-rc1-to-rc2.md), including original HTTP response capture with durable recovery and native progress, explicit YouTube ad exclusion, and cleanup of duplicate response bodies after verified publication. All native files report 0.85.0; both browser manifests report 0.63.0.

The installer was compiled with Inno Setup 6.7.3 from the pinned release branch at product commit `7d2e393`. It includes fresh native binaries, both browser bundles, FFmpeg tools, licenses, and a freshly rebuilt signed-network helper. The helper reports verified runtime and desktop broker protocol 1. The installer is not publisher-signed.

The full native suite passed 2,589 checks. The network helper passed 113 core checks, and three focused browser test files passed. The gateway suite passed 113 functional checks; its administrator-token environment check could not pass from this shell. Earlier installed Edge evidence covers the application-owned capture path, but the exact RC2 installer was not executed or installed. Clean-machine installation, upgrade, rollback, and live driver-enabled gateway operation remain untested for this package. Original-response takeover remains HTTP-only, and complete IDM parity is not established.

Package size, SHA-256, and validation details are in [the machine-readable release record](validation/rc2-release-20261010.json). This is a prerelease. Newer validator changes on main are outside this release.
