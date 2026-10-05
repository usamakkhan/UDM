# Queue deletion and mode changes — staged candidate, 1 October 2026

The retained baseline reproduced six failures: synchronization members moved to an ordinary queue, stale SyncPending state remained, queued work remained ready to run after reassignment, compatible reassignment did not survive restart, deleting the final synchronization queue left invalid completed membership, and switching a synchronization queue to ordinary mode accepted completed members.

Deletion now prefers another queue of the same type, prioritizing its primary queue. When no synchronization destination remains, completed files retain their saved content and Complete status but leave queue membership. Queued unfinished work is paused on reassignment; nonmembers remain nonmembers. Old pending-cycle state is cleared. Runtime scheduling/completion entries for the deleted name are removed after the catalog save succeeds, and wake scheduling is refreshed. Failed saves restore the queue list and all affected records.

A synchronization queue containing completed members cannot be changed to download mode until those memberships are removed. This enforces the completed-file restriction documented in [IDM’s queue guide](https://www.internetdownloadmanager.com/support/idm-scheduler/idm_queues.html). Exact IDM deletion fallback and primary-queue protection rules remain unobserved; this is not a claim of full deletion parity.

## Verification

- 15 focused deletion checks passed, including compatible reassignment, final-sync removal, saved-file bytes, restart, mode-change rejection, a real Windows file-sharing lock causing save failure, recovery after that lock, and recreating a deleted name without inheriting manual-start state.
- 17 default-queue migration checks passed again against the changed engine.
- 349 accumulated native GUI/transfer checks passed. The completed-file fixture now explicitly clears membership when marking an ordinary download complete, matching the real completion path rather than constructing an inconsistent completed member.
- 26 actual-app checks passed in an isolated profile, including deletion through the queue command/confirmation dialog, primary-sync reassignment, deleting the final sync queue, preserved saved bytes and restart.
- Core.cpp and Queue.cpp rebuilt; native host, app and GUI regression executable linked with the new engine objects. The app UI object itself was unchanged.
- Installed executables and personal catalog retain their protected hashes.

Evidence: `D:\UDM-Workspace\candidates\native-084-backup\control\queue-deletion-acceptance.json`, `queue-deletion-baseline-results.json`, `queue-deletion-model-results.json`, `queue-deletion-default-results.json`, `queue-deletion-native-results.json`, and `queue-deletion-app-1-results.json`. Reproducible sources include `native\QueueDeletionTests.cpp` and `control\queue-deletion-app-1.py`.

## Limits

Staged, not installed. Live IDM deletion behavior, primary-queue protection, project/template references, physical power loss, and exhaustive schedule lifecycle cases are not established. The broader parity gaps remain open. No production download, browser, driver or certificate changed.
