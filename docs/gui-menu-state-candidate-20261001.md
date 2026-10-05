# Native menu-state correction — staged candidate

The recovery command had been added after the end of the MFC command-update range. Opening Tasks caused MFC to disable it, although direct WM_COMMAND calls succeeded. Earlier handoff tests used direct invocation and did not cover opened-menu state.

The baseline reproduction returned menu state 3 (disabled/grayed). The corrected range includes CMD_RECOVERY. The rebuilt app returns state 0 (enabled), then successfully opens recovery and returns to the same private profile. Recovery's shared command predicate also rejects active transfers.

Verification:
- 17 actual MFC menu-update checks passed for empty, paused, completed and mixed inactive selections, covering Properties, Open, Resume, Delete, Move/Rename and Recovery.
- Six rebuilt-app checks passed, including the exact menu-update path that failed in the baseline and the full recovery handoff.
- No live active-transfer menu case was exercised in this continuation.
- Personal catalog and installed app/host/monitor hashes remain unchanged.
- No claim of complete IDM visual or keyboard parity follows from these checks.

Evidence: D:/UDM-Workspace/candidates/native-084-backup/control/menu-state-acceptance.json.
Baseline and fixed results are retained side by side. Candidate remains uninstalled and unpackaged. Full native/GUI parity and release integration remain incomplete.


Later active-transfer verification: [37 native menu and real pause/resume checks](gui-active-menu-candidate-20261001.md). Mixed-selection action side effects remain a separate open finding.
