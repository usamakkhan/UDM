# Scheduler queue membership controls — staged candidate

Scheduler still listed every completed file associated with a synchronization queue even after membership was removed, and it disabled removal/reordering whenever a file was complete. SchedulerUi.hpp now filters completed and incomplete rows by effective queue membership, using the same legacy default as the backend. Its controls permit inactive completed members while keeping inactive nonmembers and active transfers unavailable.

The Scheduler removal action already calls Manager::setMembership, which now delegates to the shared atomic implementation from the preceding candidate. This dialog is single-selection; no separate multi-record implementation was introduced. Reordering still uses the existing saved queue order operation.

## Verification

Fifteen actual-app checks passed in a fresh private profile containing two completed members and one completed nonmember. The test opened Files in queue through keyboard tab navigation, checked membership filtering and enabled controls, moved a completed row, verified selection identity and saved order, removed it, verified the immediate row count and other membership, reopened Scheduler, and checked that all saved files remained intact. The rebuilt application closed normally.

[App checks](D:/UDM-Workspace/candidates/native-084-backup/control/scheduler-membership-app-results.json) · [Acceptance](D:/UDM-Workspace/candidates/native-084-backup/control/scheduler-membership-acceptance.json).

WorkflowsUi.hpp is an unchanged copy of the native-083 base header, required so its relative SchedulerUi.hpp include resolves to the new overlay. Both files must accompany this candidate. The previous 238 native checks predate the Scheduler overlay and were not rerun; rebuild MenuStateTests before any later native acceptance, rather than relinking its old object.

Installed UDM/native host/monitor and the personal catalog are unchanged. The candidate is uninstalled. This proves the tested control workflow, not the entire Scheduler, physical power-loss behavior, mixed-DPI rendering, remote synchronization replacement or full IDM parity.
