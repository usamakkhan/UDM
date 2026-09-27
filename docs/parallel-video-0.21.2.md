# UDM 0.21.2: parallel YouTube downloads

25 September 2026 local time (26 September UTC). Native 0.21.2 is installed. Browser integration remains 0.20.3.

## What was wrong

The captured YouTube SABR path in 0.21.1 used one sequential request loop for both tracks. Its two progress rows represented audio and video, not two network connections. Direct video URLs already use parallel range transfers; HLS/DASH already has a segment worker pool. This change addresses the sequential YouTube path.

## What changed

- Up to eight independent timeline workers, bounded by the download's connection preference. Clips use at most one worker per 30 seconds. One-connection preference remains supported.
- Each worker owns its request state, redirects, cookies, contexts, pending media and temporary files. Shared rate limits and cancellation apply across all workers.
- Each response is checked for current-video identity, selected format and declared segment size. Each worker verifies continuity. Final assembly orders by segment sequence, checks the entire timeline, verifies initialization equality and compares overlapping segment hashes before removing duplicate media.
- The progress dialog displays actual connection workers, transferred bytes and a map of their audio/video coverage. Per-track rows are not presented as separate connections. Completion still waits for verified assembly.
- If a server explicitly ignores a nonzero timeline seek, UDM stops and joins the parallel workers before trying once with one connection. Identity, conflicting-content, storage, encrypted-media and attestation failures are not silently retried as success.
- No new driver, yt-dlp dependency, IDM binary patch or licensing change was introduced.

Protocol field meanings were checked against the primary project schemas for [playback requests](https://github.com/LuanRT/googlevideo/blob/main/protos/video_streaming/video_playback_abr_request.proto) and [buffered ranges](https://github.com/LuanRT/googlevideo/blob/main/protos/video_streaming/buffered_range.proto). The implementation is original UDM C++.

## Verification

**376 native checks passed, zero failed.** Four/eight response lifetimes overlap in protocol tests; independent cookies and nonzero buffered ranges are exercised across multiple requests. Checks verify exact audio/video bytes after out-of-order worker completion and overlap removal, one-connection equivalence, the eight-request cap, rejection of inconsistent overlap, cancellation of every worker, and the server-seek fallback. Existing HTTP, HLS/DASH, media-storage, queue and browser-host checks also pass.

All three installed binary hashes match the tested build. All 19 existing download records retain their download data, including status, destinations, checksums and paused ISO received-byte counts. Startup normalized one `ConfirmationPending` UI field from absent to false on the previously completed video; every other per-download value is unchanged. The C: media temporary-folder preference is unchanged. Source, binary and history rollback copies remain in the C: staging directory `parallel-video-0.21.2`. The installed native-host ping passed after restart.

## Live test limits

The same public video `BewnzhHlQuk` was tested at 1080p using isolated Chrome profiles and separate app histories. Windows desktop capture failed with `0x80070057`, and input failed with `GetCursorPos: Access is denied (0x80070005)`, preventing the regular Edge session test. The installed idle app was restarted through guarded process management with its persisted history backed up.

| Run | Payload bytes received | Maximum in-flight requests | Outcome |
| --- | ---: | ---: | --- |
| Initial 8 connections | 0 | 8 | The streaming server requires browser attestation. Refresh playback in the browser. |
| 1 connection after playback | 10,712,377 | 1 | The streaming server requires browser attestation. Refresh playback in the browser. |
| 8 connections after playback | 0 | 8 | The streaming server requires browser attestation. Refresh playback in the browser. |

The one-connection control also hit YouTube's attestation requirement. These runs do not establish a successful parallel YouTube completion, media hash equivalence to IDM, or any speed improvement. A fresh capture from the regular working Edge session and a complete file comparison remain necessary. The previous 0.21.1 completed-file comparison still applies only to that earlier serial build.

Evidence: [test log](../benchmarks/parallel-video-20260925/final-build.log), [deployment hashes](../benchmarks/parallel-video-20260925/deployment.json), [history validation](../benchmarks/parallel-video-20260925/validation.json), [one-connection control](../benchmarks/parallel-video-20260925/live-single-control.json), [parallel control](../benchmarks/parallel-video-20260925/live-parallel-control.json).
