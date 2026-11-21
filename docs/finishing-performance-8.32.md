# Bounded split writeback and sequential runway: 8.32 / browser 0.71.96

## Motivation and candidate

Keep the complete [8.31 comparison](validation/finishing-8.31/comparison.json), including the failed original process-exit pairs. Its sequential trial 3 spent 240.571 ms in final data flushing and 107.331 ms in recovery-intent flush/close. Split trial 2 spent 883.183 ms and 112.215 ms respectively. The separate symmetric post-exit output-flush endpoint favored UDM in all nine pairs, but does not replace the two failed original scores.

For a known 32–64 MiB fixed-volume HTTP transfer using two through eight workers, this candidate admits **one** optional early file flush after one eighth of the body has been credited. Later range completions cannot request another one. There is one joined writeback worker per eligible transfer, not a flush thread per range. Unknown lengths, other sizes and higher split concurrency retain their previous eligibility. This is a different bounded experiment from the historical two-operation split scheme disabled after poor performance.

Sequential 32–64 MiB transfers retain the three-operation schedule introduced in 8.31. Their minimum final runway changes from eight MiB to four MiB. The last request still advances earlier after a measured slow first flush, bounded to half-body. The hypothesis is that reducing the remaining dirty suffix helps final sequential flushing, while the early single split flush has enough body runway to settle before EOF. Neither result is assumed: early flushing can contend with writes, and the final operation can still stall.

Requests are coalesced under the short writeback mutex. Transfer progress does not wait for held IO. Finish stops admission, drops unstarted optional work and joins any running operation before the handle closes. Early errors remain advisory and are counted. Final published-file flush, recovery intent, final digest and catalog acknowledgement remain required. There is no detached background work, removed durability barrier or early success claim.

## Qualification and comparison plan

Add tests for one split operation at both size/concurrency boundaries, no repeat after later progress, held IO/nonblocking progress, joined teardown and advisory failure. Verify out-of-scope sizes/concurrency cannot enable split pacing. Update sequential runway tests while preserving legacy/adaptive, real-file and completed-checkpoint coverage.

All **1,901 maintained checks passed**, including 51 prefinish-writeback checks, 133 completed-checkpoint checks, 274 completion-dialog component checks and 43 live gzip/prefetch/resume checks. All 25 binaries qualified from 492 frozen inputs and 42 core objects. Transfer and Bridge rebuilt; forty unchanged core objects retained. Desktop 8.32 and both manifest files 0.71.96 are now installed with seven payload hashes and product registration verified. Both personal catalogs and shared tools remained unchanged. No approval prompt remains pending. Repeat three alternating paired cold launches for 4 MiB sequential, 64 MiB sequential and 64 MiB split against IDM. Measure actual process exit and, separately, a symmetric exclusive completed-output write-open/FlushFileBuffers/close endpoint before hashing; report complete elapsed times and all losing pairs. Verify actual early-flush counts from persisted job diagnostics. Delete only owned generated artifacts after hash/process checks.

The installed update is verified, but the comparison below does not establish a speed improvement. CLI process timing is distinct from a fresh live completion-dialog comparison. Browser JavaScript is unchanged; installed manifest files are distinct from browser runtime activation. No setup package, Git commit, push or release is requested.


## Installed comparison: November 15, 2026

The same eighteen-transfer protocol completed: three alternating cold-launch pairs per case, deterministic in-memory loopback origin, eight million bytes/s per connection, 20 ms latency and configured connections 1/1/8. Preserve every trial and both endpoint families. All independent output sizes/SHA-256 and final UDM Complete-record digests matched. All final writeback errors were zero. Every sequential 64 MiB trial executed three early flushes; every split 64 MiB trial executed exactly one. IDM connection settings were restored.

| Case | UDM median exit tail | IDM median exit tail | UDM median complete elapsed | IDM median complete elapsed |
| --- | ---: | ---: | ---: | ---: |
| 4 MiB sequential | 357.153 ms | 419.972 ms | 1.289 s | 2.396 s |
| 64 MiB sequential | 396.449 ms | 370.926 ms | 9.219 s | 9.892 s |
| 64 MiB split | 489.610 ms | 391.620 ms | 2.121 s | 3.740 s |

