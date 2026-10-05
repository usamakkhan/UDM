# UDM browser 0.53.0: Firefox handoff without destructive preflight pause

Follow-up: browser **0.53.1** adds bounded cross-site video preparation and guards against late native submission. [Evidence and remaining limits](parity-browser-0.53.1.md). Original 0.53.0 evidence follows.

Native UDM 0.78.0 and browser 0.53.0 are installed on disk. Personal browser activation remains unverified after desktop control was stopped. Full IDM parity is not established.

## Confirmed problem and correction

Firefox implements pause by stopping the original response. Without partial data it may not be resumable; even with partial data, resume can issue another request. This is not a lossless fallback for a single-use download. The earlier 0.52.2 release corrected manual resume offers but left this automatic path open.

The real 0.52.2 baseline made two GET requests after native capture was disabled. With the same server changed to reject a second attempt, the original 2,097,171-byte payload was lost and the browser file checked as empty. The failed assertions, network request records and pause/resume observations are retained beside the successful runs. This live baseline exercised a partial-data pause; the separate earlier toolbar evidence established the zero-byte non-resumable state.

Firefox now keeps its original response running through desktop availability, capture-policy and request-context checks. A compatible desktop saves the protected request before UDM cancels the browser response. The extension verifies cancellation, persists its proof and only then commits the native download. Its journal records whether UDM ever paused the response, so recovery cannot resume a user-paused or user-canceled download when releasing a running handoff.

If UDM is unavailable, declines capture or rejects preparation, the original response remains usable without pause/resume or another request. If that response finishes while native preparation is pending, UDM releases the preparation without creating a second download. Firefox with an older desktop lacking the transaction protocol keeps the browser download and explains that an update is needed. The existing Chromium pause/ownership path is retained.

## Validation

- 386 targeted checks passed: transaction/restart/storage races (81), legacy recovery (33), ownership recovery (27), automatic POST capture (40), file recognition (143), browser behavior (54), and capture exclusions (8). Older tests that load Firefox files behind a Chromium-shaped mock do not prove Firefox behavior; the actual-browser evidence below covers the new path.
- 37 actual Firefox 156.0.1 checks passed across 15 downloads: disabled native capture and a genuinely missing host with single-use GET/POST responses, accepted GET/POST, real native prepare rejection, lost prepare/commit replies, browser completion during preparation, two actual background reloads, binary multipart forms and a 307 redirect. Browser fallbacks each used one original request; accepted POST requests preserved their exact body/content type; published files matched SHA-256.
- During the two real Firefox reloads, preparation survived while the browser finished, and committed ownership survived a missing acknowledgement. A fresh background generation was observed, journals cleared, and neither request was replayed by recovery.
- Ten actual Edge regression checks passed across binary upload, 307 redirect and an unsupported mutated form. Cancellation preceded native commitment and output/request hashes matched.
- Fault observers live only in disposable test extensions. Native rejection uses an invalid prepare input sent to the actual native implementation. Reply-loss cases discard a real acknowledgement; native and browser responses are not fabricated. Test folders have private profiles/catalogs and temporary native-host names.
- All 18 changed payload/support files are backed up and verified. All 49 native/runtime package files remain unchanged. The entire personal catalog is byte-identical, including all 25 downloads. Eight temporary test registrations were checked absent, and no isolated Firefox/Edge/UDM test processes remained.

The combined installer is `D:\UDM\installer-out\UDM-0.78.0-Browser-0.53.0-Setup-x64.exe`. SHA-256 and the deployment receipt are in [verification](evidence-browser-0.53.0/deployment-verification.json). Source archive: `D:\UDM-Workspace\candidates\native-0.78-browser-0.53.0-firefox-handoff`. Backup: `D:\UDM\backups\browser-0.53.0-20260930`.

## Remaining scope

This closes the tested fallback defect for newly captured Firefox downloads. It cannot restore response bytes already discarded by an older extension. Older ambiguous/canceled journals still need their existing recovery workflow and broader migration coverage.

Successful native capture still makes a new network request; adopting an already-issued single-use response without replay remains a separate transport gap. Tiny files may finish in Firefox before native preparation; they remain browser-owned. No IDM speed, transport, driver or full GUI equivalence is inferred from these tests.

The earlier HLS audio-only timeout, wider real-site/player/ad association, automatic driver capture, COM cold activation, full GUI/accessibility, physical recovery and signed distribution remain open in the [gap register](idm-research-current-status-2026-09-29.md).
