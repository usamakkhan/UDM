# Browser 0.62.11 package — 2026-10-08

Built UDM-0.84.0-Browser-0.62.11-Setup-x64.exe from source aecf562 and the tested native candidate. It includes video-panel source-change recovery, live WebVTT initialization-header support, and corrected Properties subtitle link labels.

Installer: candidates/properties-release-20261008/project/installer-out/UDM-0.84.0-Browser-0.62.11-Setup-x64.exe. SHA-256: 363036CF6DD9DB1F241E7711B4186410EF4969CE20056A88E6D54BA3D3E1B030. Size: 59,134,220 bytes. All 144 staging input hashes remained unchanged after compilation and after acceptance.

Fresh acceptance against the exact staged app, host and extension: 15 isolated Edge checks passed, covering selected audio in MP4/M4A, exact parallel indexed ranges, ignored-Range rejection, live captions and separate-header fetching. Final media decodes and has the expected duration/text/track metadata. Temporary registration was removed.

Prior candidate/source results remain separately scoped: 349 menu, 19 address/search, 126 native live-subtitle, 17 browser-handoff and 6 focused Firefox mapped-caption checks. They were not rerun against this package. Native builds reused unchanged backend objects; no fresh clean/full-native build was performed.

Built but not installed, published, signed, or installer-lifecycle-tested. The user's installed UDM was still running during packaging and was not replaced. Full IDM parity, public-site coverage and equal-source speed equivalence remain unestablished.

Storage: archived six inactive test object files with SHA-256 verification, saving 116,616,887 bytes while preserving originals in ZIP files. NTFS compression was attempted but unsupported by this volume; it freed no space. Removed four inactive disposable test browser profiles only after successful results and native-registration cleanup were verified. Logs, screenshots, media output and results were retained. [Archive restoration inventory](validation/test-object-archives-20261008.json), [profile cleanup](validation/completed-browser-profile-cleanup-20261008.json).

[Package receipt](validation/properties-release-20261008.json).
