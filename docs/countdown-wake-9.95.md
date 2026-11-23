# Completion countdown acknowledgement without another timer pulse — 9.95

Native **9.95**, Chromium/Firefox source manifests **0.73.59**. Paired browser versions advance as requested; this is a desktop change. All five normal native programs are built. All **54 backend and 130 current UI checks pass**, plus **five separate frozen-control assertions**. No installer, installation, commit, push or release is included.

## Remaining delay and change

The 9.93/9.94 file and queue countdowns moved scheduler work off the UI thread and required a fresh final acknowledgement. They still waited for the next 250 ms dialog pulse to consume that acknowledgement. Completed worker work could therefore leave a ready dialog waiting for its timer.

Both dialogs now bind their existing generation-checked Form task notifier to a UI-owned scoped route. The main maintenance notification forwards a wake to that route before continuing its existing task service. `CompletionCountdownWakeGuard` clears the route after modal return and on exceptions; Form disarms its weak signal when the HWND is destroyed. No Form pointer, action or command payload crosses the worker boundary. There is no new thread or new timer. The existing main-window completion-action guard serializes these scoped bindings.

On a task wake, `CompletionCountdownDeadline` refreshes eligibility without requesting another ordinary tick. Actual periodic pulses continue normal scheduling. At expiry, the existing final-tick acknowledgement protocol still runs; a passive wake can request that one required tick and consume its completion. Repeated final acknowledgements cannot schedule it again. This distinction prevents a maintenance-completed → dialog-wake → request-maintenance loop.

All current job/scanner/queue-cycle/work-epoch checks, Cancel/Enter semantics, durable claim handling and ordered queue-step continuation remain in place. An ordinary timer remains the fallback for failed delivery or model owners that do not emit a maintenance completion signal. This is not a guarantee that every foreign-model wait or every shell operation completes without polling or UI delay.

## Qualification design

Backend checks cover scoped ownership, duplicate binding, exception cleanup, passive wake bursts, ordinary periodic scheduling, final acknowledgement without another timer, failed/stopped work and prevention of self-scheduling loops. Retained file and queue admission/deadline checks are rebuilt against the new helper.

The isolated UI comparison disables the **actual dialog pulse timer**. It waits for initial maintenance to settle, posts valid completion wakes and an old dialog notifier, verifies no extra tick, holds the real model past expiry, sends one explicit expired pulse and releases the writer. Current code must then close and perform exactly one intercepted/consumed owned action without another timer pulse. Both themes and both countdown types run. The fixture intercepts queue actions and consumes the file action's posted Exit command before dispatch; it performs no real file-open, disconnect or computer power operation.

The control uses the **exact qualified 9.94 App.cpp source**, captured before editing. It shares current headers, core objects, resources and the `/Od` isolated test build with the candidate. Its current helper's default periodic argument preserves the old App call behavior. This is a controlled App-source counterexample, not the normal 9.94 product or an IDM throughput/first-pixel benchmark. With the fallback timer disabled, the control must need the owned 800 ms cancellation watchdog. Neither its watchdog nor fixture assertion counts are included in current application results.

## Remaining gates

Current installed/browser acceptance, repeated normal IDM timing, body/intent flush attribution, automatic shell/folder operations, secondary-modal ownership and full visual parity remain open. Between-step queue continuation still has the main service timer as a fallback; this change targets acknowledgement while the countdown dialog is open. No transfer pacing or durability barrier changes.

Host installed **8.68 / 0.72.32** is preserved; the lab is not started by this increment. Temporary fixtures are retained because automatic approval review earlier rejected cleanup with “blocked by policy.” No alternate cleanup route is used.

## Recorded qualification

| Current focused suite | Assertions |
| --- | ---: |
| Scoped notifier lifetime, passive/periodic work, no self-scheduling loop and final acknowledgement/error handling | 18 |
| Retained queue admission, labels, epoch/cycle and current settings invalidation | 16 |
| Retained file admission and final scheduler acknowledgement/failure | 20 |
| New timer-disabled current UI, both themes and both countdown types | 30 |
| Retained queue countdown and ordered continuation UI | 34 |
| Retained file-countdown UI | 16 |
| Retained completion-claim UI/error/shutdown ownership | 50 |

The frozen App-source counterexample has five additional assertions, counted separately.

| Timer-disabled case | Writer release to modal return | Required cancellation watchdog? | Performed owned action |
| --- | ---: | --- | ---: |
| Current file, light | 0 ms | No | 1 |
| Current queue, light | 0 ms | No | 1 |
| Current file, dark | 15 ms | No | 1 |
| Current queue, dark | 0 ms | No | 1 |
| Frozen 9.94 App source, file/light only | 812 ms | Yes | 0 |

These millisecond readings are controlled UI-flow observations, not zero-time execution guarantees, a normal-product timing matrix or an IDM comparison. The frozen case is explicitly canceled by its watchdog; 812 ms is not its natural completion latency. All four current cases complete with the real fallback timer disabled, and all retained current checks pass on the first attempt. The model writer's own four-second watchdog does not expire.

Final verification checks **726 source inputs**, **42 core objects**, all **13 output binaries**, and nine protected installed program/manifest/tool files. All five normal PE product versions read 9.95. Only the version-bearing Bridge core object changes; the other 41 retain qualified 9.94 identities. Current and frozen UI fixtures share `/Od`, current core/resources and the same test source; normal products use `/O2`.

[Complete sanitized qualification, frozen-source proof, build inputs/logs, exact source snapshots and runtime records](validation/countdown-wake-9.95/). No fresh network download is performed by these fixtures. Local synthetic catalogs and build outputs remain; no cleanup is claimed.
