# Sequential writeback pacing: 8.31 / browser 0.71.95

## Candidate

The 8.30 installed comparison retained two slower 64 MiB sequential tails. Its final output flush measured 31.122 and 177.917 ms; recovery-intent flush/close measured 113.834 and 102.810 ms. File opens were below a millisecond. Removing a duplicate complete-catalog write did not eliminate these waits. Preserve the failed [8.30 results](validation/finishing-8.30/comparison.json).

For a known 32â€“64 MiB sequential transfer on a fixed local volume, the existing opportunistic writeback worker now admits at most three milestones: quarter-body, half-body, and eight MiB before EOF. Previously it admitted half-body and normally only two MiB before EOF. A slow first flush may advance the last milestone using the existing measured-cost/rate rule, bounded to half-body. Other sizes and multiple connections keep the previous schedule; unknown sizes do not acquire early writeback.

One worker owns all optional operations. Credited progress queues/coalesces milestones under its short private mutex; it never waits for a running flush. Finish stops admission, drops unstarted optional work and joins any operation already running before its handle closes. Optional errors are recorded and cannot replace the required final published-file flush. There is no detached writeback, new durability promise or removed recovery barrier.

The hypothesis is that earlier bounded writeback leaves more runway for sequential file IO and reduces dirty output work when journal/catalog durability begins. A flush does not freeze a prefix against later writes and does not guarantee a fast final flush. Three operations can increase contention or total elapsed time. Measure full transfer wall time alongside finishing tails; retain every outcome.

## Validation plan and status

Build desktop 8.31 and both extension manifests 0.71.95. Run maintained suites including new quarter/half/eight-MiB milestone, operation-bound, held-IO nonblocking progress, error, cancellation/teardown and handle-lifetime checks. Keep publication, recovery, final checkpoint, gzip/prefetch/resume and completion-component coverage.

After successful qualification, install verified binaries with rollback and unchanged personal catalogs/tools. Repeat three alternating pairs for 4 MiB sequential, 64 MiB sequential and 64 MiB split against IDM. Retain actual process-exit endpoints and separate symmetric completed-output flush endpoints before independent size/SHA verification. Compare complete elapsed time too. Browser JS is unchanged; manifest file installation is distinct from browser activation. Live dialog animation timing is not established by CLI comparisons.

All **1,887 maintained checks passed**, including 37 prefinish-writeback checks, 133 completed-checkpoint checks, 274 completion-component checks and 43 live gzip/prefetch/resume checks. All 25 binaries qualified from 492 frozen inputs and 42 core objects. Transfer and Bridge rebuilt; forty unchanged core objects retained. The installed files and registration now verify **8.31**, and both installed manifest files verify **0.71.95**. Seven updated files were hash-checked; both personal catalogs and shared media tools remained unchanged. An initial deployment-preparation guard caught an outdated expected test count before any installation; it was corrected to the actual 1,887 passed checks. The subsequent elevated installation completed successfully. No approval prompt remains pending. Remove owned generated payloads and unique target objects only after qualification/comparison and hash verification. Preserve canonical core, qualified binaries, source, catalogs/journals, evidence, rollback, tools and required historical fixtures.


## Installed comparison: November 15, 2026

All eighteen transfers completed with expected length and independent SHA-256, and all nine final saved UDM Complete-record hashes matched. All three sequential 64 MiB transfers executed **three** early flushes with zero errors; their total early-flush durations were 49.500, 165.342 and 306.733 ms, overlapped with body reception. Last-request runway was 8,388,608 / 8,388,608 / 9,296,263 bytes; trial 3 exercised the adaptive advancement. The benchmark includes joining this worker, final publication/checkpoint and application shutdown. Final published-file writeback errors were zero. All connection settings temporarily adjusted for IDM were restored.

