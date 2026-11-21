# 8.53 finishing research

The qualified portable 8.53 build was compared with IDM on D: after the user closed an unresponsive IDM instance. Installation remains deferred; installed UDM is still 8.32. No application source, version, installer or durability policy changed for this research.

## Fresh paired observations

One trial per case, with identical generated content and origin settings. Times below start at the last origin body send. The exit endpoint uses actual process exit times for all observed application processes. The additional endpoint opens and explicitly flushes the completed output after exit, symmetrically for both clients.

| Case | UDM exit | IDM exit | UDM through post-exit flush | IDM through post-exit flush |
| --- | ---: | ---: | ---: | ---: |
| Sequential 4 MiB | 283.304 ms | 519.526 ms | 319.587 ms | 659.077 ms |
| Sequential 64 MiB | 186.686 ms | 629.504 ms | 200.960 ms | 1,485.115 ms |
| Split 64 MiB | 663.087 ms | 517.478 ms | 689.105 ms | 1,687.971 ms |

All six final files matched their expected sizes and SHA-256 hashes. UDM's saved Complete records also matched. IDM connection registry values and types were restored. The [sanitized record](validation/finishing-8.53/d-lifetime-research.json) retains every case, phase timings, hashes and cleanup counts.

## What blocked the split case

UDM's published-file reopen took 0.119 ms, final flush 537.441 ms and close 0.079 ms. Its intent temporary-file flush took another 27.756 ms. This observation localizes the largest wait to the final file flush, rather than reopening, hashing or a deliberate pacing interval. It does not distinguish disk queueing, filesystem/filter processing or scheduling inside that call.

IDM exited earlier in the split case, but the separate output flush then took 1,134.417 ms, versus UDM's 19.257 ms. Therefore this run does not support treating IDM exit as the end of output writeback. It also does not establish that IDM never flushes at completion: prior observations lacked complete writer and lifetime coverage.

UDM was faster at the explicit post-exit flush endpoint in all three pairs, while still losing the split process-exit endpoint. Both results must be retained. One trial is diagnostic evidence, not a general speed-parity claim. Completion-dialog timing and system-service background activity were not measured.

## Next investigation

Obtain a bounded kernel trace that actually covers the target process and final flush before attributing the delay to a device, filter or scheduler. For user-mode write observations, resolve implicit offsets and establish file identity and completed interval coverage; matching aggregate byte totals is insufficient. Preserve the barrier ordered after successful final writes and metadata changes. Any proposed optimization must pass recovery qualification and repeat the paired measurements, retaining losing results.

## Cleanup

The six owned output payloads were verified by size, hash, resolved location, link count and absence of reparse points, then individually deleted: 276,824,064 logical bytes (264 MiB). Compact diagnostic records were retained. No personal downloads, catalogs, shared tools or qualified application binaries were removed.

## Bounded kernel-trace follow-up

Two additional attribution-only split pairs completed with correct hashes and restored IDM connection settings. They are instrumented observations, not substitutes for the uninstrumented timing table above. Both owned recorders were confirmed off after collection.

Neither kernel capture is usable for attribution. WPR's owned collector remained present until the stop deadline; the helper terminated only its own export child, canceled only its named collector, and retained an incomplete ETL. The read-only consumer decoded one record in each export, so no target lifetime, disk queue, filter or scheduling conclusion is supported. [First attempt](validation/finishing-8.53/kernel-trace-attempt-01.json) Â· [Second attempt](validation/finishing-8.53/kernel-trace-attempt-02.json).

The second attempt tested excluding module-loader events. Microsoft's [WPR stop/merge explanation](https://devblogs.microsoft.com/performance-diagnostics/wpr-start-and-stop-commands/) identifies loader data as a potential source of long merge time, but that general explanation did not establish the cause on this PC: the reduced profile still timed out. The three changed tooling files were restored byte-for-byte. The [experimental patch](validation/finishing-8.53/trace-profile-no-loader.patch), [22 experiment tests](validation/finishing-8.53/trace-tool-tests.json) and [21 restored-tool tests](validation/finishing-8.53/trace-tool-restored-tests.json) remain as evidence. No product version changed for this rejected tooling experiment.

