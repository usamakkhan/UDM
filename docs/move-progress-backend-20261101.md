# Move/Rename progress and cancellation backend — 1.10.0

Manager::relocate now accepts an optional cancellation token and a progress observer. Existing callers remain source-compatible. Progress events distinguish preparation, source verification, copying, verification of the original after a failed copy, destination verification, history saving, rollback and completion. Hash checkpoints report actual read position; Windows copy callbacks report transferred bytes. Observer exceptions are isolated, and observers must be nonblocking and must not re-enter Manager.

Cancellation is checked before work, during source hashing, before starting the move and during Windows cross-volume copy callbacks. A cancelled copy uses PROGRESS_CANCEL. UDM reports RelocationCancelled only after verifying that the original location is intact and the journal can be cleared. Uncertain outcomes retain the recovery journal. After Windows reports a completed move, late cancellation does not abandon verification, durable history saving or rollback; these phases are marked non-cancellable.

The Windows contract is documented in [MoveFileWithProgressW](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-movefilewithprogressw) and [LPPROGRESS_ROUTINE](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nc-winbase-lpprogress_routine). This uses supported Windows APIs and does not claim to reproduce IDM's private implementation.

This increment supplies the backend contract only. The existing desktop dialog still invokes the synchronous default call; it does not yet display progress or offer Cancel. Worker lifetime, manager-lock reservation, destination reservation and responsive UI integration remain necessary. Recovery before the operation and post-move verification may also take time without accepting cancellation.

All five native production binaries built successfully and report 1.10.0. Build-start source hashes remained unchanged. Runtime tests, installer creation, installation and publication remain deferred. Browser source remains 0.63.84; installed UDM remains 1.6.0 / browser 0.63.82. See [build record](validation/move-progress-backend-build-20261101.json).
