# Current browser capture status

Native desktop and extension 0.16.0 add a browser-level request observer alongside the page observer. A real Chrome worker fixture verifies capture when the page observer sees no requests. Direct MP4 and HLS handoffs pass real Chrome/native integration tests. The isolated live YouTube attempt encountered Google's unusual-traffic challenge before playback, so successful live YouTube capture and completion remain unverified. See [current implementation and evidence](video-capture-0.16.0.md).

The observations below are historical, collected on desktop 0.5 / extension 0.4.1. They must not be treated as new 0.16.0 test results. No external URL extractor runs in the current workflow.

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

The historical 26 browser, four early-capture, six UMP/capture and 99 C# checks describe the earlier build. Current 0.16.0 evidence is in the linked release report; the production implementation is C++/MFC. Neither fixture generation establishes current live adaptive YouTube compatibility.

Earlier 1080p output and speed results in the benchmark reports belong to the older resolver-based build. They must not be presented as performance results for capture extension 0.4.1 or desktop 0.5. No current 1080p speed comparison is available.

`browser/chromium/diagnostics.html` displays sanitized metadata locally for diagnosis; it does not upload a report. Do not export local state/history or raw media URLs when sharing this project.
