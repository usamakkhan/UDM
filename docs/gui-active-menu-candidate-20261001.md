# Active download menu acceptance — staged candidate

The native menu fixture now runs a 1 MiB loopback transfer, observes received bytes, checks active and active/completed mixed selections, pauses via the actual Stop command, resumes via the actual Resume command and verifies the final file hash.

All 37 assertions passed (17 prior menu checks plus 20 active/pause/resume/completion checks). Recovery, Delete and Move/Rename are disabled during transfer; Stop remains available. Pause reenables Recovery, Resume and Delete. Completion enables Open and disables Resume.

This extends verification of the existing menu correction; no production source changed in this continuation. Evidence: D:/UDM-Workspace/candidates/native-084-backup/control/menu-active-acceptance.json. Personal catalog and installed executable hashes remain unchanged.

New source-review lead: CMD_STOP loops through all selected rows when any row is stoppable, including completed records. Manager::pause changes GrabberImmediate/SyncPending and queue-cycle state even for completed records. CMD_RESUME similarly visits all selected rows before Manager::resume checks its terminal/active state. Mixed-selection action side effects need a dedicated reproduction and correction; enabled-state checks alone do not prove these actions preserve unrelated rows.

Not a live IDM comparison, a speed benchmark, complete GUI parity or release acceptance. The candidate remains staged.


Follow-up: the mixed-selection dispatch finding is corrected and covered by [41 native checks](gui-mixed-selection-candidate-20261001.md).
