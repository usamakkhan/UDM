# Queue drag-and-drop — staged candidate

Scheduler previously blocked dragging every completed record even though its buttons now supported completed queue members. The handler permits inactive members, and Manager::reorder now shares queue-membership validation. Eligible completed direct files can be reordered in synchronization queues. Invalid or stale target rows are rejected instead of silently appending. Mutation and save share a rollback boundary that restores both the jobs vector and record data; same-queue reorder retains pending synchronization.

## Verification

The final rebuilt native suite passed 243 checks. Five new drop checks cover a target in the wrong queue, unchanged durable catalog after rejection, actual order-change rollback under a catalog file lock, completed same-queue reorder retaining pending synchronization, and unchanged saved bytes. These tests do not exhaust every possible stale-pointer or external-file race.

Fifteen actual-app checks passed in a fresh Scheduler profile. The test obtains row rectangles from the owned list control, queues mouse-down/move/up messages to that isolated app, and verifies the resulting reordered completed members and preserved selection. It then removes the selected member, checks membership and files, reopens Scheduler and closes normally. No physical mouse or personal app window is used. The initial harness blocked while sending a synchronous mouse-down into native drag tracking; that run was stopped, the harness changed to queued messages, and the fresh run passed. The stalled run is not counted as successful evidence.

[Acceptance](D:/UDM-Workspace/candidates/native-084-backup/control/queue-drop-acceptance.json) · [Native checks](D:/UDM-Workspace/candidates/native-084-backup/control/queue-drop-native-results.json) · [Scheduler gesture checks](D:/UDM-Workspace/candidates/native-084-backup/control/scheduler-drag-app-results.json).

Current app, host and native test executable were rebuilt. Personal catalog and installed UDM/host/monitor hashes remain unchanged. Candidate remains uninstalled. Physical drag/device behavior, cross-process OLE drop coverage, full multi-monitor/DPI rendering, all Scheduler behavior and full IDM parity remain unproven.
