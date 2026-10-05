# Downloads menu placement — staged candidate

The Downloads menu now matches the 15 first-level positions in installed IDM menu resource 4/129/1033, including labels, mnemonic markers and separators. Find/Find Next precede Scheduler; Start queue and Stop queue are submenus; Speed Limiter and Options follow the reference separators.

Each queue submenu keeps its direct selected-queue command first and provides UDM's existing Choose queue route second. The added chooser is deliberately retained functionality; exact IDM dynamic queue population is not established by its static resource.

Verification: 148 native checks passed, including 24 new structure/separator/route checks. Fifteen actual app checks passed; the app harness reads its menu and compares every first-level label/position directly with the saved IDM resource extraction, and exercises limiter dialogs/reopening. This is menu structure verification, not pixel-level/DPI or keyboard navigation proof.

Evidence: D:/UDM-Workspace/candidates/native-084-backup/control/downloads-layout-acceptance.json. The app rebuilt successfully. Protected installed binaries/catalog hashes remain unchanged. Candidate is staged, not installed or packaged. Full IDM parity remains unfinished.

The current limiter submenu is at Downloads position 12. Historical positional test scripts are retained as prior evidence; downloads-layout-app.py is the matching current script.
