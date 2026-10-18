# Media size before downloading

The Chromium and Firefox YouTube handoff now carries the total size of the selected video and audio tracks when both lengths are available. A verified native player lookup can supply the same metadata. Streaming audio offers also carry their known track size.

Download File Info displays this value with a `~` prefix before Start Download. More explains that combining the tracks can change the final file size. The value is stored as `EstimatedMediaSize`, independently of `Size`, segments and received bytes; it never controls transfer bounds or resuming. An actual known size takes precedence, including an empty file. Unknown lengths remain unknown rather than displaying a partial video-only total. Session refresh replaces or clears the hint.

Native validation ignores non-integer, negative, zero and out-of-range hints. Older extensions without this optional field remain compatible. Ordinary direct downloads retain their existing metadata lookup.

Validation: 32 transport handoff checks passed for each browser variant, including combined lengths, missing audio length, changed metadata, and native player sizes. The 56 browser checks and 42 YouTube audio checks also passed, along with 27 streaming-audio checks per browser variant. The desktop AppOnly build succeeded. All 16 `MediaSizeTests` checks passed, covering desktop formatting, precedence, malformed metadata, save/reopen persistence and separation from transfer accounting. Sanitized results are saved in [validation/media-size-preview-20261016.json](validation/media-size-preview-20261016.json).

This change requires both the updated desktop executable and an extension reload. Live YouTube display and completion have not yet been verified with this build. Sources that do not expose a length still cannot provide a size before downloading.

## Live browser follow-up

The installed build was tested in the user browser. The first 720p SABR offer displayed Unknown before starting. The extension incorrectly required a ready direct audio URL to include audio length in a streaming size total. Extension 0.63.9 corrects this in Chromium and Firefox, selecting the streaming audio metadata consistently with the SABR handoff. A new regression case passes; live retest requires reloading 0.63.9. No file download was started during this check.

Live retest passed after deploying and reloading the installed 0.63.9 extension: a fresh 720p selection of Building my DREAM home network FROM SCRATCH displayed ~ 107.50 MB in Download File Info before Start Download. The catalog showed Awaiting confirmation, and the status bar showed zero active downloads. The dialog was left open; no download was started.


## Reported lengths: 2026-10-17

REA 6.5.0 with Ghidra 12.1.3 analyzed the installed `IDMNetMon64.dll`, SHA-256 `9840d78e94f9df6b03be2246b9814263a5ddf287489cd260a25a4d51e4a1bc5b`. Function `0x180055300` reads the URL `clen` value through an integer parser. Function `0x180055ab0` reads `contentLength`, including the nested `transcodeMetadata` object, and passes it into media-record construction at `0x180056950`. Evidence IDs: `ev_b1425ad9f97962e03e40f474c66e123204feff14620063c3fe4cea9bba2529c2` and `ev_d0a3a0cfb4f6947cd9fbe22a4275043f7609723bd38d60968d0b3e5bdf9df976`. Matching historical exports corroborate these observations. REA's separate IDMan.exe string search timed out; the network-module analyses completed. Raw proprietary decompilation stays outside tracked documentation.

This establishes sources for reported stream lengths, not an exact prediction of a remuxed output file or a complete reconstruction of IDM's combination logic. UDM independently reads the same metadata fields. Extension 0.63.10 sends `reportedSize`; the desktop stores `ReportedMediaSize`, displays it without an approximation prefix, and exposes the integer byte count in the details. Both selected tracks must have known lengths before a combined total is reported. Bitrate and duration are never used to manufacture an exact value. Unknown stays unknown. Legacy `estimatedSize` remains supported with the approximation prefix. Neither field changes transfer bounds, resume data, or the actual completed-file size.

Chromium and Firefox each passed 37 handoff checks and 27 streaming-audio checks. Browser behavior checks (56) and audio checks (42) passed. The installed Chromium and Firefox directories were updated to 0.63.10 and all 88 staged files hash-verified. The user confirmed reloading the extension. Desktop compilation and live validation are pending at this point; see the follow-up record for their final status.


### Installed and verified in Chrome, 2026-10-17

The desktop build completed and was installed with backup. Installed SHA-256: `FE6BB216CACB2E4398CEE71BE8EB0DF8D88D0CC12406E94BD10F3FFA82829D58`. All 30 native size checks passed; results are in `validation/reported-media-size-20261017.json`. The audio reader regression suite now has 43 passing checks, including nested metadata extraction.

