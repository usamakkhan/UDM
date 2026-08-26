# Historical benchmarks: resolver-based UDM 0.2/0.3

These results predate the 0.4.1 capture-only change and do not measure its current performance. The automatic resolver refresh described below belongs to those older builds.

# Live YouTube comparison — September 18, 2026

UDM's Chrome player button successfully downloaded the same video demonstrated in IDM: [AADAT INSTRUMENTAL/BHANWARAY, NESCAFÉ Basement Season 5](https://www.youtube.com/watch?v=Q3TI27IN7X0).

## Measured UDM results

A subsequent **0.3 file-info/progress/completion dialog run** transferred the same 130,576,510 source bytes in **29.52 s**, averaging **4.22 MiB/s** with a 5.18 MiB/s peak sample. Resolution took 11.92 s, merging 1.21 s, and total active processing 42.76 s. It used six video plus two audio workers. The completed file uses the full video title; independent FFprobe and SHA-256 checks confirmed the tracks and matching UDM output digest below. [Dialog-run data](udm-dialog-benchmark.json), [workflow inspection](idm-workflow.md).

The two earlier fresh completed runs below requested 1080p MP4, resolved formats 137+140, and downloaded 130,576,510 source bytes (124.53 MiB). UDM used six video connections plus two audio connections; both streams supported validated byte ranges. FFmpeg combined the streams without re-encoding.

| Measurement | Direct native-host test | Actual Chrome player button |
|---|---:|---:|
| Transfer phase | 31.90 s | 36.93 s |
| Average source throughput | 3.90 MiB/s | 3.37 MiB/s |
| Highest periodic speed sample | 7.36 MiB/s | 5.15 MiB/s |
| Public link resolution | 17.63 s | 19.70 s |
| Merge and publication | 1.60 s | 1.83 s |
| Total active processing | 51.25 s | 59.08 s |

Average equals received source bytes divided by the transfer-phase wall time. That phase includes stream probing and child-file assembly/verification, so it is not a socket-only bandwidth measurement. Total starts when the desktop starts processing and excludes queue wait. Peak is a periodic UI sample, not sustained throughput. Timers include small scheduling and bookkeeping overheads and do not sum exactly.

Both outputs are 130,762,165 bytes. FFprobe verified H.264 video at 1920×1080, AAC audio, and a 635.785578-second container duration. Their identical SHA-256 is:

```text
9aa92850711c9669b9db1d9b837c55bbb876df94900aadef5d2ec3616ef80ac1
```

The browser output is `C:\Users\Abuzar\Downloads\UDM\Video\BHANWARAY feat.mp4`. Its shortened name exposed a title parsing bug, now fixed for future downloads. Existing completed files were retained.

## What the IDM observation supports

IDM visibly used eight receiving connections and reported approximately 5.35 MB/sec at one stage and 4.455 MB/sec near completion, followed by audio/video mixing. Those are **instantaneous displayed rates**, not a measured full-download average. The exact unit convention was not independently calibrated. See [the original observation](idm-live-observation.md).

These results do **not** establish an overall winner or an acceleration ratio. UDM's measured averages cannot fairly be compared with IDM's two instantaneous readings. A controlled comparison would time repeated fresh downloads of matching tracks under equivalent network and playback conditions. YouTube CDN URLs may select different endpoints between resolutions; playback was not held constant during all observations.

## Reliability findings

An initial development attempt failed before transfer because of a Framework JSON ArrayList cast; the implementation and regression test were corrected. A later additional browser attempt transferred about 7.7% before HTTP 403. UDM now attempts one fresh public playback-URL resolution on HTTP 401/403 before giving up. After rebuilding, resuming that failed job triggered fresh resolution and completed successfully with the same SHA-256. This does not guarantee that YouTube will accept every request. Successful runs and failed attempts are both relevant when assessing readiness.

The recovery job retained its partial progress and accumulated timing from the original failure: 39.07 seconds in transfer phases, 49.86 seconds resolving links, and 91.16 seconds active processing in total. It is excluded from the fresh-download comparison above. The recovered output is `BHANWARAY feat (1).mp4`; [recovery data](udm-recovery-check.json) records the result.

Chrome initially reported that its native messaging host could not be found despite matching registration. Fully exiting Chrome, including its background process, and reopening it resolved the connection. No driver was needed for the working extension-to-desktop transfer.

## Evidence

- [Native-host benchmark data](udm-live-benchmark.json)
- [Chrome benchmark data](udm-browser-benchmark.json)
- [Validation and remaining gaps](validation.md)

Only the public page URL is included in these reports; temporary signed playback URLs and request credentials are omitted.
