# Backend test matrix, 2026-10-12

Follow-up on 2026-10-13: all four nonpassing or inconclusive cases below passed in individual focused runs against commit `653be44` using isolated C: data. The original-response runner now omits the positional data argument that its test interprets as a host; the media case had FFmpeg and FFprobe available. [Four-case results](backend-four-rechecks-20261013.json). A subsequent combined focused run passed all **33 current backend cases** with 1,346 reported checks; `StartupPerformanceTests` reported passing measurements and three internal identity checks but no conventional check count. [Sanitized matrix](backend-matrix-20261013.json). The separate full native suite was not run. The original snapshot remains below for history.

The current local candidate was exercised through all 32 entries in
`native/backend-tests.json`, in disk-limited batches. The latest conclusive
result for each case is 28 passing cases (827 reported checks) and four cases
without a passing result. The real-window menu harness had passed 370 checks
on this commit before the matrix run. The full `NativeSuite` was not built or
run during this pass.

| Case | Result | Detail |
| --- | --- | --- |
| `SyncProbeTests` | Fail, repeated alone | The first local HTTP download fails with Windows error 12152; the fixture then times out at `truncated range` before assertions. |
| `OriginalResponseTests` | Fail | 191 checks pass; one exception reports `Invalid original HTTP response address.` |
| `RecoveryIoTests` | Inconclusive | Fixture copy fails with Windows error 112 (disk full). |
| `LiveSubtitleTests` | Inconclusive | Initial run lacked media helpers; a rerun with FFmpeg available exhausted drive D before it could write a result. |

`StreamingContinuationTests` and `LiveAudioMetadataTests` passed on rerun
with the release FFmpeg tools available. `StartupPerformanceTests` also passed
after its optional label argument was made compatible with the consolidated
runner. The runner now accepts a final stdout pass/fail count for the five
fixtures that do not write `results.json`.

The detailed, untracked test logs and per-batch summaries are under
`candidates/ui-refresh-perf/build/backend-results/`. This record does not
claim that the full backend or native suite passes.
