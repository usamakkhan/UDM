# Recover an adaptive selection before reopening its playlist

Chromium and Firefox now look for a saved desktop receipt before rebuilding an HLS or DASH selection. The retry key is a hash of the exact retained catalog choice, its player and document identity, and the requested audio, subtitles, and output. The existing media plan fingerprint still guards submission. The retry key adds no plaintext playlist address or credentials to the receipt.

The HLS fault fixture loses the native reply and makes the playlist unavailable. It recreates the extension worker with the same browser storage, then confirms that clicking the same MP4 option recovers the original job without another playlist fetch or native submission. Clicking the TS option follows its own selection path. Both browser variants passed as part of the 59-check HLS site suite.

The native media receipt suite passed all 59 checks, including termination before and after catalog commit. The isolated Edge and 5.90 desktop HLS run passed 25 checks. It verified complete MP4, TS and M4A output and checked that the saved audio and captions matched the selections.

An isolated Edge fault run dropped a real native acknowledgement, then stopped the extension worker through DevTools. The desktop had completed exactly one HLS file, which matched its recorded SHA-256 and decoded to the end. After the origin started rejecting playlist requests, a click on the same panel choice confirmed the original handoff. The origin saw no additional playlist requests and the desktop still held one job. This live fault run covered Edge. Firefox restart coverage used the simulated worker fixture.

The other focused browser suites passed receipt error handling, integration parity, capture transactions and transport handoff. Six DASH suites passed. The Period-duration and external-index suites failed on both the frozen 5.89 browser build and the current build.

The test runner and logs are in the Windows local temporary folder under `udm-site-media-retry-5.90`.

[Production build receipt](site-media-retry-5.90-build.json)
