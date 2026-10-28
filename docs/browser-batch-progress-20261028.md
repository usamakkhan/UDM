# Browser batch progress and interruption receipts

Larger collected batches previously displayed only a static Adding message. Completed and failed batches were deleted immediately, while a reopened busy list produced a generic error with no acknowledged count.

Extension 0.63.52 adds a read-only batch-progress command restricted to the extension review page. It reports acknowledged handoffs, selected total and phase. The review polls without overlapping requests, stops at terminal status, and stops after three consecutive status failures. A lost main response does not trigger a retry; status polling can still discover the result. Reloading a submitted page shows its receipt rather than opening a reusable selection.

Before the first handoff, the busy marker and initial receipt are persisted together. Each acknowledged handoff updates and persists the count before the next link. A persistence failure stops further handoffs. Completed/failed receipts remain under the existing eight-list and ten-minute retention bounds; the clock refreshes on acknowledged progress and completion. Active in-memory submissions are not expired or evicted. If all eight retained lists are active, collecting another list reports that the user must wait.

Restored busy receipts without an active worker are labeled interrupted. The last request may have been accepted without its acknowledgement being saved, so the UI directs the user to inspect UDM and never automatically retries. Counts indicate handoff acknowledgement, not completed file downloads or measured network progress. Receipt fields contain only phase/count/total; per-file credentials remain outside persisted batch data.

The native build is unchanged. JavaScript syntax, mirror and manifest checks are permitted and recorded; runtime, browser, worker-restart and storage-failure testing are deferred at the user's request. This implements UDM's reliable bulk-admission workflow and does not claim a newly observed IDM receipt protocol.
