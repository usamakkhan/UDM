# UDM 0.85.0 RC1 — browser 0.63.0

RC1 packages the [changes since 0.84.0 RC3](changelog-rc3-to-085-rc1.md). It preserves single-use links during preview and transfer startup, retains the first full HTTP response, and supports Digest authentication for explicit HTTP proxies. Native binaries report 0.85.0; both browser manifests report 0.63.0.

The package was built with Inno Setup 6.7.3 from an isolated tree containing fresh native binaries, both browser bundles, FFmpeg tools, license files, and a freshly rebuilt signed-network helper. The helper reports verified runtime and desktop broker protocol 1. The installer is not publisher-signed.

The exact 0.85.0 native test executable passed 2,589 checks. The rebuilt network helper passed 82 core checks with local loopback access. Focused browser recognition and long-playlist tests passed. The initial-response change separately passed eight controlled live cases. The exact installer was not executed or installed, so clean-machine installation, upgrade, rollback, and live driver-enabled gateway operation remain untested. This is a prerelease; complete IDM parity is not established.

Package size, digest, and source commit are recorded in [machine-readable release evidence](validation/rc1-085-release-20261009.json).
