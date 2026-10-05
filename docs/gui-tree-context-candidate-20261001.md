# Tree context menu targeting — candidate acceptance

The keyboard audit found that the queue context-menu route already exists. IDM documents opening it by right-clicking a queue or pressing the keyboard context-menu key: [official scheduler documentation](https://www.internetdownloadmanager.com/articles/scheduler.html). No missing accelerator was invented based on the absence of an accelerator resource.

Two UDM problems were fixed in the native-084 overlay:

- DesktopUi.hpp treated every negative screen X coordinate as a keyboard request. A right click on a monitor left of the primary could therefore use the old selection instead of the clicked tree item. Only the Windows (-1, -1) sentinel now enters the keyboard path. It ensures the item is visible and checks its rectangle before positioning the popup. An off-tree mouse point is ignored.
- App.cpp read item data for an empty tree selection. The new real-control test reproduced a diagnostic modal when clearing selection. The TVN_SELCHANGED handler now ignores a null new item before querying it.

## Evidence

All 209 rebuilt native checks passed, including nine new real-control cases: tree items/geometry, negative-coordinate mouse targeting, preservation and anchoring of keyboard selection, an off-tree click, and no-selection keyboard handling. Existing menu, queue, stop/resume and byte-integrity checks also passed. Both the test executable and candidate app were rebuilt from the final source. DesktopUi.hpp is a new overlay file and must accompany App.cpp when promoting this candidate.

The initial run stalled at a diagnostic modal and timed out; it is not counted as a pass. A subsequent instrumented run recorded eight successful context checks before the null-selection failure. After fixing the handler, the final run completed with 209 successful checks. The native harness now records progress.json checkpoints to make future failures diagnosable.

[Acceptance manifest](D:/UDM-Workspace/candidates/native-084-backup/control/tree-context-acceptance.json) · [Final checks](D:/UDM-Workspace/candidates/native-084-backup/control/tree-context-native-results.json) · [Pre-fix checkpoints](D:/UDM-Workspace/candidates/native-084-backup/control/tree-context-before-null-fix.json).

## Limits

These tests move a real test window to negative screen coordinates; they do not establish visual fidelity on a physical multi-monitor setup, mixed-DPI behavior, every keyboard shortcut, or full context-menu parity. Installed UDM, its native host/monitor and all 25 personal records remain byte-identical. Candidate changes remain uninstalled. Full IDM parity remains unproven.
