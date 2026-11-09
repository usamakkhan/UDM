# Completion bounds, File Info timing and duplicate range requests

This work follows the [5.45 parity audit](../idm-parity-architecture-audit-5.45.md). The three cases have separate acceptance checks so that a shorter wait or a matching final hash cannot hide a UI or transport regression.

## Expanded completion

The installed 5.45 audit failed the scanner-expanded bounds assertion after 65 passing checks. A later run without the moved-file modal passed the bounds checks. A separate run with that modal remained live after its test deadline. The earlier failure remains valid as an observed regression, but the two later results do not establish the same triggering conditions.

UDM 5.46 uses the client area Windows actually allocated when a fixed dialog is resized. If the calculated frame yields too little room, it adds the measured shortfall. The scanner variant checks its client size before it is shown. The completion regression checks all four file actions and the suppression checkbox before and after DPI changes, with an injected non-client frame larger than the calculated one. The ordinary reference geometry remains covered.

All 128 completion checks and 207 progress-dialog checks passed on 5.46. The completion run includes the injected frame and four scanner states at 96, 120, 144 and 192 DPI. It used simulated DPI changes and omitted the optional moved-file modal. Five production executables and both 0.69.10 browser bundles are staged. [Build receipt](completion-client-dpi-5.46-build.json)

## File Info control on 5.45

Each run used an isolated Edge profile, the installed 0.69.9 extension files, and a 5.45 native presentation fixture connected through the installed host. The origin served eight seconds of synthetic HLS. The final size label appeared before Start, the staging SHA-256 matched the output, the main list stayed hidden, and the file decoded through the expected duration. The temporary host registrations were removed.

| Output | Click to enabled Start | Click to first segment | Click to exact size | Verified bytes |
| --- | ---: | ---: | ---: | ---: |
| 720p MP4, Spanish audio | 1,406 ms | 1,962 ms | 13,845 ms | 6,560,076 |
| 720p MP4, Spanish audio, repeat | 3,696 ms | 4,541 ms | 8,249 ms | 6,560,076 |
| 360p TS, English audio | 887 ms | 1,332 ms | 3,587 ms | 1,961,216 |
| M4A, Spanish audio | 823 ms | 1,532 ms | 2,977 ms | 98,791 |

Native control construction ranged from 302 ms to 1,746 ms. The MP4 runs overlapped compiler work. These measurements localize stages and prove output, but they do not support a precise speed claim. The native trace measured first paint and first enabled Start observation separately. The earlier observed timing work is linked from the [File Info input-priority report](file-info-input-priority-5.38.md).

The 5.47 source increment moves first paint after the existing form layout and theme initialization. This avoids applying the theme again after the browser-owned window is shown. Media File Info creates no hidden ordinary-file server-name controls. Start and Cancel remain available before prefetch. The final-size label continues to require a verified output, with separate labels for selected-track bytes and an assembled file still being verified. The 5.48 build carries this change.

The 5.48 native presentation suite passed 163 checks. Three isolated Edge runs passed 22 checks, including an explicit check that media File Info does not create the ordinary server-filename button. They used the staged 0.69.12 extension files and native host. Each run received bytes before Start, kept the main list hidden and decoded the completed file through the expected end. The verified size and staging hash matched the output, and the temporary host registrations were removed.

| Output on 5.48 | Click to enabled Start | Click to first segment | Click to exact size | Verified bytes |
| --- | ---: | ---: | ---: | ---: |
| 720p MP4, Spanish audio | 302 ms | 910 ms | 3,264 ms | 6,560,076 |
| 360p TS, English audio | 399 ms | 918 ms | 3,601 ms | 1,961,216 |
| M4A, Spanish audio | 378 ms | 848 ms | 2,354 ms | 98,791 |

### Alternating MP4 timing comparison

Three trials per build used the same 720p source, the same fixture settings and fresh Edge profiles. Compiler and range runs had stopped before this series. All six showed the correct 6,560,076 bytes before Start and verified the published output. The small sample shows a faster opening median with substantial trial-to-trial variation. Time to prove exact size did not improve in the median.

