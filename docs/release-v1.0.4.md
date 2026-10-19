# UDM 1.0.4

UDM 1.0.4 strengthens browser proxy routing and desktop download-list behavior. It pairs the desktop update with browser integration 0.63.20.

## Changes since 1.0.3

- **PAC-aware browser capture:** Inline browser proxy scripts are evaluated with route selection, helper functions, cancellation, and bounded execution. The selected route is handed to the desktop without changing the browser's request context.
- **More reliable handshakes:** Chromium and Firefox native messaging now have broader validation and diagnostics for proxy and browser context handoff.
- **Clearer desktop lists:** Download columns and stream ordering behave more predictably, with focused checks for visibility, titles, and sorting.
- **Stronger file recognition:** Content inspection handles Unicode and compound file-name cases more consistently.

The release is distributed as one Windows setup executable containing the desktop application, browser integrations, driver runtime, and bundled media tools.