Both trace workloads' four 64 MiB payloads and both partial ETLs were individually verified and deleted: 299,501,568 logical bytes. These bytes are additional to the first comparison's cleanup above. Repeating the same stop strategy is not the next step; a replacement capture strategy must demonstrate usable target-event coverage within the recording limit before another performance attribution claim.

## Receipt-only pruning: verified responsiveness baseline

The new [native control](../benchmarks/speed/recovery_pruning_control.cpp) links the exact 42 qualified 8.53 core objects and holds the real `temporary-flushed` catalog checkpoint while unscoped recovery prunes completed private receipts. With both one and sixteen receipts, on both C: and D:, another thread cannot acquire Model. Releasing the hold allows pruning to complete durably; every completed output retains its exact hash. This verifies a remaining blocking path independently of the finishing syscall timing. It is a causal hold, not a measurement of natural latency or GUI timing. [Four baseline cases](validation/finishing-8.53/receipt-pruning-baseline.json).

The next implementation must capture immutable receipt descriptors and journal paths, exclude conflicting publication work while checking absence outside Model, conditionally retire only those exact current receipts, and bind acceptance to a serialized catalog snapshot. A later writer must not be overwritten by rollback. New or changed receipts, new journals, active target ownership, batches, cancellation, shutdown, failed persistence and ambiguous acceptance need explicit fault cases. A bare unlock around `exists()` or `save()` is insufficient. External namespace changes remain a separate correctness gate, not something an in-process reservation automatically prevents.

All 509 qualified source hashes, 42 core-object hashes and five product hashes still match. The diagnostic fixture's 46 generated files/object/executable outputs were individually removed after retaining source and results: 7,309,440 logical bytes. The app remains source 8.53, installed 8.32; no installation, installer creation, push or release occurred.

## Zone-policy observation

An owned 8 KiB loopback IDM control produced the expected bytes without a `Zone.Identifier` stream, but the launcher reported nonzero exit and its exact exit code was not retained. Its [limited observation](validation/finishing-8.53/idm-loopback-zone-observation.json) does not establish Internet-origin attachment policy or replacement-recovery behavior. No zone metadata change was made on that basis. The owned payload was deleted.

## Direct realtime replacement probe, source 8.58

A separate direct ETW feasibility helper now bypasses WPR export/merge. It requires exact own-provider delivery and shutdown before attempting a two-flush, 8 KiB kernel fixture. Twenty normalized reader safety checks pass; access checking and nonadministrator refusal are verified. A rejected private realtime configuration (status 87, no session started) was removed. The administrator run is pending Windows approval at the linked checkpoint. No kernel-cause or finishing-speed conclusion is supported yet. Native products and installed 8.32 remain unchanged. [Implementation, limits, evidence and next gates](finishing-realtime-research-8.58.md).

The independent realtime-report validator now passes 42 synthetic acceptance-policy checks and rejects both real non-capture artifacts. An administrator-profile-dependent root guard was reproduced in preflight; a replacement is staged and passes four access cases plus twenty reader safety checks. The existing elevation request remains untouched, with no second request opened. Actual kernel capture and cause attribution remain pending. See the linked 8.58 research record for the current gates and staged patch.


The older pending-elevation statements above are historical. Subsequent owner-root, baseline/focused and normal-token captures have completed and stopped. The focused self-process probe passes; the normal-token lifetime gate currently rejects 26 undecodable FileIo opcode-84 events per fixture. Six independently contained D: flush intervals establish 20–38 ms inside successful native calls without attributing the longer download stall. A temporary hash-pinned elevated worker now batches trace launches without repeated approvals. See the current [8.58 capture follow-up](finishing-realtime-research-8.58.md) for retained failures, independent gates and cleanup.