| Case | UDM median exit tail | IDM median exit tail | UDM median full elapsed | IDM median full elapsed |
| --- | ---: | ---: | ---: | ---: |
| 4 MiB sequential | 166.396 ms | 815.631 ms | 1.177 s | 2.636 s |
| 64 MiB sequential | 246.421 ms | 451.068 ms | 9.060 s | 10.149 s |
| 64 MiB split | 396.898 ms | 458.016 ms | 3.019 s | 4.174 s |

Original process-exit tails favored UDM in **7/9 pairs** and complete elapsed times in **9/9**. All three median tails favored UDM, but the stricter all-pairs target is **not met**. Sequential trial 3 was UDM 593.553 versus IDM 471.710 ms. Split trial 2 was 1,296.665 versus 431.033 ms. Neither loss is omitted or redefined as a pass.

| Case | UDM median tail through final output flush | IDM median tail through final output flush | UDM median full durable elapsed | IDM median full durable elapsed |
| --- | ---: | ---: | ---: | ---: |
| 4 MiB sequential | 175.356 ms | 901.644 ms | 1.186 s | 2.699 s |
| 64 MiB sequential | 261.464 ms | 636.667 ms | 9.075 s | 10.390 s |
| 64 MiB split | 431.646 ms | 1,860.548 ms | 3.090 s | 5.576 s |

At the separate symmetric post-exit output-flush endpoint, UDM won **9/9 tails and 9/9 full elapsed times**. This endpoint includes exclusive write-open, FlushFileBuffers and close before hashing, and stays separate from actual process exit. IDM's extra output flush in split trial 2 took 1,701.948 ms; in sequential trial 3 it took 546.022 ms. These observations do not reveal IDM's internal calls, prove the flush waits were exclusively dirty bytes in these files, or establish all-system/hardware power-loss durability. Other device activity can contribute.

Sequential pairs, UDM / IDM:

| Trial | Exit tail | Tail through final output flush |
| --- | ---: | ---: |
| 1 | 246.421 / 407.436 ms | 261.464 / 463.485 ms |
| 2 | 172.155 / 451.068 ms | 188.834 / 636.667 ms |
| 3 | 593.553 / 471.710 ms | 660.059 / 1,122.015 ms |

Sequential trial 3 still spent 240.571 ms in final output flushing and 107.331 ms in recovery-intent flush/close, despite its completed three early flushes. Split trial 2 spent 883.183 ms and 112.215 ms respectively; split early writeback remains disabled, as before this change. Flush variation and later catalog/shutdown remain future work. Persisted publication timing covers only the pre-final-save prefix, so do not attribute all unmeasured tail time to catalog IO. The 16.7 s historical split stall did not recur and its original exact cause remains unproven.

Relative to the previous run, UDM's observed sequential median exit tail decreased from 419.426 to 246.421 ms and its durable median from 482.932 to 261.464 ms. This is an observed run-to-run change, **not a code-only causal estimate**: hardware load, caching, the existing IDM profile and scheduling were not fully isolated. The same-build comparison was not rerun to seek a passing score. Every slower observation is retained in [complete comparison evidence](validation/finishing-8.31/comparison.json). No universal speed guarantee or fresh live dialog timing is claimed.

## Cleanup and verification

After verifying all linked target-object inputs, removed 228 generated fixture/compiler files. After all comparison processes exited and every size/hash matched, individually deleted eighteen comparison payloads. Also removed twenty hash-verified superseded 8.30 test executables; predecessor product rollback files remain. Total **266 files / 1,096,001,246 logical bytes**, with all planned paths absent. Logical bytes do not measure physical reclaimed allocation. Preserved all 25 current qualified binaries, 42 canonical core objects, sources, catalogs/journals, evidence, rollback and shared tools. No source/media copies, setup package, Git commit, push or release.

[Qualification](validation/finishing-8.31.json) · [Installed hashes](validation/finishing-8.31/installed.json) · [Cleanup](validation/finishing-8.31/cleanup-total.json) · [Final verification](validation/finishing-8.31/final-verification.json)
