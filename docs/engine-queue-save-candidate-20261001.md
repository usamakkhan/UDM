# Queue Start/Stop save rollback — staged engine correction

A real catalog replacement lock reproduced a defect: failed Queue Start left the queue enabled and download metadata queued in memory despite reporting failure.

Manager::queueRun now snapshots queue definitions, job metadata and scheduler bookkeeping, restoring them when save fails. Queue Stop defers worker cancellation until after a successful save. This prevents an unsuccessful Stop from changing an active transfer while reporting failure.

Fourteen dedicated failure-recovery checks passed after the failing baseline: Start rollback, unchanged catalog, no next-tick launch, successful retry, Stop rollback, continued receiving after failed Stop, successful stop retry, and resumed completion with matching file hash. The relinked native menu/transfer suite passed 155 checks, and 18 actual app/menu/reopen checks passed.

Evidence: D:/UDM-Workspace/candidates/native-084-backup/control/queue-save-acceptance.json; queue-save-before/results.json; queue-save-after/results.json; queue-save-menu-regression/results.json; control/ui-queue-save-app.json.

Queue.cpp is now an overlay production source. The host build compiles it, and the current app/menu/failure-fixture build and relink scripts select its Queue.obj instead of the base object. A malformed compiler output argument introduced while editing the script was diagnosed and corrected before successful rebuilds. No acceptance uses the failed build.

Protected personal installed binaries and catalog remain unchanged. Source and binaries are staged, not installed or packaged. Disk-full/power-loss, nested catalog transactions, scheduled synchronization/repeat cycles and exact live IDM failure behavior remain unqualified. Full IDM parity is unfinished.
