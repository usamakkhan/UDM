# Completed synchronization queue members — candidate acceptance

The right-click audit found a functional blocker: UDM disabled removal and reordering for every completed download, even when its queue membership remained set. IDM describes retained completed files in synchronization queues in its [official scheduler documentation](https://www.internetdownloadmanager.com/articles/scheduler.html). Static menu resources did not establish the full live right-click ordering, so no unsupported ordering claim is made.

## Changes

- App.cpp enables Remove from queue and queue Up/Down for inactive queue members, including completed members. Nonmembers and active transfers remain unavailable. A legacy completed record without a membership flag is treated as a nonmember.
- FileWorkflows.cpp clears pending synchronization when removing membership, in the same saved transaction. On save failure its existing rollback restores the full prior record, including membership and SyncPending.
- Queue.cpp only schedules completed synchronization checks for queue members. Starting the queue no longer silently brings a removed completed file back into synchronization.
- New overlay FileWorkflows.cpp is built by build-recovery-host.ps1. All 17 shared-selector build/relink scripts now select the current Core, Bridge, Queue and FileWorkflows objects; selection was verified explicitly. Future rebuilds must include this fourth engine overlay object.

## Verification

The final rebuilt native suite passed 221 checks, including 12 new membership checks: availability, actual reordering, removal preserving completion and downloaded bytes, disabled nonmember actions, legacy membership handling, pending-sync clearing, save failure under a real catalog file lock, preserved state on rollback, retained members scheduling synchronization and removed members not scheduling it. Existing queue/menu, actual loopback stop/resume, and downloaded-byte checks also passed. The intermediate 220-check run passed before the retained-member positive case was added; final acceptance uses 221, not their sum.

The app and native host were rebuilt with the current backend. Installed UDM, native host, monitor and personal catalog hashes remain identical. Tests used isolated C: temporary fixtures to preserve D: working space. No app deployment, browser reload, driver or certificate change occurred.

[Acceptance](D:/UDM-Workspace/candidates/native-084-backup/control/completed-membership-acceptance.json) · [Native results](D:/UDM-Workspace/candidates/native-084-backup/control/completed-membership-native-results.json).

Full live synchronization replacement/remote-server coverage, complete context-menu order and rendering, and full IDM parity remain unverified. This change does not add arbitrary completed files to new queues; that remains a separate workflow requiring review. Candidate remains uninstalled.
