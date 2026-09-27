# UDM 0.21.1: live video comparison and storage repair

25 September 2026 local time (26 September UTC). Native 0.21.1 is installed; Edge integration remains 0.20.3.

## Result

IDM and UDM both completed the same YouTube video (`BewnzhHlQuk`) at 1080p. UDM's completed output has **identical encoded video and audio SHA-256 hashes** to IDM's output: H.264, 1920×1080 at 30 fps, AAC, approximately 17:53. Packaging differs. The new UDM file contains 265,982,413 bytes and took 104.84 seconds according to its recorded transfer/assembly elapsed time, including initial waiting. This is one live run, not a general speed benchmark.

IDM showed eight active transfer rows and approximately 4.433 MiB/s near completion. Its new file is byte-identical to its previous output. We did not reliably measure IDM's whole-download duration, so no paired speed graph, speed win or speed parity is claimed. The earlier interpretation that the trial reminder prevented downloading was incorrect; IDM downloaded successfully after its normal browser integration was restarted. No licensing settings were changed.

## Failure reproduced and repaired

UDM 0.21.0's first repeat failed at 196,849,506 received bytes with `Cannot write streaming segment.` D: had only about 206 MiB free after cleanup. The failure is consistent with disk exhaustion, but the old code discarded the Windows error, so the exact error code is unavailable.

Code inspection confirmed that captured YouTube and HLS/DASH media ignored `TemporaryFolder` and always stored working media under the data directory on D:. Version 0.21.1 uses the configured temporary folder and persists each job's chosen location. Existing nonempty legacy working folders and persisted job locations remain in place when preferences change. New downloads now use `C:\Users\Abuzar\AppData\Local\UDM\Temp`.

Streaming create/write/flush failures now identify the temporary directory and Windows error. Disk-full errors explain how to recover. Failed streaming track rows change to `Stopped` instead of retaining `receiving` labels. The build script also accepts an absolute `-OutputRoot`, allowing build and test output on a drive with space.

## Verification

- **362 native checks passed, 0 failed.** New checks cover actual streamed output in the selected folder, persistence across preference changes, preservation of legacy partials, empty-directory relocation, disk-full diagnostics, a real Windows sharing violation, stopped failure rows, and HLS output in the selected folder.
- All three installed binary hashes match the tested build. Existing history was byte-identical during deployment.
- All 18 pre-deployment records retained their status, received bytes, size, filename, destination and checksum after the live test. There are now 19 records, including the retained failed test and the completed retry. Paused ISO downloads remain paused.
- Regular Edge successfully opened the quality menu, handed off to native UDM, displayed File Info and live video/audio progress, and completed the new download using the C: temporary folder.

Evidence: [live samples and media hashes](../benchmarks/video-comparison-20260925/udm-0.21.1-live.json), [IDM retry and initial UDM failure](../benchmarks/video-comparison-20260925/retry-evidence.json), [deployment](../benchmarks/video-comparison-20260925/deployment-0.21.1.json), [history validation](../benchmarks/video-comparison-20260925/validation-0.21.1.json).

## Remaining differences

Complete IDM parity is not established. The observed IDM path uses a direct video endpoint; UDM used its captured streaming transport in this test. UDM still lacks the observed subtitle choices. This capture requires a fresh transfer after interruption. The earlier intermittent browser connection failure was not reproduced in this session; its underlying cause remains unproven. UDM's startup delay and a controlled paired speed comparison remain outstanding.

