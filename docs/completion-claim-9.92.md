# Completion presentation before armed-action catalog I/O — 9.92

Native **9.92**, both source browser manifests **0.73.56**. All five native products are built. This candidate is not installed or released; there is no new installer or browser behavior change in this increment.

## Cause and change

`MainWindow::tryComplete` previously called `Manager::takeDownloadCompletion` on the UI thread. An armed action disarms its durable record through `save()` before returning. A slow checkpoint therefore delayed construction of the completion dialog, even though the downloaded file was already complete.

The UI now captures presentation inputs using the existing nonblocking model admission, retains a deduplicated claim request, and drains completion presentation before starting the claim worker. `CompletionClaimTask` owns the accepted save independently of the completion window. It waits for the live model and any already-accepted decision; the existing manager method rechecks membership, completion, active ownership, scanner policy and current action settings before committing the disarm. It returns only a canonical action specification. The worker never accesses a window or invokes a shell or power action.

Only a successful durable result can enter the existing action/countdown dispatcher. Closing the completion window remains responsive and does not abandon the accepted save. A later UI wake joins and acknowledges the worker exactly once. Unarmed completions create neither a task nor an extra checkpoint. Competing claims still serialize through the manager and produce at most one action.

Save failure retains the manager's existing live/durable rollback. The UI offers Retry/Cancel after failure, without executing an action; Retry retains the exact failed job. Cancel stops the automatic retry and explicitly leaves the saved setting unchanged. A ready error is retained while the main owner is disabled and reviewed after re-enable. CLI Exit waits for both queued claims and joined UI acknowledgement. Main-window shutdown joins accepted claims before stopping the manager; a failed claim prevents silently treating that save as successful and closing.

The actual publication/flush barriers are unchanged. No fake speed, optimistic completion, detached writer or network pacing change is introduced. The 9.91 transfer object and remaining core objects are reused unchanged except for the version-bearing Bridge object.

## Focused qualification

**147 backend assertions and 210 current UI assertions pass**, plus two assertions for the frozen UI counterexample:

| Coverage | Assertions |
| --- | ---: |
| New claim ownership, duplicate/competing claims, foreign-decision priority, scanner rejection, exact checkpoint rollback, explicit retry, notification failure and joined cleanup | 20 |
| Retained range ownership/queue, staging, publication availability and cleanup backend controls | 127 |
| New completion claim UI, light and dark | 50 |
| Actual modal-owner completion regression cases | 136 |
| Completion suppression preference and shutdown UI regression cases | 24 |
| Frozen App source counterexample, separate from current UI totals | 2 |

The UI fixture holds the actual manager checkpoint at `temporary-flushed` for a bounded 550 ms watchdog. Its original source is reconstructed and verified against the captured 9.91 App source hash; other objects and resources are current. Both isolated UI fixtures use **`/Od`**, whereas the five normal products use **`/O2`**. These are controlled responsiveness/ownership checks, not a timing comparison of the normal 9.91 product or IDM.

| Controlled UI source | Completion dispatch, ms | Save watchdog expired before return? |
| --- | ---: | --- |
| Frozen 9.91 App source, light | 593 | Yes |
| Current 9.92, light | 31 | No |
| Current 9.92, dark | 31 | No |

Current cases observe a created completion window and enabled Close while the claim remains held, repaint it, close it without waiting, then receive exactly one committed action after the originating window is gone. Real Retry/Cancel dialogs, rollback, disabled-owner review retention and the CLI Exit command are exercised. A ready durable claim still prevents CLI Exit until its UI acknowledgement joins the worker; afterward the normal route posts the expected owned Exit command, which the fixture consumes without executing it. No real file-open or power action is invoked by these tests.

An initial run stalled because the test assumed native MessageBox button IDs in dark mode; themed Form buttons use generated IDs. That owned fixture was explicitly aborted and is not counted as a pass. The final selector verifies labels within the owned dialog tree, accounting for native accelerator markers. A corrected test declaration also required refreshing build inputs before qualification. Initial records, the final App rebuild inputs, exact source snapshots and final fixture roots distinguish those attempts. Earlier 44-assertion claim results are superseded by the final 50-assertion result; the unchanged modal/suppression fixture results are retained rather than double-counted.

## Normal 4 MiB smoke and remaining speed loss

One normal uninstrumented C: pair against installed IDM 6.43.10.2 downloads the complete deterministic 4 MiB file. Both exact hashes and lengths pass; UDM's saved catalog reports Complete and the expected digest; all tracked processes exit. IDM's connection settings are restored and checked. Other-session IDM processes are not stopped. IDM again returns launcher code 1 despite a correct output; its code semantics remain unestablished.

| Endpoint | UDM 9.92 | IDM |
| --- | ---: | ---: |
| Total process lifetime | 1.183 s | 2.092 s |
| Last origin send to exclusive file availability | 31.314 ms | 16.100 ms |
| Last origin send to process exit | 95.715 ms | 373.198 ms |
| Last origin send through post-exit output flush | 106.670 ms | 420.873 ms |
| Total through post-exit output flush | 1.194 s | 2.139 s |

**UDM loses file availability by 15.214 ms in this sample.** Faster total/exit/flush times do not erase that loss, and the earlier 9.91 C: wins do not prove a universal gate. There is one pair here, no fresh 64 MiB or D: comparison, and no claim that this isolated difference is caused by the claim change. The small smoke is unarmed; the armed workflow is qualified separately above. Measurement includes observed app/thread exit and a symmetric explicit file flush, not every external service, GUI first-pixel timing, device cache or power-loss behavior.

The general speed gate stays open on both volumes. Next use repeated alternating small normal-product controls, retaining availability, total, exit and post-exit-flush measurements before attributing that remaining gap. UI work still includes countdown model ticks/eligibility waits, automatic folder/shell operations and current browser selection-to-actionable paint. This increment closes the armed-claim save blocking point, not the entire completion or visual-parity backlog.

[Sanitized build, current/frozen source, fixture results and full smoke records](validation/completion-claim-9.92/). Host installed binaries, manifests, tools and personal catalogs are not replaced by these checks. The lab remains off. Earlier automatic approval review blocked fixture and payload-only deletion with “blocked by policy”; no alternate cleanup route is used, and temporary fixtures remain.

Exact source snapshots opt out of Git newline conversion so captured LF/CRLF input hashes remain meaningful. A clean-filter check verifies that a CRLF transfer snapshot retains the same Git blob bytes; other validation documents retain the existing LF policy.

## Repeated availability follow-up

The subsequent [9.92 availability controls](availability-controls-9.92.md) supersede the single-pair next-step status above: 57 complete 4 MiB transfers include explicit C: profiles/D: output and three rejected controls. Normal application binaries remain unchanged. Remaining availability losses, 64 MiB repetition, installed acceptance and GUI paint gates stay open.