A fresh tab in the user's existing Chrome profile captured RouterOS 7.24 changelog at 720p H.264 through the installed 0.63.10 extension. Download File Info displayed **29.32 MB** without an approximation marker. More displayed **30745042 bytes reported by the server**. Start Download remained untouched; the catalog showed Awaiting confirmation and the status bar showed zero active downloads. The dialog was left open for inspection. This verifies the reported selected-track byte total before download; it does not assert that the final combined MP4 has exactly that length. No new media download was started during this verification.


### Complete download and media validation, 2026-10-17

The previously saved capture had expired and failed before receiving any bytes. A fresh capture of the same 720p H.264 selection again showed 29.32 MB / 30,745,042 reported bytes and was started immediately. It completed successfully as a 30,821,085-byte MP4 (29.39 MB): 76,043 bytes, or 0.247334%, larger than the reported source-track total. Thus the preview is not an exact prediction of the final remuxed MP4 size.

FFmpeg decoded the entire video and audio with strict error detection, exit code 0, and no errors. FFprobe found 9,131 H.264 frames at 1280x720 and 15,731 stereo AAC packets at 44.1 kHz. Video duration was 365.240 seconds; audio duration was 365.273107 seconds. Neither track had backward decode timestamps or meaningful packet gaps (largest numeric residual below 1e-12 seconds). Audio was non-silent: mean -24.5 dB, peak -2.9 dB. Opening the resulting file in MPC-HC visibly showed playback with H.264 1280x720 and AAC 2.0. UDM displayed Complete, 29.39 MB, and zero active downloads.

Sanitized evidence: `validation/reported-media-size-completion-20261017.json`. No claim of a byte-for-byte match to a separately downloaded reference is made; the checks establish full decodability, expected duration and streams, timeline continuity, non-silent audio, and successful desktop playback.


### Exact final-size feasibility follow-up

A structural inspection of the completed test MP4 accounts for every byte: ftyp 32, moov 262486, free 8, mdat 30558559 (including its 8-byte header), total 30821085. Its moov contains sample-size, timing, composition-offset, synchronization and chunk tables. The browser's current length/duration/format capture does not include these tables. Therefore no exact final-file prediction follows from the current reported-size input. The measured 76043-byte difference is not a universal correction. See `validation/final-media-size-feasibility-20261017.json`.

FFmpeg's official MOV/MP4 documentation describes maintaining information about every packet until the file is closed: https://ffmpeg.org/ffmpeg-formats.html#Fragmentation . Metadata-only prediction could be investigated for direct files that expose complete sample tables, with strict format and muxer-version matching. The current streaming capture has no such implemented path. Downloading and assembling into a cache first would reveal the exact output length but would not satisfy "before downloading".

No exact-final-size feature was implemented or installed in this follow-up. Prior REA findings prove source-length parsing, not IDM's final-output-size prediction. Do not describe the earlier 29.32 MB preview as an exact final MP4 size.

## Complete sample metadata preflight (2026-10-17)

A fresh browser capture exposed direct 720p H.264 and AAC track URLs. A standalone VISIONOS player request had returned LOGIN_REQUIRED, so that request was not treated as a usable source. Bounded byte-range requests against the captured tracks retrieved every initialization, segment-index and fragment box while skipping media-data bodies. The two tracks required 186,491 metadata bytes. A bounded codec probe read another 422,100 bytes. Their initialization tables alone contain zero samples; the complete sample records are in the fragment headers.

The prototype builds temporary, zero-filled track placeholders with those exact headers and the bounded codec probe, then runs the installed FFmpeg muxer with packet parsing disabled for the placeholders. It verifies that all 9,131 video packets and 15,731 audio packets survive. The resulting length was 30,821,085 bytes, matching the independently completed browser download exactly. Header-only placeholders were 35 bytes short, so codec probing is required; a fixed overhead correction was not used.

The native implementation in MediaPreview.hpp is restricted to direct fragmented H.264/AAC MP4, one sample description per track, complete per-sample sizes, bounded requests and scratch space. It rejects unsupported or incomplete inputs and keeps the existing reported-track-size fallback. Calculation runs through the asynchronous DownloadPreview worker. It does not set download Size, Received, segment bounds, or resume validators. Download File Info describes a successful result as a calculated final MP4 size. This is a supported-format calculation, not a promise that every YouTube transport or codec exposes sufficient metadata. Temporary placeholders are removed when the calculation finishes or fails.

REA 6.5.0 analyzed the installed hash-pinned idmvconv.dll routine at 0x1001ea00 (evidence ev_dc0ab2bbfb27f57799259624a15181a72e114300dea3638186056afd258876da). The routine performs conversion-related calls and sends progress notifications. This does not establish IDM's pre-download prediction method. UDM's implementation is independent; proprietary pseudocode is not included in the repository.

