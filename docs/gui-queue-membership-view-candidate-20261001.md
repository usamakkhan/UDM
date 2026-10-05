# Queue membership display — staged candidate, 1 October 2026

Removing a file from a queue previously left it visible in that queue’s download list because the list filtered only the remembered queue name. The Q column and its sort order also used that remembered destination for nonmembers. This contradicted the membership behavior described in [IDM’s queue documentation](https://www.internetdownloadmanager.com/support/idm-scheduler/idm_queues.html).

The native list now checks QueueMember, with the existing legacy fallback (unfinished records are members; completed records are not). Removed and ordinary completed files disappear from queue views while remaining in All Downloads. Explicit completed synchronization members remain visible. The Q column clears for nonmembers and sorts by its displayed membership value. The destination name is retained in the record for convenient re-enrollment.

The context menu and destination dialog now say Add to queue / Add for a selection without members, and Move to queue / Move when any selected file is already a member. Both routes retain atomic batch validation and the synchronization-only restriction for completed files. Cancellation leaves records unchanged.

## Evidence

- Baseline native run failed: Queue view excludes completed records that only remember the queue name.
- Corrected native run: 285 checks passed, including eleven new membership/display/selection checks and the accumulated GUI/transfer regressions.
- Actual staged app: 13 checks passed in an isolated profile, covering real Q cells, queue tree selection, removal, re-enrollment through the Add dialog, Move cancellation, normal close, and persistence after restart.
- Installed UDM/native-host/monitor hashes and the personal catalog remain unchanged.

Evidence is in `D:\UDM-Workspace\candidates\native-084-backup\control`: `queue-view-acceptance.json`, `queue-view-baseline-results.json`, `queue-view-native-results.json`, `queue-view-app-1-results.json`, and the reproducible `queue-view-app-1.py` harness. Source changes are in `native\App.cpp` and `native\MenuStateTests.cpp`; staged executable is `build\UDM.RecoveryCandidate.exe`.

## Remaining scope

Not installed. Full IDM parity remains unestablished. The Q column still uses UDM’s existing text presentation, including the blank Main queue label; IDM’s distinct main/additional, download/synchronization, and scheduled queue glyphs remain a confirmed visual gap. The complete context-menu order, all dynamic entries, and arbitrary DPI/accessibility cases are not qualified by these tests. No production downloads, browser session, driver or certificate was changed.
