# Mixed-selection Stop/Resume correction — staged candidate

Stop and Resume previously dispatched to every selected record whenever any selected record enabled the command. That could call pause on completed records or require fresh media capture for a completed record selected alongside an unfinished download.

The candidate now shares per-record eligibility rules between menu enablement and command execution. Stop dispatches only to active or queued records. Resume dispatches only to inactive records that are neither completed nor already queued.

All 41 native menu/transfer checks passed. Four new assertions prove that mixed Stop preserves completed metadata and file bytes, and mixed Resume skips a completed media record and preserves its file. The real unfinished loopback transfer still pauses, resumes and completes with an exact hash. Earlier menu and selection checks also passed. These counts overlap previous acceptance; they are not 41 newly implemented features.

The native application rebuilt successfully. The tested MFC fixture includes the same App.cpp implementation. Personal catalog and installed app/host/monitor hashes remain unchanged; the fix remains staged and uninstalled.

Evidence: D:/UDM-Workspace/candidates/native-084-backup/control/mixed-selection-acceptance.json.
This closes the specific mixed-selection dispatch finding from gui-active-menu-candidate-20261001.md. It does not establish every concurrent completion race, batch error flow, GUI action, visual equivalence or full IDM parity. Broader acceptance and release integration remain pending.


Engine entry-point guards now also protect active/completed records from stale caller decisions: [before/after verification](engine-terminal-actions-candidate-20261001.md).
