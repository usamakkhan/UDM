# Responsive queue-finished countdown and ordered action continuation — 9.94

Native **9.94**, Chromium/Firefox source manifests **0.73.58**. The browser increments follow the requested paired-version policy; this change affects desktop queue completion. All five normal native programs are built. All **36 backend and 100 UI checks pass**. No installer, installation or release is included.

## Cause and change

Queue-finished dialogs still called `Manager::tick()` from their UI pulse and waited for the model in `completionEventReady` and queue-label construction. A slow scheduler/checkpoint could therefore block repaint and Cancel. Unlike the file countdown corrected in 9.93, this route also rechecked readiness synchronously between a sequence of completion actions.

`tryQueueCompletionCountdown` uses a try-lock and respects an accepted File Info decision. It calls the existing queue-cycle/work-epoch readiness contract only while holding the model. Presentation labels and completion text are captured together; repeated pulse/step checks do not rebuild the labels. No filesystem or scheduler operation runs in this snapshot helper.

The countdown requests scheduler work from the existing owned maintenance worker and uses the 9.93 deadline acknowledgement: older work drains, a fresh tick completes, and current eligibility is checked before execution. Busy leaves the dialog cancellable. Failed/stopped final work cancels the action. The Cancel button is the default, and the Enter route cancels rather than accepting a computer action.

The main window retains pending queue events and a canonical per-event step cursor. Busy before presentation retains the event; disabled main ownership and foreign accepted decisions do not consume it. Busy between steps retains the confirmed cursor and resumes the unfinished step on a later service call, without another countdown or replay of earlier actions. Each step rechecks the current queue cycle, global work epoch and existing completion policy. A stale or edited queue discards the remainder. A throwing action consumes that event without silently retrying a possibly performed side effect; other pending events remain owned for subsequent service. Explicit Exit still closes the application, abandoning volatile remaining queue intents as before.

There is no new worker or detached thread. Existing maintenance shutdown joins accepted scheduler work before Manager destruction. Queue intents remain UI-owned, process-local work; this change does not make queue actions durable across application restart. It does not remove publication/catalog barriers or change transfer pacing.

## Qualification scope

Focused checks cover current and stale queue identities, owned display labels, foreign model/decision ownership, new queued/synchronization work, actual queue-settings invalidation and ordered cursor construction. Light/dark UI fixtures exercise a pulse past expiry while the model is held, repaint, Cancel/Enter, normal timer expiry, between-step contention with resumption, and real queue-settings invalidation between steps. The side-effect performer is replaced only in the isolated component fixture; no real file-open, dial-up, exit or power operation is invoked.

The UI fixtures start with explicit owned queue identities. They exercise real Manager readiness, settings mutation, maintenance and modal code, but do not claim a fresh browser/download-to-queue completion workflow. Retained 9.93 file-countdown and completion-claim checks are rebuilt against current App source. Isolated UI fixtures use `/Od`, normal native programs `/O2`.

## Remaining gates

The 250 ms countdown pulse remains the fallback for final acknowledgement/resumption; immediate dialog wake-up is separate work. Automatic shell/folder actions can still block at the Windows shell boundary. Storage finishing stalls, repeated normal/installed IDM comparisons, current browser capture acceptance and complete visual parity remain open. These focused UI changes do not establish universal speed parity or a completed project.

Host installed **8.68 / 0.72.32** is preserved. The lab remains off because of limited storage. Temporary fixture cleanup remains blocked by the earlier automatic approval rejection; no deletion or cleanup is claimed.

## Recorded qualification

| Current focused suite | Checks |
| --- | ---: |
| Queue admission, labels, stale cycle/epoch, accepted decisions, queued/synchronization work and real settings invalidation | 16 |
| Retained file admission and final scheduler acknowledgement/failure | 20 |
| New queue UI in both themes, cancellation/Enter, normal expiry, step contention/resumption and mid-sequence settings invalidation | 34 |
| Retained file-countdown UI in both themes | 16 |
| Retained completion-claim UI and shutdown/error acknowledgement | 50 |

Both queue UI cases hold the model past the one-second fixture countdown. Their explicit pulse and Cancel/Enter callback durations read **0 ms** on the millisecond clock; this is not a zero-time execution guarantee. Neither writer watchdog expires, and the modal window is destroyed before the fixture releases its writer. Normal timer expiry also performs exactly one intercepted action. The confirmed multi-step case retains cursor 1 under real model contention, resumes only Exit on the later service call, and cannot replay on another wake. A real `setQueue` edit after the first intercepted step invalidates the remaining step.

Final verification checks **724 source inputs**, **42 core objects** and all **10 produced binaries**. The five normal PE product versions are 9.94. Only the version-bearing Bridge core object changes from qualified 9.93; the other 41 core objects retain their identities. Nine protected installed program/manifest/tool files are unchanged. No full-suite result, live browser test or fresh UDM/IDM speed comparison is claimed.

[Sanitized inputs, logs, complete current results, binary identities and exact source snapshots](validation/queue-countdown-9.94/). All checks completed on the first build attempt for this increment. Synthetic catalogs and native outputs remain local; no test download payload is generated by these fixtures, and no payload or fixture cleanup is claimed.
