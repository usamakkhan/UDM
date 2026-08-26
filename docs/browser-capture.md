# Current browser capture status

Desktop 0.6 uses capture extension 0.5.0. The original SABR/UMP implementation now passes synthetic tests, but the most recent live HD selection still reported no usable captured stream. The observations below were collected earlier on desktop 0.5 / extension 0.4.1 using local diagnostics. No external URL extractor runs in this workflow.

## Live evidence

On the public watch page with video ID `Q3TI27IN7X0`:

- The original document-start observer was present. Both its early catalog and the current player response matched the page's video ID.
- The player listed 1080p, 720p, 480p, 360p, 240p and 144p. The menu no longer invents 2K/4K/8K choices.
- Progressive MP4 format 18 (360p, H.264/AAC) exposed a URL. A later live test handed it to UDM and displayed File Info, but the transfer failed before receiving bytes. Follow-up GET header probes with and without a Range header both returned HTTP 403. [Sanitized attempt record](udm-capture-360p-attempt.json)
- Adaptive video formats 137/248/399, 136/247/398, 135/244/397, 134/243/396, 133/242/395 and 160/278/394, and audio formats 140/249/250/251, contained neither direct URLs nor signature-cipher URLs.
- The last twelve observed playback resource entries used `/videoplayback` with a `sabr` query key and no `itag` or `mime` parameter. No signed URL values, cookies or authorization headers are included in this report.
- The attempted 1080p selection reported that the quality exists but a downloadable stream has not been captured. It created no desktop download and invoked no yt-dlp fallback.

This established the need for a SABR transport path, rather than proving that the observer started too late. It does not establish IDM's exact internal SABR implementation.

The 360p rejection is a second limitation: a URL present in player metadata is not necessarily accepted for an independent download. The test used fresh capture, did not invoke an extractor, and did not publish a file. Its 0.31-second failure time is not a download-speed result. The exact server-side rejection reason was not established. UDM now preserves the HTTP code in the media failure message.

## Supported path

The extension validates current-page identity, excludes ad/stale catalogs, validates stream host/path/format/expiry, and pairs usable video with default AAC audio or a muxed progressive stream. Validated directly addressable URLs, or a bounded captured SABR session when available, are handed to the native application. UDM downloads those bytes itself, merges separate tracks locally, and verifies the requested dimensions before publishing output.

Current logic supports directly addressable MP4/WebM streams and an experimental original SABR/UMP implementation. See [implementation and limits](sabr.md). Synthetic tests do not establish live service compatibility. Changing signed format parameters, stripping a transport query key, or saving a framed response as `.mp4` is not an implementation of that protocol.

## Validation boundaries

The 26 browser, four early-capture and six UMP/capture checks exercise controlled fixtures. The 99 C# checks exercise local transfer, storage, native handoff, media and network-monitor fixtures, including HTTP rejection of captured media. They do not prove the current live adaptive YouTube flow.

Earlier 1080p output and speed results in the benchmark reports belong to the older resolver-based build. They must not be presented as performance results for capture extension 0.4.1 or desktop 0.5. No current 1080p speed comparison is available.

`browser/chromium/diagnostics.html` displays sanitized metadata locally for diagnosis; it does not upload a report. Do not export local state/history or raw media URLs when sharing this project.
