# Backend test matrix, 2026-10-12

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
