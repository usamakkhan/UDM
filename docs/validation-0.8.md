# UDM 0.8 validation — 19 September 2026

The migrated project at `D:\UDM` built successfully with the native C++/MFC toolchain. Final native checks: **81 passed, zero failed** (`release-native/native-test-evidence.json`). Browser logic checks: **26 browser + 4 capture + 6 UMP + 25 cross-site = 61 passed**. A real native-host stdio/desktop-pipe ping passed.

Chrome loaded UDM Browser Integration **0.8.0** from the migrated source through the preserved junction. On the user's open YouTube video, the compact panel was observed at the upper-right player edge. Dragging moved it beside IDM's own panel. Its menu showed the detected 144p, 240p, 360p, 480p, 720p and 1080p choices; no 2K/4K/8K option appeared. UDM's panel hid during the observed advertisement. A CSS shadow-host reset conflict found during this check was corrected.

Selecting 1080p did not produce a saved native job during this test. Successful current YouTube capture/download and speed parity remain unverified.

The native tests verify downloaded segment ranges, completed-part reuse, MP4 assembly, requested height/audio, checksum rejection and destination collisions using local fixtures. HLS/DASH parsing and frame/player association pass controlled JavaScript tests. A synthetic direct/iframe/HLS browser fixture was created, but its live Chrome-to-desktop test was interrupted when the computer-control tool could not verify Chrome's current URL and stopped UI input. Chrome currently grants only targeted YouTube/media-host access; broad-site activation remains a user permission step.

No claim is made of every-platform compatibility, universal ad exclusion, live-stream or DRM support. Firefox/Edge live validation, large media stress tests and production signing remain outstanding. The optional driver was not installed or kernel-tested in this work.
