# Named queue submenus — staged candidate

Downloads > Start queue and Stop queue now populate queue names each time the submenu opens. Their first Start/Stop command retains the selected-queue behavior. Named commands bind to the displayed queue name, not its current list index; reordering cannot redirect an existing command. A removed queue is rejected by the existing queue engine. Ampersands in names display literally.

Each submenu supports 1,000 direct named commands; larger catalogs retain Choose queue as a fallback. This bound/fallback was implemented but not stress-tested in this turn. Toolbar and tray queue pickers retain their existing behavior.

Verification: 155 native checks passed, including seven new checks for new queues, escaped names, MFC enablement, reorder-safe named action, named Stop, deleted-queue rejection and refresh. Eighteen actual app/menu/dialog checks passed, including opening the named submenu and persisting Start/Stop changes for the intended empty private queue. Full transfer regressions remain included in the native suite; the new actual-app queue case uses an empty queue.

Evidence: D:/UDM-Workspace/candidates/native-084-backup/control/queue-submenus-acceptance.json. App rebuilt; protected personal binaries/catalog hashes are unchanged. Overlay now requires QueueMenus.hpp and ToolbarUi.hpp. The candidate remains staged.

This closes the extra-popup interaction gap. Exact live IDM dynamic menu population, very large queue catalogs, delete-and-recreate with the same name, schedule/cycle transitions and keyboard/DPI comparison remain unqualified. Static first-level menu matching is not proof of complete dynamic parity. Full IDM parity remains unfinished.
