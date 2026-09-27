# IDM comparison and UDM 0.21.0

Later follow-up: [0.21.1 live retry and media-storage repair](video-comparison-0.21.1.md). Both apps completed a fresh transfer in the follow-up; the older limitations below describe the earlier session.

25 September 2026. Native UDM 0.21.0 is installed in `D:\UDM\release`; browser integration 0.20.3 is loaded in regular Edge. This update does not establish complete IDM parity or a speed win.

## Same-video findings

The user started IDM's 1080p MP4 download of YouTube video `BewnzhHlQuk`. Observation began near completion, so its start-to-finish time and average speed were not measured. The completed IDM file and the previously completed UDM 0.20.0 file have identical SHA-256 hashes of their encoded video and audio streams. Both contain H.264 1920×1080 at 30 fps and AAC audio, with duration about 17:53. The MP4 containers differ in packaging and size: IDM 265,391,223 bytes; UDM 265,982,413 bytes. Evidence: [file comparison](../benchmarks/video-comparison-20260925/file-comparison.json).

IDM's observed menu offered six MP4 qualities, 1080p through 144p, plus English and Hindi ASR subtitles in TTML. UDM offered the six video qualities but no subtitles. IDM's File Info dialog exposed a direct video URL with client `VISIONOS`, format 137 and declared video length 248,107,458 bytes. That observation establishes the endpoint used, not the internal method used to obtain it. Signed URLs were excluded from this report.

An earlier repeat attempt stopped after an expired-trial reminder; that observation did not establish that IDM could not download. The later live retry completed successfully after restarting IDM's normal browser integration and refreshing playback. No license settings were changed. The repeated IDM file is byte-identical to the earlier completed file. Eight active rows and 4.433 MB/s were observed near completion, but a reliable whole-download timing was not obtained. These observations do not establish a paired speed comparison. See [retry evidence](../benchmarks/video-comparison-20260925/retry-evidence.json).

## Implemented changes

- Captured streaming downloads now show progress based on confirmed media coverage, even when total byte size is unknown. Overall progress waits for both video and audio; partial or unvalidated segments do not advance it.
- The download window displays separate video/audio received-byte counts and timeline progress, a duration-based progress bar, and an estimated remaining time. Progress remains below 100% until final publication succeeds. Ordinary file downloads retain byte-based progress.
- Sequential captured-stream requests reuse the existing HTTP session, allowing connection reuse. No measured Internet speed improvement is claimed.
- Browser format lookups now have deadlines both in the background and at the panel. Missing connections produce recovery instructions; stalled handoffs advise checking UDM's list. Downloads are never automatically resubmitted after an uncertain response, and late replies cannot replace a timeout result.

## Validation and remaining limitations

The native build passed **353 checks, 0 failures**, including streaming integrity/progress and the existing download engine suite. A queue test harness race discovered during the first run was corrected without weakening the production retry checks. All three installed binary hashes match the tested build. The real native-host framing and desktop pipe round trip passed. [Native evidence](../backups/stream-progress-20260925/native-test-evidence.json) · [deployment and history verification](../benchmarks/video-comparison-20260925/deployment.json).

Browser logic passed **54 checks**. The visible panel passed **12 checks in isolated Edge**, including timeout recovery, late replies and duplicate prevention. The isolated browser hung during shutdown after recording its passing results and was closed separately. [Panel results](../benchmarks/browser-0.20.3-controls-final/controls-results.json).

The attempted repeat in regular Edge encountered `Could not establish connection. Receiving end does not exist.` Reopening the panel, reloading the extension and restarting Edge did not yet establish a completed new transfer. The background debugger also failed to attach. The updated 0.20.3 timeout message was also verified on the actual YouTube page after its deadline. The exact cause of this regular-profile connection failure remains unresolved; the new timeout messages address the stuck UI, not a proven root-cause repair. Native 0.21.0 streaming progress is covered by automated tests but has not yet been verified through a completed fresh YouTube transfer on this profile.

All 17 pre-update history records retained their status, byte counts, filename, folder and checksum at deployment verification. Both paused ISO downloads remain paused with their previous byte counts. New comparison downloads and the original files have been retained.

Still outstanding: reliable regular-profile browser handoff in the current failing state, subtitle capture, establishing an independent direct-stream retrieval route comparable to the observed IDM endpoint, broader site coverage and a controlled paired speed comparison. Complete IDM GUI/backend parity is not finished.

## Rollback copies

- Native source and prior binaries: `D:\UDM\backups\stream-progress-20260925`.
- Browser changes and affected tests: `D:\UDM\backups\browser-0.20.3-20260925`.
