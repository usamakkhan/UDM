# Atomic multi-file queue changes — staged candidate

The per-record save gap from completed-file enrollment is closed for the main-window Move to queue and Remove from queue commands. Both now use shared QueueMembership.hpp. Manager::setMembership delegates to the same implementation for a single record.

The operation holds the manager lock while validating the entire selection and capturing its previous records. Duplicate pointers are handled once; an empty selection returns without saving. Only after every record passes validation are membership, destination and pending-sync fields changed. One manager.save call persists the nonempty batch. Any exception during mutation or saving restores all prior records using JSON swaps. The existing catalog persistence primitive provides the file replacement behavior; this is not new physical power-loss qualification.

## Verification

Final current-source native suite: 238 checks passed, including eight new cases covering a later invalid record, unchanged durable catalog after validation failure, blocked-save rollback of every selected record and disk bytes, duplicate selections, successful persisted removal, empty selection, and a later actually active transfer rejecting the entire selection without stopping it.

Ten actual-app checks passed using two isolated completed files: synchronization-only picker, Cancel, moving both records, preserving their completion state/bytes, keeping the destination disabled, removing both memberships and normal shutdown. The first UI attempt encountered a transient PermissionError while observing state.json during replacement; the observer now retries only temporary access/missing-file errors for up to three seconds. A fresh fixture then passed all checks. Production code was not changed to hide read errors.

[Native checks](D:/UDM-Workspace/candidates/native-084-backup/control/membership-batch-native-results.json) · [Actual-dialog checks](D:/UDM-Workspace/candidates/native-084-backup/control/enroll-batch-dialog-app-results.json) · [Acceptance](D:/UDM-Workspace/candidates/native-084-backup/control/membership-batch-acceptance.json).

QueueMembership.hpp must accompany the overlay App.cpp and FileWorkflows.cpp. Current app, host and native test executable were rebuilt. Personal catalog and installed UDM/host/monitor hashes remain unchanged; candidate remains uninstalled. Broader physical-failure, live synchronization replacement, full GUI/browser/driver fidelity and full IDM parity remain unproven.
