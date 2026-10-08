# Browser 0.62.13 installer — 8 October 2026

Built and saved `D:\UDM\installer-out\UDM-0.84.0-Browser-0.62.13-Setup-x64.exe` from application source a15e8fe and the verified native candidate. This consolidates recorded and live WebVTT header support, browser navigation capture guards, video-panel source-change recovery, and Properties subtitle labels.

Size: 59,135,175 bytes. SHA-256: `35A5B2ED5508B6B71C17E5337521FB521EC3C031F816A5209E1B5C23149D9BDC`. All 144 staging input hashes remained unchanged after compilation and after browser acceptance. The four native companions declare 0.84.0 and both extensions declare 0.62.13. Packaging also passed the signed network-runtime status/protocol checks; it did not load a driver.

Fresh acceptance of the exact staged app, host and extension passed 15 isolated Edge checks: selected Spanish audio in MP4/M4A, exact parallel ranges, ignored-Range rejection, live captions, one-time WebVTT initialization fetching, output metadata/timing/text and complete decoding. The temporary native-host registration was removed. Only the original personal UDM process remained among test app/browser/compiler processes checked afterward.

The staged SetupHelper passed 10 isolated data-migration checks. Custom history location, paused partial bytes and state hashes survive prepare/rollback; later user-edited configuration is preserved; conflicting catalogs are rejected without mutation. The first wrapper reported failure because the test's intentional conflict left native LASTEXITCODE=1 despite ten passing assertions. `installer/test-data-helper.ps1` now clears that stale code only after every assertion succeeds, and the fresh rerun returns success to the caller. This test-runner change is not an installer payload change.

Prior source/candidate checks remain separately scoped: 143 native, 131 browser regressions, 15 recorded-caption Edge and 14 recorded-caption Firefox checks. Candidate app/host hashes were verified before staging. No fresh clean/full-native build or real installer execution was performed. This is not a clean-machine install, upgrade, uninstall or rollback qualification.

The installer is built, but not installed, publisher-signed or published. The running personal `D:\UDM\release\UDM.exe` remains untouched. Public-site comparisons, equal-source speed measurements, full GUI/driver compatibility and complete IDM parity remain open.

Two completed disposable test profiles were removed after successful results and native-registration cleanup were checked, retaining all source, executables, logs, screenshots and media output. They contained 53,099,830 bytes; filesystem free space increased from 56,623,104 to 138,805,248 bytes before saving the installer. D: had 79,429,632 bytes free afterward, so further large builds still need another staging location.

[Release receipt](validation/recorded-caption-release-20261008.json) · [Profile cleanup receipt](validation/observer-profile-cleanup-20261008.json). Raw package and acceptance evidence: `C:\Users\Abuzar\AppData\Local\Temp\udm-release-0.62.13-20261008`.
