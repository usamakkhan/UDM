# Concurrent DASH track capture

Live DASH capture can now fetch selected audio and video concurrently. Each track still receives its initialization and media segments in order, so interruption leaves a contiguous committed prefix rather than arbitrary holes within a track. The worker count is bounded by the job's connection setting and number of selected tracks; a single connection preserves sequential capture. The native batch API supports at most three track workers, while the current browser plan supports one video and optional audio or one audio track.

Each worker edits a private track draft. A mutex serializes replacement of that track in the shared recording index and atomic checkpoints. Initial pending identities and URLs are saved before launching any worker, and completed resources are checkpointed individually. Duplicate track identities are rejected before requests so workers cannot write the same receipt or temporary resource path.

Failures cancel sibling workers through a shared child cancellation token. The caller joins every worker before returning or retrying, including failures to launch a worker. Network callbacks receive that token, so manager bandwidth and quota waits also stop when a sibling fails. Shared Rate objects and the manager's existing limits continue to account for combined traffic rather than assigning a separate allowance to each track. Verified resource receipts remain authoritative if cancellation prevents the corresponding index checkpoint.

The job exposes a progress row per selected track, with received-byte accounting protected by the manager lock. Polling reads the current connection setting for the next batch. This does not add parallel requests within a single track, metadata-byte accounting or live Period transitions.

All five native production targets and the installer compiled successfully, recorded in `validation/native-dash-track-workers-build-20261028.json`. Runtime throughput, cancellation and recovery tests remain deferred at the user's request. No speed gain has been measured. Browser 0.63.62 is unchanged; the candidate is unsigned, uninstalled and unpublished.
