# UDM 0.78.0 / browser 0.52.0

Follow-up: browser **0.52.1** is now installed on disk, correcting the Firefox Downloads button and adding real Firefox recovery-page acceptance. See the [hotfix report](parity-browser-0.52.1.md). The original release evidence below is retained.

This paired release adds review for uncertain downloads left by older browser integrations and fixes Chromium/Firefox identification during canceled-download handling. Installed 30 September 2026: all 40 deployed files and the running native connection verified. The personal catalog remains byte-identical with 25 records. Edge 0.52 activation is pending because computer control was stopped with Escape; the last verified personal Edge version is 0.51. The deployment record distinguishes installed files from browser activation. Full IDM parity remains incomplete.

## Behavior

The extension's **Recover interrupted downloads** button opens a review page for older uncertain handoffs. Each entry identifies the browser's current filename, host and start time without storing its URL or request body in the recovery journal. The user can inspect UDM and browser Downloads, then explicitly choose to resume in the browser, keep the UDM copy and cancel the browser copy, or clear a notice after handling both copies. A failed choice remains durable and can be retried after restart. Dismissal changes neither transfer.

For token-bearing older captures, native UDM writes a release barrier before a browser resume. A delayed Add with that token is rejected. An accepted native receipt takes precedence over an uncertain browser record. The operation never guesses which native job to delete or change. Tokenless older captures require the user to check both lists; there is no native receipt from which UDM can prove ownership.

Downloads that the browser cannot resume are identified as such. Their original requests are not reconstructed or replayed. The review directs the user to browser Downloads for manual handling. Protocol-2 protected captured links retain their existing native review workflow.

## Edge identification correction

The installed Edge 154 runtime exposes both `browser` and `chrome` extension namespaces. The old test for the presence of `browser` wrongly classified a zero-byte, user-canceled Edge response as Firefox's special pause behavior. Engine-specific handling now checks the extension URL scheme (`moz-extension:` versus `chrome-extension:`). The browser Downloads button uses the same distinction. A real isolated probe and regression cases retain this finding.

## Validation

- 2,403 native checks passed, including 68 focused receipt/transaction checks.
- All 45 browser/native-host test scripts passed. The changed recovery, transaction, automatic POST-capture and background paths were rerun after the engine correction; 29 checks focus on manual legacy recovery.
- Two final isolated Edge runs passed 17 checks using the actual popup and review page. They exercise normal resume/cancel/dismiss and canceled-on-restart handling, exact output hashing, the native release barrier, rejected webpage access, and reopening the settled page.
- The final transfer tests requested each of their three fixture URLs once. The normal resume completed the original response with its expected SHA-256; UDM created no native download in these seeded uncertain fixtures.
- Temporary native-host registrations were removed. The personal catalog was checked against its pre-update SHA-256. Deployment verification records the installed-file count and the personal Edge connection result.

The first restart test assumed that closing the isolated browser preserved resumable responses. Edge canceled them instead. A subsequent test exposed the API-namespace misclassification; the final tests cover the observed canceled state explicitly. This is not proof of all physical crash, suspend or shutdown scenarios. Firefox shares the prepared code and has unit coverage, but its new recovery page has not received real Firefox acceptance in this release.

## Preservation and remaining work

The previous release is backed up under `D:\UDM\backups\release-0.78.0-20260930`. The verified source archive and manifests are under `D:\UDM-Workspace\candidates\native-0.78-browser-0.52-legacy-recovery`. Build, test and installer output stay on D:. The driver/network runtime is unchanged.

The earlier intermittent HLS audio-only timeout remains unexplained. Wider media/player association, transport and proxy modes, automatic driver capture, automation/Explorer integration, backup interoperability, full GUI/accessibility coverage, physical recovery, speed qualification and signed release lifecycle remain open. This release does not establish an IDM-parity percentage.

[Deployment verification](evidence-0.78.0/deployment-verification.json) · [Acceptance snapshot](evidence-0.78.0/acceptance-snapshot.json) · [Current gap register](idm-research-current-status-2026-09-29.md)