Native build, installation, and before-download GUI validation are pending at this checkpoint. See the validation JSON and subsequent notes for final outcomes.

Native validation follow-up: the rebuilt app (SHA-256 16F3B92E4471A4261F4F772A9C757C544185F7265F7AC1DE3CAEFDE2D8C13FE0) compiled successfully. All 40 native checks passed, including the real direct-track metadata preflight and unchanged download state. The native result was 30,821,085 bytes using 608,591 network bytes. Chromium and Firefox each passed 37 handoff and 27 streaming-audio checks; Chromium also passed 43 audio and 56 browser checks. Generated prototype media placeholders were removed. Extension 0.63.11 is staged; deployment is waiting for the user to approve Windows UAC. The new native GUI has not yet been tested in the user's browser, and the installed executable is still the previous build.

Deployment outcome: Windows returned 'The operation was canceled by the user' for the elevated extension installer. Neither extension nor desktop deployment completed in this pass. No retry was launched after cancellation. The built app and hash-guarded installer remain ready for deployment.

Final installed browser verification: on the user's explicit request to retry, elevation succeeded. All 88 installed Chromium/Firefox extension files verified at 0.63.11; the user confirmed reloading. The desktop executable hash matches the tested build. A fresh 720p capture opened Download File Info showing 29.39 MB and 'Calculated final MP4: 30821085 bytes' while the job remained Awaiting confirmation, Received 0, Size -1. After Start Download, the saved file was exactly 30,821,085 bytes (difference zero), SHA-256 BC85EFBDB4CB6C3245938D9F912611B5CFA18C02B889F2A1EE05799230E93BAE. Strict full FFmpeg video/audio decode completed with exit 0 and no errors. FFprobe confirmed H.264 1280x720 and AAC audio, 365.273107 seconds total. No preview scratch directories remained. See validation/final-size-browser-20261017.json. This verifies the supported direct H.264/AAC path; unsupported transports retain the reported-size fallback.

## Additional codecs and qualities (work in progress, 2026-10-17)

A browser-captured 1440p AV1/AAC selection completed as 54,867,860 bytes. Reusing only the H.264-style initialization and sample tables incorrectly predicted 54,886,018 bytes. Inspecting every AV1 packet prefix reduced that to 54,867,776 with parsing disabled; enabling the AV1 parser produced the correct 54,867,860 bytes. Both the OBU filtering and keyframe table matter. These are prototype results against a completed browser download, not yet an installed-GUI verification.

The native extension of the calculation walks bounded AV1 OBU lengths inside every sample, copies packet prefixes and complete sequence/metadata/standalone frame-header OBUs, and enables FFmpeg's AV1 parser. It rejects malformed boundaries, excessive headers, parser errors, changed packet counts and wrong output height. The AV1 track has an 8 MiB network budget and 20,000-request ceiling; the asynchronous media lookup has a four-minute deadline and remains cancellable by Start Download. A long or unsupported source can still fall back to reported track lengths. It does not download full tracks to a hidden cache.

