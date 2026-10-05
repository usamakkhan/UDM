# Arrange files menu — staged candidate

Read-only extraction from installed IDM 6.43 build 10 confirms View > Arrange files has eleven choices: addition order, file name, size, status, time left, transfer rate, last try date, description, save path, referer and parent web page. The executable SHA-256 matches the retained reference inventory. The extraction script and exact resource (type 4, ID 129, language 1033) are preserved.

UDM now exposes these eleven choices in View > Arrange files, using its existing column sorter. Selecting a choice sets ascending order, preserves selected records, stores the list layout and checks the current choice. Main-menu mnemonic markers have also been added to Tasks, File, Downloads, View and Help. Existing column-header sorting remains available.

Verification:
- 69 native MFC/transfer checks passed, including 28 new Arrange assertions for all command mappings, checked state, real name/size/date ordering, selection preservation and saved layout.
- Ten actual app checks passed, including menu presence, choosing Size, saving its layout, reopening and restoring the checked Size choice.
- Prior checks overlap these totals; they are not additive feature counts.
- Installed UDM and the personal catalog remain unchanged.

Evidence: D:/UDM-Workspace/candidates/native-084-backup/control/arrange-acceptance.json.
The reference resource proves labels and menu structure, not IDM's dynamic sort direction or every keyboard interaction. Exhaustive keypress, DPI, accessibility and visual comparison remain unverified. UDM currently chooses ascending order on each menu invocation; live IDM direction semantics remain to be compared. This staged candidate is not installed or packaged, and full IDM parity remains unfinished.
