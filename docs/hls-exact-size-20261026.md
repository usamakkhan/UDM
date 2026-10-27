# Recorded HLS exact final size

Recorded adaptive downloads can now use the existing File Info prefetch setting.
UDM downloads the selected segments, assembles the requested container, checks the
selected streams and hashes the output in the background. File Info then shows
the actual assembled file length as **Exact final size**, while retaining the
file privately until Start. Confirmation publishes those same verified bytes;
changing the destination folder does not require downloading the segments again.

This is not a metadata-only size prediction. Until assembly finishes, the dialog
shows **Preparing...**. Start remains available during preparation. Live streams
are excluded because they do not have a fixed final length. Cancelling clears the
size claim and does not publish a final file.

## Validation

The `--adaptive-prefetch-checks` native test entry runs existing media checks and
adaptive checks, including new MP4/TS pre-Start size comparisons, destination
changes, cancellation and early confirmation. The native test build succeeded;
the targeted suite passed **212 checks, 0 failures**. MP4 and TS both matched the
pre-Start size without another HTTP request after confirmation. The suite also
checked real MPEG-TS output and full decoding, M4A, selected audio/subtitle
metadata, collision protection, hash failures and adaptive recapture recovery.

The desktop build was installed and its SHA-256 verified against the tested
candidate (`4F3F8FBAB5CC780034663A3C6FBE14CC2EC52D4680489E9B57DEF37067215C07`).
The installed Chrome workflow passed on the public NASA post
<https://x.com/NASA/status/2113776971603689874>. The selected entry was
`x.com MP4 1080p HD 6289 kbps Audio` (HLS, not Original file).

Before Start, File Info displayed **20.19 MB**, **Exact final size**, and the
expanded text **Exact final file: 21169896 bytes. Prefetched and verified;
waiting for confirmation.** Confirmation was pending and the destination file
did not yet exist. After clicking Start, UDM reported Complete with
`ReusedPrefetchedOutput=true`. The destination contained **21,169,896 bytes**:
**zero-byte difference** from the displayed exact size.

FFprobe confirmed H.264 1920×1080 video (59.893167 seconds) and AAC audio
(59.935833 seconds). A full `ffmpeg -v error -xerror` decode of both streams
exited 0 with an empty error log. SHA-256:
`55166D3C8AA666A0E784A8C6430830E7E729C369649F03A5DF780D53458136DA`.
This also matches the earlier complete HLS reference file.

Sanitized results: [validation/hls-exact-size-20261026.json](validation/hls-exact-size-20261026.json).

## Additional installed-browser quality matrix

All remaining qualities and TS options were selected individually in Chrome.
Each expanded File Info dialog displayed the exact bytes before Start; the
destination did not exist until confirmation. Every output reused the prefetch,
matched the displayed bytes, contained the selected resolution with audio and
the complete roughly 60-second duration, and passed full audio/video decoding
with `ffmpeg -v error -xerror` (exit 0). TS files were verified as MPEG-TS.

| Quality | MP4 before = final bytes | TS before = final bytes |
| --- | ---: | ---: |
| 1080p | 21,169,896 | 22,095,264 |
| 720p | 10,270,181 | 10,960,588 |
| 360p | 3,629,235 | 4,152,732 |
| 270p | 1,262,198 | 1,742,196 |

All eight combinations passed (seven additional tests plus the original 1080p
MP4 test). A repeated 1080p MP4 selection was cancelled and excluded. Stale
extension menu choices were refreshed through the visible Refresh button before
continuing; no original-file option was substituted for HLS.

Per-file hashes, probe results and comparison records:
[validation/hls-quality-matrix-20261026.json](validation/hls-quality-matrix-20261026.json).
