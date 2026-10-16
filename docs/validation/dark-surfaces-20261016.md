# Dark dropdowns, status bar and scrollbars - 16 October 2026

The previous menu-bar update left dropdown menus and the status/scrollbar surfaces light. The user's screenshot and a fresh IDM Tasks-menu comparison confirmed those remaining differences.

MenuBarTheme now paints main-window dropdowns and context menus when initialized, retaining native labels, handles, command IDs, check state, disabled state, separators, shortcut columns and submenu routing. Popup entries are restored on uninitialization and protected against stale handle/position reuse. Duplicate access keys select/cycle instead of immediately executing; disabled commands are excluded. Submenu arrows are painted with a contrasting triangle; the completed row is clipped afterward to suppress Windows' black arrow overdraw. A live isolated menu verified single bright arrows and nested-menu navigation.

ThemeStatusBar renders its existing text against the dark palette while retaining native status metadata. List/tree controls request the Windows Explorer dark scrollbar theme. The optional per-window dark-mode export is dynamically checked on Windows 10 build 17763 or newer; no global Windows theme/security setting changes are made. Light mode and system contrast retain native status rendering.

Final validation: 444 appearance checks, 380 menu/workflow checks and 8 native app/host launch checks passed. Restricted-sandbox loopback/IPC tests failed; the same tests passed outside that sandbox. Live screenshots confirmed dark dropdowns, nested menus, status text and both scrollbars. Installed with a verified executable hash and unchanged catalog; previous app and catalog backed up.

Evidence root: %LOCALAPPDATA%/Temp/udm-surfaces-20261016. This is a scoped appearance correction, not full IDM parity.