The FFmpeg [AV1 mux helper](https://raw.githubusercontent.com/FFmpeg/FFmpeg/master/libavformat/av1.c) filters certain OBUs, while the [AV1 parser](https://raw.githubusercontent.com/FFmpeg/FFmpeg/master/libavcodec/av1_parser.c) derives keyframe and stream metadata. UDM's range walker is independently implemented. No fixed per-frame correction is used. VP9/WebM and SABR remain outside this calculation. The current H.264 implementation is not restricted to 720p, but other resolutions still need completed browser comparisons. An attempted 1080p lookup timed out and was closed without starting that download.

Desktop build, native live validation and renewed browser tests are pending at this checkpoint. Extension 0.63.12 is staged for the updated desktop cohort.

Follow-up: the first native AV1 run passed 42 unit checks but failed to complete its live preflight within the deadline. Reusing 512-byte range windows across adjacent samples reduced the request count. The rebuilt native test passed all 44 checks, calculating 54,867,860 bytes from 3,709,240 network bytes, with 9,131 video and 15,731 audio packets. The 720p H.264 regression also passed all 44 checks and still calculated 30,821,085 bytes. Full decoding of the actual 1440p AV1/AAC file completed with exit 0 and no errors (SHA-256 BC5E735236DFC39596BDBA4B8F0A8826C5ABAB5F578F0449497DF6BD68D85587).

The tested desktop was installed with a backup and relaunched. Installed SHA-256: F88BE98EF7FA55F01C93340184129CB91B98E40FE25ABC2C87543D3C67BF19B4. All 88 extension files verified at 0.63.12, and the user confirmed reload. Chrome control continued timing out and a test navigation remained on about:blank; a browser restart was requested. Installed-dialog comparisons for additional qualities and M4A remain pending. See validation/av1-size-preview-20261017.json and the native validation records. Installation and a native calculation must not be described as a completed browser workflow test.

## Installed browser matrix completed (2026-10-17)

After Chrome restarted, the installed desktop and extension 0.63.12 completed four fresh captures of the same 365-second source. Before each Start Download, the new job had Received 0 and Size -1. The expanded dialog showed the following:

| Selection | Preview type | Preview bytes | Completed bytes | Difference |
| --- | --- | ---: | ---: | ---: |
| 1080p H.264/AAC MP4 | Calculated final MP4 | 56,731,539 | 56,731,539 | 0 |
| 1440p AV1/AAC MP4 | Calculated final MP4 | 54,867,860 | 54,867,860 | 0 |
| 360p H.264/AAC MP4 | Reported tracks, SABR | 12,091,913 | 12,164,620 | +72,707 |
| Original English AAC M4A | Reported tracks, SABR | 5,912,340 | 5,908,172 | -4,168 |

The 1080p metadata lookup took approximately 105 seconds and the AV1 lookup approximately 100 seconds in this run. These are observed timings, not performance guarantees. Both exact results appeared before Start Download. The 360p and M4A entries explicitly warned that the final file size could differ; neither counts as an exact-size pass. M4A initially requested playback to capture its audio session; playing the video and reopening the format list allowed capture to succeed.

All four downloads completed. Full strict decoding of the new 1080p, 360p and M4A outputs returned exit 0 with no errors. The new AV1 output has the identical SHA-256 as the previously fully decoded AV1 reference, so its decode validation was reused rather than repeated. FFprobe verified the selected codecs, heights and 365.273107-second duration. The browser was left paused and UDM had zero active downloads.

Sanitized results: `validation/quality-size-browser-20261017.json`. Completion screenshot: `validation/quality-size-browser-20261017.png`. Exact final-size support for SABR and VP9/WebM remains unfinished. No additional application code or extension deployment was needed for this verification pass.

## Actual IDM 360p comparison (2026-10-17)

The user selected the same source in installed IDM 6.43.10.2 and left Download File Info open. Before Start Download, IDM showed 5.89 MB. Its selected direct VISIONOS video URL was itag 134 with clen 6,179,573. After downloading, IDM's completion dialog stated 11.53 MB (12,091,913 Bytes). The actual saved MP4 was 12,031,058 bytes, 60,855 bytes smaller than that completion count. The destination edit did not take effect; the file was saved under the existing D:\IDM Downloads\Video directory, confirmed on disk.

FFprobe found H.264 640x360 plus AAC, duration 365.273107 seconds. Strict full decode passed with exit 0 and no errors. Compressed video packet SHA-256 matched UDM's previously tested 360p file, but the audio packet hash differed, so this is not a claim of identical audio payloads or solely container-overhead differences. UDM's reported track total was also 12,091,913 bytes; its SABR output was 12,164,620 bytes.

This observed IDM workflow does not show exact final-file size before downloading. It shows a video-track size initially and track-transfer total at completion. No conclusion about every IDM format or its internal size algorithm follows from one run. Sanitized evidence: validation/idm-360-size-comparison-20261017.json. Raw UI proof remains local in the temporary folder because its address field contains a signed media URL.

## Fast size display candidate (2026-10-17)

The observed IDM comparison showed that its immediate size was not an exact final MP4 prediction. UDM now keeps the reported selected-track total visible immediately and no longer launches the expensive full-sample metadata calculation when a media confirmation dialog opens. Supported direct tracks can still request it explicitly under More > Calculate final size. During that optional calculation, a known size remains visible instead of changing to Checking. Repeated calculation clicks are disabled while the worker is active.

The extension's direct-link grace period is shortened from two seconds to 350 ms before using an already usable captured stream. Without a captured stream, the existing bounded direct lookup is still awaited. Final document and format checks remain in place; a late response cannot create a second job. This limits an intentional wait, not the complete browser-to-dialog latency. Native host, page capture and desktop scheduling still contribute to that latency.

Chromium and Firefox each passed 37 handoff checks and 27 streaming-audio checks; the general browser suite passed 56 checks. The updated extension cohort is 0.63.13, installed with 88 verified files and reloaded by the user. Desktop build and new live quality comparisons are pending at this checkpoint.

UDM's existing PrefetchFileInfo setting starts ordinary file transfers before confirmation, retaining partial data without publishing a final file. Captured media is deliberately excluded in FileWorkflows.cpp. This change does not enable video prefetch; its start/cancel and publication behavior requires separate implementation and testing. The user's report that IDM downloads early has not yet been instrumented independently in this session.

## Fast size display installed and browser-tested (2026-10-17)

The full native build completed and was installed with backup. The installed executable SHA-256 is F45CAB0779F8BFE8A86824C3D408A9FBAE246B1322EDFDCB773D8793A73FCCC1. The user confirmed extension 0.63.13 reloaded.

Fresh browser captures verified immediate reported sizes for 480p H.264/AAC and 2160p AV1/AAC. The 480p selection took 13.891 seconds from browser click timestamp to job creation; the subsequent 4K selection took 1.334 seconds. These measurements include browser dispatch and native handoff, not just the shortened grace period. The cold lookup delay is still a limitation.

| Selection | Immediate track bytes | Optional calculated final bytes | Actual final bytes |
| --- | ---: | ---: | ---: |
| 480p H.264/AAC | 17,174,392 | 17,246,771 | 17,246,771 |
| 2160p AV1/AAC | 119,748,297 | Not requested | 119,815,243 |

Both dialogs displayed a known size with Received 0 before Start Download. Opening 480p did not initiate the full-sample scan. Explicitly selecting Calculate final size retained the known size while checking, disabled repeat calculation, and eventually produced an exact match. That calculation finished before Start was clicked, so cancellation during calculation was not exercised in this run. The 4K file completed without requesting the optional calculation. Both files have 365.273107-second duration and the requested video height plus AAC audio.

Sanitized machine-readable results are in validation/fast-size-browser-20261017.json; the final application screenshot is validation/fast-size-browser-20261017.jpg. Captured-video prefetch is still unfinished, and these results do not establish exact final-size support for every format.

Full strict audio/video decoding of both new files completed with exit 0 and empty error logs. The 4K AV1 decode was CPU-intensive and took several minutes with two decoder threads; this was a post-download integrity check, not download latency. All 184 browser regression checks and the full native app build passed for this candidate.

## Parallel capture preparation and media prefetch candidate (2026-10-17)

Extension 0.63.14 starts browser playback preparation while native direct-link retrieval is still pending. Previously the cold path awaited the native lookup deadline before preparing playback, adding sequential waits. The direct-link grace period remains 350 ms. An arriving direct pair or newly captured stream can satisfy preparation; the original native deadline remains a last fallback when neither becomes usable. Document changes during preparation are rejected, and a late native reply never submits another job.

Captured video and streaming audio now use the existing PrefetchFileInfo preference. The media worker can receive and verify internal tracks before confirmation, but waits before final assembly and publication. Cancel stops the worker and retains resumable data; Start stops the prefetch, applies the chosen destination, and resumes with saved tracks or segments. Unsupported adaptive/live paths, requests requiring fresh capture, duplicates, POST requests, and capacity limits retain their existing restrictions. Prefetch can consume network and temporary disk space while the dialog is open.

Source validation: 39 transport-handoff and 27 streaming-audio checks pass for each browser source, plus 56 general browser checks (188 total). The new cold-preparation fixture obtains a captured stream before an unresolved native lookup finishes; a late response produces no duplicate. Native prefetch tests cover publication gating, cancellation, reused tracks after rename, disabled preference, and actual HTTP prefetch with stalled-response cancellation. Native build and browser prefetch verification are still pending at this checkpoint.

After the user reloaded 0.63.14, a first 480p capture reached the old installed desktop in 5.015 seconds from the control-call timestamp, compared with 13.891 seconds previously. A preceding browser input timeout submitted no job. The successful input call itself took over four seconds during compilation, so this is not a clean measure of extension-only latency and requires a repeat after the build.

The new full app build passed and was installed with backup (SHA-256 F233FDC5908242EE59DF101650BA3F81CE48C80075DD9D9C75F9E71B22BECE6E). Extension 0.63.14 was installed with 88 verified files and reloaded by the user. The native media suite passed 41 checks, and the dedicated HTTP prefetch suite passed 3 checks. These prove the confirmation gate, retained tracks after cancellation, identical output after rename/resume, preference enforcement, actual pre-confirmation HTTP bytes, and stalled-response cancellation. Live browser prefetch testing is pending because browser control timed out and native control detected concurrent user input.

Follow-up native regression suites also passed: parallel streaming 29/29 and recovery 330/330, including retained segment reuse and cleanup only after publication. Total focused native checks: 403; total browser source checks: 188. Installed-browser prefetch and final timing repeats remain pending, not passed.

## Installed browser prefetch verified (2026-10-17)

After the user made Chrome available, a fresh navigation to the comparison video produced a 480p job 487 ms after the browser selection timestamp. A subsequent 720p selection produced its job in 568 ms. Both measurements are selection-to-catalog-creation timings, not instrumented window-paint timings or guarantees for every network. The browser input calls completed normally in this run.

Both captures used SABR. While Download File Info remained open, 480p received 17,174,392 track bytes and 720p received 30,745,042 track bytes. ConfirmationPending remained true, MediaPhase became Waiting for confirmation, and neither final destination existed. The expanded 720p dialog visibly stated Downloaded: 29.32 MB. Waiting for your confirmation.

Canceling 480p left the job paused, cleared ConfirmationPending, preserved both internal tracks, and did not publish a final file. Resume reused the verified tracks. For 720p, clicking Start Download directly from File Info reused its prefetched tracks. Both jobs recorded ReusedMediaTracks=true, and network byte counters were unchanged across confirmation/resume: 19,700,386 for 480p and 34,183,555 for 720p (these include transport overhead).

The completed outputs were 17,246,771 bytes (480p) and 30,821,085 bytes (720p). FFprobe verified H.264 at 854x480 and 1280x720 respectively, AAC audio, and 365.273107-second duration. Full strict decoding of both complete files passed with exit 0 and empty error logs. The browser source remained paused and UDM had zero active downloads afterward.

Evidence: validation/video-prefetch-20261017.json and validation/video-prefetch-live-20261017.jpg. This completes the previously pending browser prefetch tests. Direct HTTP prefetch was covered by native fixtures; these two live cases validate SABR. Immediate displayed sizes remain track totals rather than guaranteed final MP4 sizes. No claim of universal sub-second handoff follows from these two measurements.

## Exact size after background prefetch (2026-10-17, candidate)

Media prefetch now assembles and verifies its staging output on the worker before waiting for confirmation. File Info opens without waiting for that work. Once ready, its size changes to the measured final file size and the compact dialog labels it "Exact final size"; More shows the integer byte count. Until then, reported track totals retain their existing explanation. This supports the existing MP4 video and AAC/M4A assembly paths, including SABR, without applying container-overhead estimates.

The final destination is still unpublished before confirmation. Cancel removes the staging output, clears the exact-size claim, and keeps verified source tracks. Start or Resume can reuse those tracks and assemble for the selected destination. The confirmation wait is excluded from merge-duration accounting. This is an exact size before pressing Start, obtained using prefetch; it is not a claim that arbitrary streaming output sizes are knowable before receiving media bytes. No new WebM download support is implied.

Extension cohort 0.63.15 is installed with 88 verified files. Desktop build and live quality/format comparisons are pending at this checkpoint.

### Installed exact-size browser comparisons

The desktop build succeeded and was installed with a backup. Installed SHA-256: `5D9376D3D517BC767262644075A6F91C0CF4928F9DBCA097F1CBD5496D6729C9`. The user confirmed reloading extension 0.63.15. Chromium and Firefox each passed 39 handoff and 27 streaming-audio checks. Native checks passed 46 media and 4 HTTP-prefetch checks, including exact video/audio output equality, cancellation, rename and rejection of invalid tracks. Parallel media passed 29 checks. Recovery initially passed 329/330: its live-HLS stop/save check took 1,828 ms against a 1,500 ms threshold during concurrent decoding. A separate rerun passed all 330; the initial failure is retained in the evidence record rather than omitted.

Fresh selections from the user's Chrome YouTube tab displayed "Exact final size" before Start. More showed the integer bytes below. Final destinations did not exist while waiting for confirmation; completed files had exactly the displayed lengths, and network counters did not increase after Start.

| Selection | Transport | Before Start, bytes | Completed bytes | Difference |
| --- | --- | ---: | ---: | ---: |
| 360p H.264/AAC MP4 | Direct tracks | 12,164,620 | 12,164,620 | 0 |
| 1080p H.264/AAC MP4 | Direct tracks | 56,731,539 | 56,731,539 | 0 |
| 1440p AV1/AAC MP4 | SABR | 54,867,860 | 54,867,860 | 0 |
| Original English AAC M4A | SABR | 5,908,172 | 5,908,172 | 0 |

The exact-size work runs after job creation on the transfer worker and does not block dialog initialization. Observed selection-to-job times were 847 ms for 1080p and 562 ms for M4A. The first 360p and AV1 selections took 17,419 ms and 16,168 ms, respectively, amid browser-control timeouts and capture-session renewal. These runs do not establish consistently sub-second browser handoff. The initial M4A attempt asked for playback to capture its audio session; brief playback and reopening the list allowed the successful test. No WebM option was offered for this source, and no WebM claim is made.

FFprobe confirmed the requested streams/resolutions and 365.273107-second duration in all four files. Full strict decoding of 360p, 1080p and M4A passed with no errors; AV1 full decoding was still running at this checkpoint. Sanitized byte counts, hashes, timings and probe results are in `validation/exact-prefetch-size-20261017.json`. `validation/exact-prefetch-completed-20261017.jpg` shows the four newest completed files and zero active downloads, without exposing signed media URLs.

AV1 validation finished: full strict audio/video decoding returned exit 0 with an empty error log. All four installed browser cases now have exact pre-Start/final byte equality, unchanged network counters across Start, verified codecs/duration, and successful full decoding. The browser was left paused and UDM had zero active downloads. This completes the pending exact-size matrix; variable capture/handoff latency remains a separate limitation.

## Concurrent capture lookup (0.63.16 source candidate)

The initial captured-stream read now starts alongside native direct-link retrieval instead of after its 350 ms grace period. Direct links retain priority during that grace period. On fallback, the browser read has already been running, removing up to 350 ms of sequential waiting in this part of selection. This is an overlap bound, not a measured improvement in live dialog timing or a fix for all previously observed 16–17 second delays.

Capture errors are collected immediately and surfaced only if fallback needs them. A verified direct pair does not wait for a stalled capture read or fail because that read rejects. Existing document/format confirmation still runs before submission, and neither late lookup can submit a second job. Exact-size assembly and prefetch behavior are unchanged.

Chromium and Firefox packages are synchronized at 0.63.16. Automated validation passed 45 transport handoff and 27 streaming audio checks per browser source, plus 56 general browser checks (200 total). New cases cover overlapping lookups, speculative capture failure with and without a direct pair, stalled capture with successful direct handoff, and absence of late duplicate submission. Git whitespace validation passed. This source candidate has not been installed or timed in the user's browser; the installed and live-tested version remains 0.63.15.

Follow-up: after the grace period, native retrieval now continues racing the pending capture read. A verified pair arriving at 500 ms can proceed even if capture is stalled; an unavailable native result cannot mask valid browser capture. Both cases have regression coverage. Extension installation has been started but remains unconfirmed while Windows elevation is pending.

Installation completed: all 88 installed extension files match the staged SHA-256 records in validation/concurrent-capture-06316-20261017.json. Installed version is 0.63.16, with the prior extension files backed up. Browser reload and live timing remain pending.

### Installed 0.63.16 live follow-up

After the user confirmed reloading, the actual Chrome menu handed 360p and 1080p H.264/AAC selections to the installed desktop. Selection-to-job creation measured 1,948 ms for 360p (desktop launch included) and 501 ms for 1080p. These measure catalog creation, not window paint latency, and do not establish consistent latency across qualities or networks.

Both used SABR, displayed exact final sizes before Start, and had no published destination before confirmation. The 360p output matched 12,164,620 displayed bytes; 1080p matched 56,731,539 bytes. Both reused prefetched tracks. The recorded 1080p network counter remained 61,811,191 bytes across Start; the 360p pre-Start counter was not recorded. FFprobe verified the requested dimensions, H.264/AAC and 365.273107-second duration. Full strict decoding passed for both, with exit 0 and empty error logs.

The first PowerShell catalog reads returned historical content. A fresh Python read of the native host's reported catalog path returned the current records, including both creation timestamps. No catalog mutation or repair was performed. All timing calculations use those current records. Sanitized results and hashes are in validation/concurrent-capture-06316-20261017.json; the completion screenshot is validation/concurrent-capture-06316-live.jpg. UDM was left with zero active transfers. Complete IDM parity remains unfinished.

### Additional live formats, 2026-10-18 UTC

After the user restarted Chrome, the installed extension handed off 1440p AV1 from the actual video menu. Download File Info showed Exact final size before Start: 54,867,860 bytes. The destination did not exist before confirmation. The saved file matched that count exactly; the transfer counter remained 60,145,894 across Start. FFprobe reported 2560x1440 AV1 plus AAC, duration 365.273107 seconds. A two-thread full decode timed out at 240 seconds; a subsequent eight-thread full audio/video decode completed with exit 0 and an empty error log. The earlier timeout is retained in the evidence. Evidence: [AV1 results](validation/av1-live-20261018.json).

M4A initially reported that a fresh playback session was needed. After playback and refreshing the format menu, handoff succeeded. The dialog's exact 5,908,172-byte size matched the saved file; the transfer counter remained 6,634,233 across confirmation. FFprobe found one AAC audio stream with duration 365.273107 seconds. Full decoding exited 0 with an empty error log. Evidence: [audio results](validation/audio-live-20261018.json).

Browser CDP input became intermittent; native window control completed the audio interaction on the verified YouTube page. These runs validate the installed build, not the new metadata-lookup cancellation fix. The progress dialog regressed to track-total size for AV1 and Unknown for M4A while remuxing after Start. That display continuity issue remains open.

### Direct publication candidate, 2026-10-18 UTC

Start Download now has a source candidate that releases an already verified media worker from its confirmation wait instead of cancelling it and remuxing the same tracks. It applies when the destination folder is unchanged; a filename change in that folder is supported. Transfer-property edits are refused on this path, and the existing destination/category/queue validation is reused. Confirmation is saved before publication is released, with metadata rollback on save failure. Cancel and Download Later continue to stop prefetch without publishing.

Changed folders and media still being fetched use the existing stop/configure/resume path. Cross-folder and cross-volume staging reuse remain unfinished. The candidate is not installed or yet validated. Regression cases exercise actual MP4/M4A staging, retained file timestamps, filename changes, destination collisions, transfer-property refusal, and a locked catalog during confirmation. Build is in progress.

The prior cancellation fix is installed (SHA-256 847497e245db70f2527e831c74703fb34dbae473d4076727b658127854b9bda7). Normal desktop launch shows 25 records, while Python and child processes see six records at the same nominal catalog path. The previous live tests used the latter view. These histories were not merged or overwritten; subsequent live tests must identify both the running process's diagnostics and the matching catalog view.

The first direct-publication run completed 57 checks and failed two timestamp assertions. An independent synthetic file demonstrated that writing Zone.Identifier changes the last-write timestamp; the production publication path intentionally writes this stream. The assertions now compare volume/file identity plus bytes instead. The initial log is retained in validation/prefetch-publication-first-run-20261018.log. A test-only identifier error in the first correction was fixed before rerunning; no passing rerun or new app build is claimed yet.

The corrected run passed all 59 media checks and all four HTTP-prefetch checks. File identity confirms that both MP4 and M4A publication retained the verified staging file, rather than creating another assembly. Tests also passed for rename, rejection of a conflicting destination, refusal to modify transfer properties, and confirmation-save rollback while the catalog was locked. The desktop build and live browser verification are still pending. Logs are retained in validation/prefetch-publication-media-20261018.log and validation/prefetch-publication-network-20261018.log.

Installation completed with a backup. The installed executable matches SHA-256 7B030F580B3B8F1D204FD950502BCE4B4C7F4A326BACDE0EEF97E364ED66FBFD. UDM reopened normally. The normal desktop catalog retained all 25 records with identical IDs, status and size values compared with the pre-install snapshot. Live browser confirmation remains pending because browser automation detected user input; no fresh browser timing or download result is claimed for this build.

### Early Start candidate

A subsequent, uninstalled source change handles Start while media is still downloading or assembling, provided all submitted dialog values match the job. It releases confirmation without stopping the worker and requests the progress window. It does not claim a verified output size before verification. Destination or metadata edits made before assembly retain the existing stop/configure path. The installed build remains the completed-staging publication build described above.

The new local HTTP test waits for both stalled audio/video requests, submits Start, checks that no replacement requests appear and that one progress event is requested, then verifies Pause cancels the active transfer. The test build and regression runs are pending; see validation/early-prefetch-confirmation-20261018.json.

The first early-Start candidate passed nine HTTP checks and all 59 media checks. Its logs are preserved separately. A follow-up found a scheduling race: the worker read ConfirmationPending after launch, so immediate confirmation could cause both it and the Start handler to request progress. Manager::start now captures the prefetch flag before launching the worker. An immediate-confirmation test checks for one progress event and retained cancellation. This follow-up is rebuilding and is not installed.

The immediate-Start follow-up passed all 12 HTTP/confirmation checks and all 59 media checks. This covers the worker-launch timing case, exactly one progress notification, preservation of the in-flight HTTP requests, cancellation, and the prior staging-publication checks. The desktop build is running; no installation or fresh browser result is claimed for this follow-up yet. See validation/early-prefetch-confirmation-20261018.json and its linked logs.

The early-Start desktop build completed with SHA-256 C7E9B285CAD8DEA3D76271550164B75C9898D9D447C3E3FA893461ABC6494D8A. Installation is pending: native control returned GetCursorPos access denied twice, so UDM could not be closed for replacement. The installed executable remains the prior completed-staging publication build. The user has been asked to exit UDM manually. This is an installation-control limitation, not a failed backend test.
