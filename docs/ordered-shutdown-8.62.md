# Ordered transfer shutdown qualification, 8.62

The unchanged private 8.62 candidate passes **20 native assertions in four processes on C: and D:**. On each drive, actual Manager shutdown waits for a held older catalog checkpoint, joins ordered transfer IO and saves partial written credit. A fresh process reloads that catalog, resumes and publishes the exact complete 8 MiB file. Normal builds still use synchronous writes; this is test-only work, with native/browser versions unchanged at 8.62 / 0.72.26.

## What is exercised

The [controller](../benchmarks/speed/ordered_shutdown_control.cpp) downloads through the real Manager pipeline from the versioned synthetic range origin. After reception exceeds 512 KiB, a dedicated save captures an actual detached catalog snapshot. Its callback holds outside the model lock. The controller starts `Manager::stop` concurrently and verifies that shutdown has not returned after a 100 ms observation, the hold has not expired, and neither final output nor completion acknowledgment is present.

Releasing the save lets shutdown join all workers. The transfer's joined diagnostics confirm that the private ordered path was used. Shutdown saves partial rather than completed credit, matching the reconciled job counters. Each drive saves **557,056 bytes** in this run. The orchestrator independently reads and compares every credited segment prefix against the source's expected bytes, not merely the sum of counters.

A second process loads the existing catalog and requires paused, partial recovery state. It resumes using the saved address, completes normal publication and checks completion acknowledgment and exact byte counters. The orchestrator independently verifies final length, expected SHA-256, native/catalog digest and a separate post-exit output flush. The [results](validation/ordered-shutdown-8.62/results.json), [C: cases and origin](validation/ordered-shutdown-8.62/C-cases.json) and [D: cases and origin](validation/ordered-shutdown-8.62/D-cases.json) retain the evidence. These are correctness runs, not performance comparisons or installed-app tests.

## Rejected driver and preservation

The first driver incorrectly checked `OrderedBodyWrites` during reception, before Transfer emits it at writer settlement. It failed that driver assertion; exception cleanup still joined the transfer. The assertion now runs after shutdown. The [rejected result](validation/ordered-shutdown-8.62/rejected-controller-result.json) and [build identity](validation/ordered-shutdown-8.62/rejected-build-result.json) remain excluded from the passing aggregate. No production code was changed to satisfy it.

[Cleanup](validation/ordered-shutdown-8.62/cleanup.json) removed 21 generated files / 34,841,252 logical bytes in three owned roots, including rejected-run data and the fixture executable/object. Deletion verified resolved ownership boundaries, reparse absence, per-file hashes and complete owned hardlink groups. This is not a physical allocation claim.

[Preservation](validation/ordered-shutdown-8.62/preservation.json) rechecks 524 qualified inputs, ten products, 43 distinct core objects and three protected installed/personal files. No installed app, installer, browser deployment, administrator prompt, push or release changed.

## Remaining scope

This closes the scoped direct Manager-stop / held detached-save / reload-resume case. It does not establish crash/power-loss behavior, every shutdown/save failure ordering, refresh-address or folder changes, removal, child/media buffered bindings or performance parity. Those operation races and balanced whole-lifetime C:/D: comparisons still precede normal-build activation. The broader IDM parity objective remains open.

The subsequent [operation lifecycle qualification](ordered-lifecycle-8.62.md) closes the scoped active/retiring folder, refresh-address and removal command boundaries and post-retirement save rollback. Broader failure interleavings and performance comparisons remain separate.
