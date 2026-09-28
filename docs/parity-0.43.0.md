# UDM 0.43.0 / browser 0.36.0 verification

This release implements a native audio-only path for captured YouTube SABR sessions. It removes the previous structural requirement to download a video track alongside streaming audio. It does not establish 99% or full IDM parity.

## Changes

- The browser offers AAC track/bitrate choices with complete stream identities and sends only the selected audio identity. Existing direct audio URLs still use ordinary parallel file downloads. The streaming route requires the desktop's `sabr-audio` capability.
- The native streaming engine supports one or two tracks, independent timeline windows, bounded concurrency, overlap verification, cancellation, single-connection fallback when seeking is unsupported, and audio-only progress aggregation.
- Audio jobs preserve their captured session in encrypted storage, distinguish same-itag languages/revisions, display an audio streaming endpoint in Properties, and publish M4A only after FFprobe confirms exactly one AAC track and no video. Failed assembly preserves source data.
- No video track or yt-dlp lookup is requested by this audio-only route. Network driver and Test Mode settings are unchanged.

## Fresh verification

- 887 native checks passed, including parallel audio streaming, independent cookies, overlap integrity, cancellation, seek fallback, job persistence, progress, language/revision identity and AAC/M4A assembly. A sandbox run stopped at state-file replacement; the full suite passed outside the sandbox.
- 8 real native-host framing checks passed against 0.43.0.
- 559 browser checks passed across 23 suites, including 25 new audio-streaming checks. The packaging subprocess test needed an outside-sandbox rerun.
- Chrome and Edge each passed 12 real UI/direct-audio fixture checks: selector defaults, language selection, Back, native handoff, overlapping range requests, output hashes and decoding. These fixtures use generated audio and an explicit loopback URL substitution; they are not YouTube CDN evidence.

## Public YouTube result and remaining gap

An isolated Edge profile detected an AAC streaming track on public video `Q3TI27IN7X0`. The new desktop accepted the first handoff in 3.319 seconds and a second run in 1.346 seconds. Both downloads were refused by the media server with a browser-attestation requirement. A video control reused the exact captured session body and audio identity and was refused with the same error, receiving zero bytes. No live M4A completion or speed improvement is claimed.

The next compatibility task is to investigate the legitimate browser-session requirements behind that refusal. This release does not bypass the server response or label a rejected download as completed. Firefox sources are synchronized, but this release has no fresh Firefox UI or persistent-install acceptance run.

The 103-workflow inventory remains 88 implemented, 12 partial, 2 unverified and 1 user-deferred English-only scope item. Implemented means code exists, not complete IDM-equivalence acceptance. Full GUI, site coverage, performance and distribution parity remain unestablished.

## Package and installation

[UDM-0.43.0-Browser-0.36.0-Setup-x64.exe](../installer-out/UDM-0.43.0-Browser-0.36.0-Setup-x64.exe) was compiled and installed on the current PC. SHA-256: `48C2998FA503D59B85125D871633227C4576087C29DEFDCEAB2E46DD72347E2A`. All 40 deployed files were hash-verified, the signed network runtime is unchanged, and all 23 download records remain byte-identical.

Native 0.43.0 is running on the correct user catalog, and the existing Edge extension is enabled at 0.36.0 with a successful desktop connection. Chrome's isolated fixture passed, but current-profile activation could not be observed because launching Chrome exposed no targetable window. Firefox shared sources are updated; persistent activation is not claimed. See [deployment evidence](evidence-0.43.0/deployment.json). Original source/binaries and prior installers are retained for rollback. See [test evidence](evidence-0.43.0/summary.json) and [current comparison](idm-parity-0.43.0.md).
