# File menu alignment — staged candidate

File now matches the installed IDM English resource's four commands, including labels, mnemonic markers and ordering: Stop Download, Remove, Download Now, Redownload. These route to the existing Stop, remove-record, resume and redownload implementations.

The nine additional former File-menu actions remain in the download context menu: recycle, properties, refresh address, progress, open, open folder, open with, ZIP contents and move/rename. The same fillDownloadMenu function builds the actual context menu and the test fixture's menu, so tests inspect actual generated menu state rather than only command predicates.

Verification: 169 native checks passed, including 14 new File structure/route and context-action-preservation checks. Existing active, paused, completed and mixed-selection checks now inspect the context menu for commands no longer in File. Nineteen actual app checks passed, comparing File labels/positions directly with the extracted IDM resource and retaining queue/limiter/reopen coverage.

Evidence: D:/UDM-Workspace/candidates/native-084-backup/control/file-menu-acceptance.json. App rebuilt; protected personal binaries/catalog hashes remain unchanged. Candidate remains staged.

This establishes menu labels, ordering and preserved routes, not every IDM command semantic, context-menu visual arrangement, mnemonic keyboard interaction or DPI behavior. The prior whole-menu audit is a historical snapshot; File's placement gap is now addressed, while Tasks, View and Help remain open. Full IDM parity is unfinished.
