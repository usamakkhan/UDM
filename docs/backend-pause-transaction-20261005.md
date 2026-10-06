# Pause persistence and worker cancellation — 5 October 2026

Individual Pause previously changed the job and scheduler bookkeeping and signaled worker cancellation before saving the catalog. A blocked catalog replacement reported an error but left the transfer stopped and the in-memory job changed. It could also mark the queue cycle as failed, preventing a later successful completion event. Queue-wide Start/Stop already used save-before-cancel semantics.

`Manager::pause` now snapshots the job, scheduled-pause set, queue failure map, and browser capture presentations. A failed save restores these values and leaves the worker running. A successful save precedes the irreversible cancellation signal and removal of the pending offer.

Stopping an idle completed record no longer inserts a false synchronization flag or attempts an unnecessary save. Pending synchronization/individual-start state is still cleared transactionally, and an active completed-file worker can still be signaled to stop its scanner wait.

## Focused verification

The new `PauseTransactionTests` uses a real Windows file handle that permits catalog reads but blocks replacement, plus loopback HTTP transfers. It checks queued and active failures, capture metadata, actual continued incoming bytes, retry, exact resumed output, persisted pause state, completed-file no-op behavior, synchronization intent, and queue completion readiness. Completion events are inspected; no exit or power action is executed.

- RC1 backend baseline: 13 checks passed, **6 failed**.
- Corrected pause regression: **19 passed**, zero failed.
- Existing terminal-action/scanner checks: **16 passed**.
- Existing queue completion-cycle checks: **15 passed**.
- Existing idle-checkpoint checks: **5 passed**.

All **55 focused checks passed**. Local results are under `candidates/pause-transaction/build/backend-results/`; baseline run `b2cc3f10a978431abad881938cdc7acc`, corrected run `ca1e435853894fe38ba64fe6e0166f87`. The object cache was copied from the clean RC1 build before recompiling the changed Core source and focused test executables; historical release outputs were not overwritten.

## Application and broader validation

All four native programs built successfully. The isolated app/native-host scheduler smoke passed all **12 checks**.

The full native run completed with **2,546 passed and one failed**: `Scanner result survives app restart`. Its saved fixture catalog records a scanner subprocess exit code of `3221226505` (`0xC0000409`) during manual rescan, leaving an `Attention` result. A subsequent `--scanner-checks` run against the same executable passed **293 checks**, zero failed. This rerun includes the scanner persistence check and prerequisite media checks; it does not turn the earlier full run into a clean full-suite acceptance. The subprocess failure's root cause remains unconfirmed.

Logs: `candidates/pause-transaction/full-build-tests.log`, `full-build-tests.stderr.log`, `scanner-rerun.log`, `scanner-rerun.stderr.log`; app smoke: `app-smoke/results.json`.

This change is source development after RC1 and has not been installed or added to the published installer. The separate R213 deployment record concerns the R212 completion UI and R207 companions, not this Pause build.
