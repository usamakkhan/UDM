# Clipboard refresh performance - 16 October 2026

Code review found MainWindow::refresh opened and converted clipboard text on every timer refresh. It now uses GetClipboardSequenceNumber to reuse a successful snapshot until content changes. Explicit Paste remains a fresh read. Failed OpenClipboard, GlobalLock, and advertised-text rendering are retried without clearing the last visible offer. A zero counter disables caching, and a counter change during a read prevents caching that result. Disabling monitoring resets the cache.

ClipboardSnapshotTests covers initial/changed/empty text, failed-read retry, unavailable/change-during-read/wrapped counters, re-enabling monitoring, and 1,000 unchanged refreshes. These are injected reader/counter tests; they do not measure real Windows clipboard contention or establish comparative IDM responsiveness.

Validation complete: all 11 focused clipboard checks passed. The app built successfully and passed all 8 isolated launch/native-host checks, including byte-identical download completion while File Info awaits confirmation. Git diff --check passed. The build is staged at %LOCALAPPDATA%/Temp/udm-resume-app-20261010/release-native/UDM.exe. Launch evidence: %LOCALAPPDATA%/Temp/udm-clipboard-launch-20261016/launch-results.json. The installed app has not been replaced for this change. No matched IDM timing test or real clipboard-contention acceptance was performed.

Deployment follow-up: this change is included in the installed GUI candidate recorded in installed-grid-20261016.json; the earlier not-installed statement describes the preceding candidate.
