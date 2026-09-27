# UDM 0.20.0 - YouTube panel repair and browser controls

Installed 25 September 2026. The native desktop, messaging host and browser extension are now 0.20.0. This release fixes the missing YouTube download button and adds working desktop browser-panel configuration. It does not establish complete IDM parity or a download-speed advantage.

## What caused the missing button

The installed extension was capturing the current video's formats, but its panel had `display: none`. YouTube's absolutely positioned application leaves the body at zero height. When the root overflow is visible, the body's scrolling applies to the viewport; UDM incorrectly used the body's zero-height box to clip the player.

The panel now follows CSS viewport overflow propagation, while retaining real container clipping, containment and advertisement checks. Fullscreen players inside open shadow roots also keep a visible panel in the fullscreen container. The relevant behavior is specified in [CSS Overflow propagation](https://www.w3.org/TR/css-overflow/#overflow-propagation).

Before/after evidence: `benchmarks/gui-0.20/youtube-layout-before.json` and `youtube-layout-after.json`. Both have a zero-height body and an 814 x 458 player. Before: panel hidden. After: visible 168-pixel panel, with no page errors in that isolated test.

## Added browser GUI workflows

Open **Help > Browser integration**, or **Options > General > Configure browser integration**:

- Enable the video panel; choose compact mode, hover-only visibility and one of four player corners.
- Set format-menu width from 260 to 640 pixels; reset saved panel positions.
- Choose force-capture and keep-in-browser modifier keys.
- Set HTTP/HTTPS address-pattern exceptions and selected-link panel mode, hosts and compact display.
- Changes reach existing pages while UDM is running; browser capture and cookie permissions remain controlled by the extension's existing opt-in.

The extension popup now offers **Restore hidden video panels** and **Open desktop browser settings**. Reloading the extension reattaches panels to already-permitted open tabs. No new manifest permission or extension identity was added.

Force capture can bypass file-extension filters. It cannot override exclusions, private browsing, disabled capture or missing permissions. Keep-in-browser is handled before native messaging, so it does not start UDM. Gestures are bound to one matching URL, expire after eight seconds and require a trusted browser click.

## Verification

- 39 browser handoff checks, seven browser policy checks, and the existing media/capture/streaming/package suites passed.
- Chrome: 19 geometry checks, nine integration controls, seven panel buttons, seven selected-link controls, four format-menu checks and six cross-origin extension checks passed.
- Edge: the same 19 geometry, nine integration-control and seven panel-button checks passed in isolated sessions. A full extension test on the same public YouTube video also displayed the button and six quality buttons, and independently verified the matching current-video catalog. See [Edge screenshot](../benchmarks/gui-0.20/msedge-youtube-isolated/panel-and-menu.png) and [result](../benchmarks/gui-0.20/msedge-youtube-isolated/result.json).
- Actual installed Chrome: extension version 0.20.0 verified; the button appears on `https://www.youtube.com/watch?v=BewnzhHlQuk`. Its menu lists 1080p, 720p, 480p, 360p, 240p and 144p. Reset position visibly returns it to the player's top-right corner, and Close leaves the button available.
- Installed desktop browser-settings dialog reviewed and dismissed without changing preferences. Native diagnostics and quiet browser preferences return successfully.
- All 15 user download records and queue data are unchanged. Paused ISO counts remain 546,308,096 and 571,867,136 bytes. No new user download was created by this verification.

The Dailymotion check uses a synthetic page/player/CDN fixture in an actual Chrome extension. It verifies cross-origin scope and actual source identification; it is not a fresh live download from Dailymotion. Isolated public YouTube smoke tests are separate from the signed-in browser and do not download a file.

## Installation and evidence

Desktop: `D:\UDM\release\UDM.exe`. Extension: `D:\UDM\browser\chromium`. Firefox files were regenerated from the same source, but Firefox was not exercised live. Native messaging registration for Edge points to the same verified host. The normal Edge profile could not be opened through the control tool (two launch attempts returned no targetable window), so its installed-extension state is not verified. The successful Edge test used an isolated profile. One earlier public-page attempt timed out; another rendered the panel before the quality menu populated. The final run verified both the populated menu and catalog.

See [deployment evidence](../benchmarks/gui-0.20/deployment.json) and the test result files under `benchmarks/gui-0.20`. The original source, browser files, binaries and history are backed up in `D:\UDM\backups\gui-0.20-20260925`. Exit UDM before restoring a backup.

## Remaining work

The native workflow additions from [0.19](gui-0.19.0.md) remain available. Offline site mirroring, SOCKS/dial-up establishment, portable full settings/schedule backup, language catalogs, external toolbar skins and a full accessibility/mixed-DPI audit are still outstanding. This release does not claim every video site, live/DRM playback, complete IDM GUI parity or faster Internet downloads.
