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

[Sanitized validation copies and provenance](validation/2026-10-05/README.md) are versioned for repository readers. Original local logs: `candidates/pause-transaction/full-build-tests.log`, `full-build-tests.stderr.log`, `scanner-rerun.log`, `scanner-rerun.stderr.log`; app smoke: `app-smoke/results.json`.

This change is source development after RC1 and has not been installed or added to the published installer. The separate R213 deployment record concerns the R212 completion UI and R207 companions, not this Pause build.


## Scanner follow-up — 6 October 2026
Forty consecutive isolated scanner-fixture launches passed, each verifying exit code zero, completion receipt and exact Unicode/special-character filename argument. A fresh focused `--scanner-checks` run against the same retained Pause candidate then passed 293 checks, including scanner-result persistence after app restart, completion-action gating and interruption recovery. Evidence: `candidates/scanner-subprocess-diagnostic/results.json`, `integration.log` and `acceptance.json`. No production changes were made. The earlier full-suite subprocess failure remains unexplained; these passing focused checks do not establish that its root cause is fixed or that the full suite passes.


## Clean full-suite follow-up — 6 October 2026
A fresh full native run against the same Pause candidate passed **2,547 checks, zero failures**, including `Scanner result survives app restart`. It used a new isolated root under `candidates/pause-full-verification/data`. The process exited zero; complete stdout/stderr and binary hashes are retained in that candidate directory. A portable summary is versioned at `docs/validation/native-full-20261006.json`. This is a clean full-suite result for this run; the earlier scanner subprocess crash was not reproduced and its original cause remains unknown. No application binary was changed or deployed during this verification. Full IDM parity and broader browser/installer/physical-GUI coverage remain unproven.


## Deployment provenance check — 6 October 2026
Deployment was withheld after the current DragUi.hpp byte hash differed from the Pause candidate source receipt. Core.cpp still matches exactly. Current Git history contains the expected completion icon changes, but the old header byte hash has not been reconciled; neither a functional change nor a formatting-only cause has been proven. A fresh source build and application/host verification are required before replacing installed programs. The retained candidate full-suite result remains valid for its recorded executable hash. Details: candidates/pause-deployment-20261006/preflight.json. No installed file was changed.
