# UDM 0.85.0 RC3 — browser 0.63.0

RC3 packages the [changes since RC2](changelog-085-rc2-to-rc3.md). Captured partial HTTP responses now preserve strong validators through the signed broker and desktop catalog, and UDM resumes them only after remote identity and range checks. Native binaries remain 0.85.0 and both browser manifests remain 0.63.0.

The installer was compiled with Inno Setup 6.7.3 from the pinned release branch at product commit `1fb2877`. It contains freshly built native binaries, both browser bundles, FFmpeg tools, licenses, and a rebuilt signed-network helper. The helper reports verified runtime and desktop broker protocol 1. The installer is not publisher-signed.

The full native suite passed 2,589 checks; the network helper passed 116 core checks; and three focused browser test files passed. The controlled resume receipt records 111 passing checks. The gateway suite passed 116 functional checks, but its administrator-token environment check could not pass from this shell. The exact RC3 installer was not executed or installed; clean-machine installation, upgrade, rollback, and live driver-enabled gateway operation remain untested for this package.

Browser authentication or session-header replay and HTTPS original-response takeover remain outside this release. Cleanup of the retained original prefix after successful continuation also remains open. Complete IDM parity is not established. This is a prerelease.

Package size, SHA-256, and validation details are in [the machine-readable release record](validation/rc3-085-release-20261010.json).
