# Native live DASH job worker

The adaptive job dispatcher now routes validated live DASH jobs to a native worker. Its plan uses a manifest URL and stable Period, AdaptationSet and Representation IDs for one video plus optional audio, or one audio track. MP4 and M4A are supported by this path. Browser admission explicitly rejects live DASH while the extension selection and authorized request-scope handoff remain unfinished.

The worker connects native manifest refresh, synchronized availability planning, scoped resource requests, persistent receipts, stop sealing, local assembly and the manager's existing file-publication operation. It sets the existing live recording/progress fields and uses the existing Stop request and child cancellation mechanism. Parent cancellation preserves the recording for later recovery. Stop finalizes the common completed interval. A sealed index can proceed to assembly without requesting the manifest again.

New captures start at the synchronized live position minus the MPD's suggested delay (six seconds when absent). Polling advances from the committed prefix of each track, retaining pending segments for retry rather than skipping them. The availability planner independently checks the synchronized clock before requests. The initial-clock-plus-monotonic horizon only bounds planning; it does not grant availability. Dynamic-to-static completion records an explicit Period end, so missing final coverage prevents publication.

Selected media kind and video height are checked against the initial resolved tracks. Subsequent refreshes use the existing stable-ID and format continuity checks. Media bytes pass through manager quota/rate accounting and update transferred-byte progress. Resource failures use the queue retry count with bounded backoff. Assembly retains the selected stream checks and expected SHA-256 check before publication. Transfer, merge and total elapsed times are recorded. Recording receipts, excluded tails and assembly inputs are retained for recovery; automatic cache cleanup is not introduced here.

## Remaining integration and limitations

- Browser live DASH remains unavailable; this native path is not yet reachable through normal browser capture.
- Period transitions and live subtitles are not implemented. A changed or missing selected Period fails with retained data.
- The subsequent [ended-stream resume change](native-dash-ended-resume-20261028.md) adds a persisted presentation/selection bootstrap for new recordings. Older captures without it need a matching dynamic MPD to adopt one. Sealed recordings can finalize offline.
- The subsequent [track-worker change](native-dash-track-workers-20261028.md) downloads selected tracks concurrently, and [metadata accounting](native-dash-metadata-accounting-20261028.md) charges completed manifest/clock bodies. Parallel requests within a track, partial metadata-read accounting and richer retry classification still need integration.
- Limits and unsupported addressing modes from the native DASH parser and storage layers still apply.
- Packet-granular cuts, exact timestamps, interruption behavior and file playback have not been exercised at runtime.

All five production targets and the installer compiled successfully; results and source hashes are recorded in `validation/native-dash-job-worker-build-20261028.json`. Testing remains deferred at the user's request. Browser 0.63.61 is unchanged. This candidate is unsigned, uninstalled and unpublished, and complete IDM parity is not established.
