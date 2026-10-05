# Candidate browser 0.49.0 — file-form capture in Edge and Firefox

**Staged, not installed.** Installed UDM remains native 0.74.0 / browser 0.47.1. This combined candidate includes the pending 0.48 recognition work; recognition acceptance still gates deployment. Temporary certificate cleanup is complete.

Trusted UTF-8 download forms can now contain bounded File entries or appended Blobs. The extension preserves bytes, entry order, duplicate fields, filenames, MIME types and the original multipart boundary, and verifies the browser's observed request before handing it to UDM. The native transfer retains encrypted POST storage, non-range replay and browser cancellation after ownership.

## Actual Firefox failures fixed

The first Firefox run reached native UDM but left the file in Firefox. The browser log showed that FormData.entries() was not iterable in the content-script environment. A diagnostic fixture then showed valid File objects and ArrayBuffers, but typed-array subarray access failed with “Permission denied to access property constructor.” These were real browser failures missed by the earlier shared-module unit tests.

The observer now uses the pristine FormData forEach method to collect entries, and the binary encoder uses indexed reads. Neither fix unwraps page objects or reads filesystem paths. Mozilla documents Firefox's [content-script wrappers](https://developer.mozilla.org/en-US/docs/Mozilla/Add-ons/WebExtensions/Content_scripts); the specific failures and recovery are established by the retained local browser logs.

Actual traces also showed that Firefox preserves escaped filenames in its request dictionary, while both browsers decode form field names. Matching now applies that distinction. A separate escaped-text scenario caught and corrected an intermediate implementation that incorrectly treated Firefox field names like filenames.

Initial/final snapshots still detect ambiguous form mutation. Observations remain tied to the request, tab, frame and document; unsupported or indistinguishable requests stay in the browser.

## Verification

**1,119 nonduplicated checks pass:** 1,067 component/native-protocol checks across 40 scripts, plus 26 actual Edge/native and 26 actual Firefox/native assertions. The Firefox fixture ran Firefox 156.0.1. Repeated focused tests, prior Edge results and unsuccessful attempts are excluded from this total; it is not a parity score.

Each browser ran 13 scenarios. Ten positive cases preserved every original POST byte and produced the expected SHA-256 output: binary file, literal-percent/Unicode filename, 960 KiB file, disk input, multiple files, empty file, appended Blob, quoted/control-character filename, escaped text fields and same-origin 307 redirect. Three negative cases—ambiguous mutation, oversized file and non-UTF-8 submission—completed in the browser with no native replay.

The checks also verify one persistent native connection, no file bodies in extension storage or plaintext history, and removal of temporary native-host registrations. Edge exercises in-memory and disk inputs; Firefox uses WebDriver file selection plus real page-generated File/Blob cases. The initial sandbox attempt failed before registry registration, and three subprocess-dependent unit scripts initially received EPERM; authorized reruns passed. All failure logs are retained.

The Firefox generator reproduces the tested bundles byte-for-byte. Installed application, browser and network hashes still match; all 25 personal download records, queues and preferences remain byte-identical. Of 101 deployment-receipt files, 100 match the original receipt; the sole changed file is the research status report, verified against the prior audit's publication receipt.

[Summary](evidence-browser-0.49.0-firefox/summary.json), [source changes](evidence-browser-0.49.0-firefox/source-review.diff), [Edge acceptance](evidence-browser-0.49.0-firefox/candidate-edge-final/results.json), [Firefox acceptance](evidence-browser-0.49.0-firefox/candidate-firefox-9/results.json), [Firefox wrapper diagnostic](evidence-browser-0.49.0-firefox/candidate-firefox-5/diagnostics.json), [field-name diagnostic](evidence-browser-0.49.0-firefox/candidate-firefox-8/diagnostics.json).

## Remaining scope

The 1 MiB total native POST limit remains. Larger/non-UTF-8/custom submissions without a trusted form snapshot, single-use response adoption, general byte-signature recognition and automatic driver handoff remain open. This local acceptance does not establish all public sites, personal browser activation, speed parity or complete IDM parity.

Recognition acceptance remains the deployment gate. The HTTPS helper was stopped after its certificate expired while consent was pending; its tests did not complete. The exact temporary certificate is verified absent. No replacement helper was started. Complete recognition acceptance before installing the combined candidate. [Cleanup evidence](evidence-candidate-0.75.0/trust-cleanup-1.json). The [earlier Edge evidence](evidence-browser-0.49.0/summary.json) is retained as historical evidence and superseded by the totals above.

The candidate installer compiled successfully; packaged native and browser inputs match their reviewed hashes. It has not been installed. [Package verification](evidence-browser-0.49.0-firefox/package-verification.json).
