# Tasks menu alignment — staged candidate

Tasks now matches the installed IDM English resource's eleven first-level positions, labels and mnemonic markers. Import/export reference formats keep their first two routes and matching captions. UDM catalog formats remain additional submenu entries. Backup and recovery moved to Tasks > Export > Backup and recovery. Ctrl+N remains implemented even though its label is no longer appended to the reference menu caption.

Verification: 200 native checks passed, including 15 added Tasks shape/label/route checks and active-transfer Recovery gating. Twenty-two actual app checks passed, comparing Tasks, File, Downloads and View first-level structures with the resource (UDM branding substituted in View), using the nested Recovery route and reopening the original private profile.

The first native run reported Recovery enabled during an active transfer because the old harness initialized only the parent Tasks popup. The corrected harness locates and opens the command's actual containing submenu before querying state. The unchanged production code then passed. Both runs are retained; the failed harness run is not acceptance.

Evidence: D:/UDM-Workspace/candidates/native-084-backup/control/tasks-menu-acceptance.json. App rebuilt; protected personal binaries/catalog hashes are unchanged. Candidate remains staged, not installed or packaged.

The first four main menu structures now have reference-backed coverage. Help, dynamic submenu details, visual/DPI rendering, keyboard behavior and broader dialog/backend parity remain unfinished. Additional UDM catalog/recovery routes deliberately remain within submenus. Full IDM parity is not established.