| Median across three runs | 5.45 | 5.48 |
| --- | ---: | ---: |
| Click to enabled Start | 558 ms | 451 ms |
| Native control construction | 171.0 ms | 113.8 ms |
| First paint return to enabled Start observation | 50.0 ms | 41.4 ms |
| Click to first segment | 1,063 ms | 952 ms |
| Click to exact size | 3,165 ms | 3,667 ms |

The baseline click-to-Start samples were 651, 463 and 558 ms. The candidate samples were 278, 603 and 451 ms. The recorded median network phase was 1.77 seconds on 5.45 and 2.40 seconds on 5.48. The recorded median merge phase was 0.28 and 0.24 seconds. The next exact-size timing pass should collect per-segment start and finish times alongside the output verification stages. These are owned HLS results. Public YouTube and regular browser profiles remain separate checks.

The follow-up records those segment timings and tests catalog access during a blocked receipt replacement. [5.49 receipt and exact-size evidence](adaptive-receipt-lock-5.49.md)

The ordinary-file filename and prefetch fixture passed 253 checks. It confirmed URL-and-MIME filename suggestions, filename adoption and cancellation, Start while a response was still arriving, destination changes, and Download Later reuse after reopening. Valid retained responses used one GET each. A missing or damaged staged file triggered one recovery GET to the replayable origin. The fixture rejected that request with HTTP 410 and verified that no unverified file was published. Test-only updates matched the current reader buffer and single-file staging layout.

## Ranged request control on 5.45

Two 24 MiB healthy runs used 11 and 12 GETs, with nine and ten dynamic splits. Two 192 MiB healthy runs used 18 GETs each. A 192 MiB trace included the same start and end offsets twice, with the first request already at the origin before it was displaced.

A deterministic owned 4 MiB fixture held the adjacent range's response headers until the first range had finished. UDM 5.45 made two requests for bytes 2,097,088–4,194,303 and repeated two smaller tail requests. It produced the correct SHA-256. The candidate should preserve that already-owned request while continuing to reuse idle sockets for unclaimed work. Failed opens still need the normal retry and range-release logic. The delayed range, healthy, two-connection cap and reset cases must all preserve the expected hash.

The 5.48 source increment keeps an in-flight request with its owner. A finished worker can still claim an adjacent range if nobody owns it. A failed request can still release the range through the existing retry path. This change addresses the repeated request seen while headers are pending. It is separate from tuning the policy for splitting small, healthy tails.

The staged 5.48 executable requested the held adjacent range once. Total HTTP requests fell from nine to five on this reproduction. Both executables completed with the exact expected SHA-256. One wall time was 2.925 seconds on 5.45 and 3.087 seconds on 5.48. The sample establishes the request count and correctness. Five native executables and 95 browser/setup-guide files are staged. Each 0.69.12 extension bundle passed 130 focused checks. [5.48 build receipt](file-info-and-range-5.48-build.json)

| Loopback origin, two runs each | 5.45 HTTP requests | 5.48 HTTP requests |
| --- | ---: | ---: |
| Healthy ranged, 24 MiB | 11 / 12 | 10 / 12 |
| Healthy ranged, 192 MiB | 18 / 18 | 14 / 14 |
| Two allowed connections, 24 MiB | 7 / 7 | 6 / 6 |
| Forced connection resets, 24 MiB | 20 / 24 | 17 / 17 |

All eight outputs per build matched the expected size and SHA-256. The series overlapped different compiler and fixture activity, so the wall times do not establish a speed gain. The two-slot origin still refused five or six opens per trial, and healthy 24 MiB request counts still varied. Those are separate targets for a quiet rerun and tail/admission tracing.

The first native transfer run exceeded the forced-reset case's six-second timing threshold while producing the exact file. A quiet rerun passed the reset timing check and exposed a fixture race in which Pause could follow the initial split before any bytes were written. After the fixture waited for both the split and written bytes, all 69 native transfer checks passed. That run also passed the six-second reset check and observed the first accepted connection 32 ms after the simulated outage ended.

