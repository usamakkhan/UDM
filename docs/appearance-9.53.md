# Owned appearance repaints — 9.53 / extension 0.73.17

Use owned appearance snapshots for Windows system-theme and DPI repaints, with a nonblocking settings read and fallback to the last rendered state. Keep font, toolbar and layout reads coherent while leaving ordinary preference commands on their existing settings path. Correct the busy-resize fallback to inspect the real toolbar window style. Controlled system-color handling improves from 2,503.01 ms on frozen 9.52 to 15.48 ms. Build five native 9.53 products; pass 154 backend and 1,336 UI assertions on child-attested desktops. Source browser manifests advance to 0.73.17. Installed 8.68 remains unchanged; installed IDM parity and the previous total-time gate remain open.

## Settings and rendering ownership

Windows color/settings broadcasts and DPI messages must not wait for a shared catalog writer. Take the active render snapshot first, then an accepted Options snapshot, then a live settings copy only if the model can be acquired immediately. Otherwise use the last successfully rendered appearance. The lock ends before any control messages, image work, theme work or drawing starts.

A full appearance apply owns its complete settings value and installs a scoped read pointer for rendering helpers. Font creation, toolbar definitions and images, tray style and layout read the same value. Successful rendering moves it into the last-rendered cache. Nested rendering restores the outer pointer, and exceptions restore the preceding scope without leaving a dangling pointer. Ordinary preference readers and mutation commands retain their existing preference path, including the pre-existing Options snapshot priority, so a render snapshot cannot silently overwrite settings changed elsewhere.

Theme-only refresh updates just the cached dark-mode preference after repainting. It retains the last rendered font, toolbar and layout values, instead of incorrectly caching unrelated settings that have not appeared on screen. High-contrast state is evaluated live; the stored preference remains separate from the resulting palette. DPI repaint reads fresh committed settings when immediately available, and an accepted Options snapshot has priority even while Manager is busy. No catalog write is performed by these paths.

## Toolbar visibility defect

CToolBarCtrl::GetStyle sends TB_GETSTYLE. The local MFC SDK implementation at afxcmn2.inl lines 55–56 establishes that API distinction. Its toolbar-control style retained the creation-time WS_VISIBLE flag after the actual window was hidden. Using that value for a busy-layout fallback could reshow a hidden toolbar.

Use GetWindowLongW(hwnd, GWL_STYLE) for the actual window style. The qualified fixture records storedToolbarStyleVisible=true and actualWindowStyleVisible=false, then holds Manager and resizes the frame. The toolbar stays hidden. This also corrects the initial diagnostic assertion, which queried the wrong style API. Failed diagnostic records are retained as failures; they are not counted as passing qualification.

## Qualification and measured limits

All 154 backend assertions passed across eight suites. All 1,336 optimized UI assertions passed: 36 appearance checks, 469 main-menu checks and 831 modal workflow checks. The appearance fixture checks real Windows messages, a real modeless companion's theme, busy/fresh setting priority, font scaling, consistent toolbar/category visibility, custom and hidden column widths through a DPI roundtrip, Options ownership, failure cleanup, nested scope restoration, live-reader independence and absence of catalog writes.

The same system-color event and setup on frozen 9.52 takes 2503.0097 ms and returns after its 2.5-second model writer expires. On 9.53 it returns in 15.4806 ms while that writer is still held. Busy DPI repaint returns in 122.8656 ms, and busy resize with a hidden toolbar returns in 0.9724 ms. These single fixture observations are internal regression evidence, not a hardware benchmark distribution or an installed IDM comparison. DPI still rebuilds images and fonts; this does not establish zero latency or remove all rendering IO.

Each qualified test self-attests its own desktop name before UI initialization. Explicit native launch assignment and child names match, with no desktop switch or physical input injection. The diagnostic runs use Od and old 9.52 core objects and are excluded from qualification counts. Final current fixtures use normal O2 flags and the frozen 9.53 core set. All final production and test source hashes belong to one 659-file inventory. Baseline App, MainFeatures, ToolbarUi and Version inputs are hash-verified 9.52 copies. The baseline uses the initial fixture; its measured prefix and application setup are identical to the final fixture, whose later current-only checks add visibility observations and scope checks.

## Remaining work

Font/tray/theme command mutations and other synchronous preference paths still need ownership work. Image/font caching during DPI repaint, startup behavior, real-network limiter accuracy, installed/browser IDM comparisons, standard build profiles for standalone fixtures, the earlier total-time gate and complete inside/outside parity remain open. No installer, installation, commit or publication was performed. The installed app remains 8.68 with extension files 0.72.32; browser source versions advance mechanically without a capture behavior change.

Sanitized inventories, compiler records, assertions, diagnosis, baseline reconstruction, ownership proof, cleanup and preservation: [validation status](validation/appearance-9.53/status.json).
