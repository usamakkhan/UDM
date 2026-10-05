# UDM browser 0.52.2: accurate manual resume and real Firefox popup acceptance

Follow-up: browser **0.53.0** is installed on disk and fixes the automatic Firefox fallback described below. See [new evidence and limits](parity-browser-0.53.0.md). Original 0.52.2 evidence follows.

Browser 0.52.2 is installed on disk beside native UDM 0.78.0. Existing personal browser sessions still need reload/activation verification. Full IDM parity remains incomplete.

## Fix

The real Firefox toolbar test exposed a false recovery offer: a response canceled before receiving any payload had `canResume:false`, but UDM treated Firefox's zero-byte USER_CANCELED state as resumable. Choosing Resume failed with “Download 1 cannot be resumed.” A previous successful test had retained 65,536 bytes and was actually resumable; it did not establish the zero-byte assumption.

Manual recovery now honors the browser's explicit resume capability. A saved browser-resume choice that can no longer run keeps its Resume option disabled and explains how to handle the file in browser Downloads before clearing the notice. Clearing it changes no transfer. Valid paused/resumable downloads keep the existing native release barrier and exact-file resume path. Mozilla's [Downloads implementation](https://searchfox.org/firefox-main/source/toolkit/components/extensions/parent/ext-downloads.js) likewise requires partial data for this capability; a canceled error alone is insufficient.

## Validation

- 304 targeted checks passed, including 33 manual-recovery checks and new capability, refresh and saved-choice cases.
- The actual Firefox 156.0.1 Extensions menu opened UDM's real popup. A test-only observer inside that popup confirmed `getViews({type:'popup'})` identity, clicked the existing connection/recovery controls and recorded the real replies. Neither runtime replies nor the native backend were mocked. Firefox's remote popup frame cannot be entered through WebDriver's normal frame command, so this observer is limited to the disposable test extension and is absent from the shipped browser files.
- Eleven Firefox checks passed across actual popup entry, isolated native connection, four interrupted responses, resume/cancel/dismiss, a non-resumable saved choice, reopening and history refresh. The positive fixture waits for real partial bytes before pausing; a separate finalized cancellation proves the negative case. Resumed bytes match SHA-256 and native release precedes the browser's range request.
- Nineteen actual Edge checks passed over normal and restarted profiles, including its real popup, sender boundary, correct unavailable-resume behavior and native Downloads button.
- All 16 updated files are backed up and verified. All 49 native/runtime package files remained unchanged. All 25 download records, queues and projects remain unchanged.

The personal state hash changed during testing only because the saved basket coordinates changed from (1495,128) to (2372,53). Full JSON comparison established that those two fields were the sole differences. Their cause was not established; the newer position was preserved. Deployment then retained the complete current catalog byte-for-byte. [Catalog reconciliation](evidence-browser-0.52.2/catalog-baseline.json).

The first toolbar probe intentionally stopped after collecting controls; a subsequent fixture hit WebDriver's unsupported remote-frame operation. The next real-popup run exposed the product defect. All three are retained beside the successful corrected run. Headless screenshots do not capture the floating Firefox popup; actual popup identity, controls and replies are recorded separately. This is functional acceptance, not complete popup visual parity.

## Still open

This fix concerns explicit legacy recovery. Automatic handoff preflight/release fallback still has a separate zero-byte Firefox pause path that can attempt an unsupported resume. That path needs a real native-refusal/unavailable-host test and a lossless acquisition/fallback design; this release does not claim it fixed. Keep it as a P0 follow-up rather than inferring general recovery reliability from the manual tests.

The earlier HLS audio-only timeout remains unexplained; its original failure is `D:\UDM\.media-cache\paired-edge-hls`, distinct from the older HLS codec-assertion failure in `hls-catalog-edge-1`. Wider media/player/ad association, transports, automatic driver capture, COM activation, full GUI/accessibility, physical recovery, speed comparison and signed lifecycle remain open.

Installer: `D:\UDM\installer-out\UDM-0.78.0-Browser-0.52.2-Setup-x64.exe`. Backup: `D:\UDM\backups\browser-0.52.2-20260930`. Verified source archive: `D:\UDM-Workspace\candidates\native-0.78-browser-0.52.2-firefox-resume`.

[Deployment and installer hash](evidence-browser-0.52.2/deployment-verification.json) · [Firefox acceptance](evidence-browser-0.52.2/firefox-toolbar-fixed-0522-20260930/results.json) · [Current gaps](idm-research-current-status-2026-09-29.md)
