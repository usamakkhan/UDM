# Separate Pause all and Stop all — staged candidate

Installed IDM menu resource 4/129/1033 exposes separate Pause All and Stop All commands. Its official main-window documentation says Stop All stops downloads in progress, and distinguishes Pause from Cancel, which stops and closes the download dialog: https://www.internetdownloadmanager.com/support/main.html.

UDM now exposes both commands in Downloads and the tray. Pause all preserves progress windows. Stop all pauses active/queued work, closes unfinished progress windows, preserves completed windows/records, and removes pending progress-open events for the stopped jobs. The toolbar's existing Stop All button now routes to this behavior without changing its persisted toolbar index. Stop all is disabled when only completed windows remain.

Verification: 101 native checks passed (11 additional checks over the preceding 90), including real transfer pause/stop/resume with exact final file hash, progress-window lifetime, toolbar routing, and completed-record preservation. Thirteen actual app/menu/dialog/restart checks passed, including limiter regression after the menu insertion. The earlier 99-check intermediate run is retained. Protected personal binaries/catalog hashes are unchanged; candidate is staged, not installed.

Evidence: D:/UDM-Workspace/candidates/native-084-backup/control/stop-all-acceptance.json.

The resource confirms separate commands; treating Stop All as closing unfinished progress windows is an interpretation of the documented stop/cancel semantics, not a fresh live IDM observation. Multiple simultaneous transfer timing, live-media stop/finalization, modal child dialogs and scheduler transitions remain unqualified for this command. Selected-file Stop still uses its earlier behavior and needs a separate dialog-lifecycle review. Full IDM parity is not established.

Build overlay now includes ToolbarUi.hpp as well as App.cpp and Core.cpp. Diagnostic LimiterTrace files must not be packaged. Older positional menu test scripts refer to limiter index 9; this candidate's new script uses index 10. Historical scripts/evidence were retained.