Original process-exit tails favored UDM in **4/9 pairs**, complete elapsed times **8/9**. The sequential and split median exit tails still lose. Losing pairs include both 4 MiB trials 2/3 (357.153/752.934 versus IDM 330.985/475.289 ms), sequential trial 2 (396.449 versus 370.926 ms), and split trials 2/3 (489.610/535.466 versus 391.620/373.523 ms). Sequential trial 3's IDM 1,534.003 ms outlier remains in the data and is not used to hide those losses.

| Case | UDM median tail through final output flush | IDM median tail through final output flush | UDM median complete durable elapsed | IDM median complete durable elapsed |
| --- | ---: | ---: | ---: | ---: |
| 4 MiB sequential | 426.198 ms | 565.420 ms | 1.352 s | 2.463 s |
| 64 MiB sequential | 457.102 ms | 1,493.473 ms | 9.280 s | 11.015 s |
| 64 MiB split | 562.273 ms | 730.045 ms | 2.193 s | 4.078 s |

The separate symmetric post-exit completed-output flush endpoint favored UDM in **9/9 tails**, but **8/9 full durable elapsed times**. **Both full performance targets remain unmet.** Split trial 3 was UDM 4.462 versus IDM 3.668 s through exit, and 4.524 versus 3.912 s through output flush. The whole-transfer loss prevents describing a shorter durability tail as overall success.

## Observed blocking and interpretation

Sequential trial 2 recorded 992.102 ms total early-flush IO (overlapped with reception), 98.641 ms final output flushing and 107.738 ms recovery-intent flush/close. Trial 3 recorded 1,132.620 ms early IO, 115.239 ms final output flushing and 282.594 ms intent flush/close. Their adaptive last-runway distances grew to 18,022,240 and 19,269,418 bytes. The split early operations took 250.765, 28.472 and 453.266 ms; final output flushes took 107.360, 157.257 and 156.679 ms, and intent flush/close 1.485, 46.927 and 113.790 ms.

These are measured portions of the path, not proof that all remaining time was catalog IO. Persisted publication counters stop before the final catalog save. The complete process-exit endpoint covers subsequent checkpoint, cleanup, integration teardown and shutdown. Shutdown source review confirms it already joins workers/publication owners and skips an unchanged catalog save only with exact model, acknowledged sequence, no pending outcome/error and a matching pinned physical catalog. No new redundant shutdown save was established, so no unsupported optimization was added there.

The unchanged 4 MiB path also slowed across trials. Existing IDM profile, competing device activity, cache state and request scheduling remain confounds. Relative to 8.31, some medians worsened and one full split transfer lost; **this experiment is not a demonstrated speed improvement**. A single split flush can still contend with writers, and three sequential flushes cannot eliminate later journal/device latency. No unchanged-build rerun was used to seek a better score. Further work needs attribution of the final catalog/teardown and actual writer/flush blocking before more heuristic changes. The old 16.7 s and 2.7 s stalls did not recur; their exact original causes remain unproven.

The symmetric final output flush does not measure every unrelated service/registry write or hardware power-loss resilience, and its favorable tails cannot replace failed original/full metrics. CLI timings do not establish live completion-dialog animation latency. Browser JS is unchanged; manifest files being installed does not establish runtime extension activation.

## Cleanup and verification

After verifying all 25 linked target-object inputs, individually removed 228 generated fixture/compiler files. After all benchmark processes exited and output size/hash checks passed, removed eighteen comparison payloads. Removed twenty hash-verified superseded 8.31 test executables while preserving predecessor product rollback files. Total: **266 files / 1,096,280,687 logical bytes**, with every planned path absent. Logical bytes are not measured physical reclaimed allocation. All 25 current binaries, 42 canonical core objects, source, personal catalogs, journals/evidence, rollback and shared tools remain. No setup package, Git commit, push or release.

[Qualification](validation/finishing-8.32.json) · [Installed hashes](validation/finishing-8.32/installed.json) · [Complete measurements](validation/finishing-8.32/comparison.json) · [Cleanup](validation/finishing-8.32/cleanup-total.json) · [Final verification](validation/finishing-8.32/final-verification.json)
