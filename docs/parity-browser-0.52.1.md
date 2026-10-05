# UDM browser 0.52.1: Firefox recovery correction

Follow-up: browser **0.52.2** is installed on disk. It corrects manual resume capability and qualifies the real Firefox toolbar-popup workflow; see the [new report](parity-browser-0.52.2.md). Original 0.52.1 evidence follows.

Browser 0.52.1 files are installed beside native UDM 0.78.0. The installer and source archive are verified. Existing personal browser sessions have not been reloaded or qualified; the last observed personal Edge version remains 0.51. Full IDM parity remains incomplete.

## Fixed behavior

Firefox rejected the previous recovery page's “Open browser downloads” button with **Illegal URL: about:downloads**. Browser 0.52.1 replaces that operation with **Show Firefox downloads**: an expandable list read from Firefox's real download history, including filename, saved path, host, start time, status and bytes received. It shows up to 100 recent records, supports refresh and identifies Firefox's Downloads keyboard shortcut. Reading this list changes no transfer or recovery decision. Edge and Chrome retain their native Downloads-page action.

The full Firefox Downloads window still requires its normal browser command (Ctrl+J on Windows). The inline list does not open that window and is not a replacement for all of its controls. Mozilla tracks the missing extension API in [bug 1298215](https://bugzilla.mozilla.org/show_bug.cgi?id=1298215); the available [Downloads API](https://developer.mozilla.org/en-US/docs/Mozilla/Add-ons/WebExtensions/API/downloads) supplies the history used here.

## Acceptance

- 300 targeted checks passed across recovery, ownership transactions, file recognition and browser integration.
- Nine actual Firefox 156.0.1 checks passed in a disposable profile: isolated native connection, engine identity, recovery-page entries, explicit resume/cancel/dismiss, reopening, history display and refresh. The resumed output matched its expected SHA-256. A browser range request occurred only after the native release barrier. Listing history preserved all download states and made no transfer request.
- Nine actual Edge checks passed in a disposable profile, including its real toolbar popup, recovery actions, sender boundary, reopened page and native Downloads button.
- All 17 changed files were backed up and verified after deployment. All 49 native/package runtime files remained byte-identical, as did all 25 personal download records. Temporary test native-host registrations were removed. No certificate, driver or system-network setting changed.

Firefox recovery-page testing navigates directly to the extension page. It does not qualify Firefox's real toolbar-popup entry point. Opening popup.html as a normal tab was rejected by its existing sender guard; that failed fixture was retained and is not classified as a toolbar defect. The initial timing fixture also missed an already completed download; the corrected fixture pauses at the actual download-created event. WebDriver's documented system-access option was required only in the disposable test profile to automate extension pages. All failed runs remain in the evidence folder.

## Artifacts and remaining work

The installer is `D:\UDM\installer-out\UDM-0.78.0-Browser-0.52.1-Setup-x64.exe`. Its SHA-256 is `0afd778064c190ec484cf4b05e27c99f6fb397a1353e54044b9439b7ca905ca3`. Previous files are under `D:\UDM\backups\browser-0.52.1-20260930`; the verified browser-source archive is under `D:\UDM-Workspace\candidates\native-0.78-browser-0.52.1-firefox-recovery`.

Personal extension activation, the Firefox toolbar entry, the earlier intermittent HLS timeout, wider real-site video/player association, transport/driver integration, COM activation, GUI and accessibility coverage, physical recovery, matched-route speed tests and signed release lifecycle remain open. This correction does not establish a parity percentage. The native 2,403-check result belongs to the unchanged 0.78 release; it was not rerun for this browser-only change.

[Deployment verification](evidence-browser-0.52.1/deployment-verification.json) · [Firefox acceptance](evidence-browser-0.52.1/legacy-review-firefox-final-0521-20260930/results.json) · [Edge acceptance](evidence-browser-0.52.1/legacy-review-edge-0521-20260930/results.json) · [Current gap register](idm-research-current-status-2026-09-29.md)
