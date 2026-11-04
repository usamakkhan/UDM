# Direct tray queue controls

Development version 1.18.0; browser source remains 0.63.85.

The tray context menu now exposes Start queue and Stop queue submenus containing named queues directly. It shares the existing main-menu command mappings, ampersand escaping and manager queue operations. Stop entries are disabled when there is no running/scheduled work that can be stopped; execution rechecks the manager state.

The IDM main-menu reference already records named queue commands in docs/gui-parity-2026-10-18/idm-main-menu.txt. Current UDM main menus were already refreshed dynamically in QueueMenus.hpp; the old GUI audit description of their placeholder entries was stale. This change extends that direct selection to the tray. It does not claim that the IDM tray was freshly observed. Official [IDM queue documentation](https://idm.internetdownloadmanager.com/support/idm-scheduler/idm_queues.html) describes named queue control across menus and toolbars.

Queue popups now have a bounded 1,000-command range. Beyond that size, toolbar and overflow actions open an editable queue chooser rather than generating IDs that can overlap other commands or exceed Windows command limits. The chooser accepts an existing full queue name, rechecks existence before execution, updates Stop availability while open and supports Cancel. Main and tray menus retain their first 1,000 entries plus Choose queue for the remainder.

Build status: passed for all five production targets. Embedded versions and SHA-256 hashes are recorded; source hashes matched the pre-build snapshot. Runtime/visual tests, installer creation, installation and publication remain deferred. Installed UDM remains 1.6.0. See [build receipt](validation/tray-queue-controls-build-20261104.json).
