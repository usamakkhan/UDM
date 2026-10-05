# Completed-file synchronization enrollment — staged candidate

Completed saved downloads can now join a synchronization queue through Move to queue. When any selected record is completed, the picker lists only synchronization queues; when none exist, the command is unavailable. The backend rejects ordinary queues, unknown records/destinations, active transfers, missing saved files, duplicate placeholders and unsupported capture/media/offline records. Eligible files must have a direct HTTP(S) address.

Successful enrollment preserves Complete status and file bytes, saves membership/destination, clears pending synchronization, and does not enable or start the destination queue. Starting the synchronization queue later makes the retained file eligible for checking. Failed single-record saves restore the original record.

## Verification

The final native suite passed 230 checks, including nine new enrollment cases: no eligible queue, filtered destinations, ordinary-queue rejection, duplicate/missing-file rejection, file-lock save rollback, successful enrollment, unchanged bytes and later synchronization eligibility. Eight actual-app checks passed for the queue picker, Cancel, persistence, unchanged saved file/unselected record, disabled destination and normal close. Test profiles and empty fixture files were isolated under C: temporary storage. Current app and host were rebuilt.

[Acceptance](D:/UDM-Workspace/candidates/native-084-backup/control/completed-enrollment-acceptance.json) · [Native checks](D:/UDM-Workspace/candidates/native-084-backup/control/completed-enrollment-native-results.json) · [Actual-dialog checks](D:/UDM-Workspace/candidates/native-084-backup/control/enroll-dialog-app-results.json).

## Remaining scope

The GUI still applies a multi-selection as repeated single-record saves. Batch-wide validation and one atomic save remain unfinished; a later record failure can leave earlier selected records moved. This is the next related implementation gap. Live remote synchronization/replacement, unsupported media enrollment, complete menu rendering/order and full IDM parity remain unproven.

Installed UDM/native host/monitor and all personal catalog bytes are unchanged; the candidate is uninstalled. FileWorkflows remains a required engine overlay object. build-membership-host.ps1 is an incremental workflow/host rebuild using already-built current Core/Bridge/Queue objects; use build-recovery-host.ps1 for a complete engine overlay rebuild.
