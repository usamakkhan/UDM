# UDM browser integration

Version **0.43.0** accompanies native UDM **0.65.0**. Edge video selection now rejects unobserved player links that require an n transform and prefers an available same-quality adaptive route. It also corrects malformed quality-label separators. See [Edge verification and limits](../docs/parity-browser-0.43.0.md). Reload the unpacked extension and refresh video tabs after updating; no new permission is required.

These are development extensions. Automatic download capture and cookie transfer are disabled initially. Right-clicking a link or selecting a discovered direct media URL explicitly sends it to UDM.

## Chrome, Edge and Chromium

1. Open the browser's extension management page.
2. Enable its development-extension controls and load the `chromium/` folder as an unpacked extension.
3. Confirm the extension ID displayed by the browser. The checked-in public manifest key gives this package the stable ID `kahfappnpjdcboccpnhinkcobcdgbdpl`.
4. Register the native host for that ID:

```powershell
.\browser\register-host.ps1 -ExtensionId kahfappnpjdcboccpnhinkcobcdgbdpl
```

5. Open the UDM extension and choose **Check desktop connection**.

The extension is already loaded and registered in the current Chrome profile. If Chrome reports that the host cannot be found after registration, fully exit Chrome through its menu and reopen it; closing a window can leave its background process running. A full exit/restart resolved that condition during this test.

For YouTube, run `setup-media.ps1` as described in the main README, reload the video page, and use **Download this video** on the player and select a detected format. UDM opens **Download File Info**: review the destination and choose **Start Download** or **Download Later**. This prompt can be disabled in UDM Settings. The menu lists only formats found in the current video metadata or actual player quality levels. Captured links must match that video; network observations additionally require the same opaque resource ID and format ID. If necessary, an explicit download request asks the player to prepare the selected available quality, then waits up to 3.2 seconds for capture. If no usable URL arrives, UDM reports this rather than invoking a resolver. There is no silent quality downgrade. See [capture status](../docs/browser-capture.md). After updating the unpacked extension, reload it and fully refresh the video page so the early observer can run.

If you installed UDM elsewhere, pass the native executable explicitly:

```powershell
.\browser\register-host.ps1 -ExtensionId YOUR_32_CHARACTER_ID -HostExecutable "$env:LOCALAPPDATA\Programs\UDM\Udm.NativeHost.exe"
```

The current script stores one allowed Chromium extension ID. Loading the same unpacked path in another browser usually retains the ID, but if its ID differs, extend `allowed_origins` in the generated manifest deliberately. Do not use a wildcard; Chrome does not allow it. [Chrome native messaging](https://developer.chrome.com/docs/extensions/develop/concepts/native-messaging)

## Firefox

Load `firefox/manifest.json` as a temporary development add-on, then register:

```powershell
.\browser\register-host.ps1 -Browser Firefox
```

The add-on ID is `udm@local.example`. Firefox 128 or newer is required for MAIN-world injection ([Mozilla announcement](https://blog.mozilla.org/addons/2024/07/10/manifest-v3-updates-landed-in-firefox-128/)). Temporary installation and regular signed distribution have different lifecycles; store signing remains outstanding. Firefox 156.0.1 passed native MP4 and HLS downloads in an isolated profile.

## Features and boundaries

The link context menu and all/selected-link commands use the canonical integration. A per-tab toggle can disable panels and handoff without changing global settings. The popup can inspect links and direct media elements in the active tab when the user asks it to. The compact player panel supports HTML5 video on granted sites, embedded frames and container fullscreen. In the popup choose **Enable panels on this site** or **Enable panels on all websites**, then accept the browser permission yourself. Drag the panel handle to reposition it. The 0.23.0 manifest requests HTTP/HTTPS page access for early video observation across sites. Cookies and captured authorization still require a separate opt-in. Media observations use session storage and are removed on navigation or tab closure. An original document-start observer retains bounded player responses by video ID. The YouTube panel matches the current video identity and resource ID before using observed streams. This guards against mixing an ad or previous video into the selected download; broad live ad coverage remains unverified.

Automatic capture only handles chosen extensions, respects excluded hostnames and skips private-window downloads. It pauses the browser download, waits for a durable UDM acknowledgement, then cancels the browser copy. A failed handoff attempts to resume it. Some servers or browser downloads cannot pause/resume; those need manual transfer.

Site cookies and captured Authorization headers are included only when the user enables that option and grants the browser's cookie/host permission. Cookies are passed through the local native host and protected with DPAPI in UDM state. They are not sent to any separate service. Cookie partitioning, browser-container identities and cross-site redirect authentication need further integration work.

Clear recorded HLS and static MP4 DASH playlists are parsed and passed to the native segment engine. See [cross-site video](../docs/cross-site-video.md) for format limits and stream-association safeguards. A blob URL alone is not downloadable; arbitrary JavaScript-generated streams and encrypted/DRM media remain unsupported. YouTube's supported public direct MP4/AAC streams are handled by the page workflow. Live videos, account-only media and OAuth refresh remain unsupported. Clear recorded HLS/DASH has selected audio and WebVTT subtitle support; YouTube uses a separate path. Ordinary POST downloads support complete bodies within negotiated limits; multipart, oversized and incomplete requests remain in the browser. The original SABR/UMP transport is experimental and tested with controlled transcripts; broad live compatibility is unverified. A rejected or expired URL requires fresh browser capture; there is no automatic external resolution fallback.

## Protocol

`com.udm.download_manager` uses UTF-8 JSON framed by a four-byte little-endian length. The host enforces an 8 MiB request limit and a 256 KiB reply limit. Ordinary captured POST bodies are limited to 4 MiB, further constrained by the bytes the browser exposes. Actions include `hello`, `diagnostics`, `preferences`, `capture-reconcile`, `ping`, `show`, `add`, `media` and `adaptive`. The adaptive action carries a bounded clear segment plan, not a page URL requiring an external resolver.

```json
{"action":"add","url":"https://example.com/file.zip","filename":"file.zip","referrer":"","cookies":"","userAgent":""}
```

```json
{"action":"media","url":"https://www.youtube.com/watch?v=Q3TI27IN7X0","filename":"Video title","height":1080,"pixelHeight":1080,"formatId":"137","exactQuality":true,"videoUrl":"https://rr1.googlevideo.com/videoplayback?itag=137&...","audioUrl":"https://rr1.googlevideo.com/videoplayback?itag=140&..."}
```

Reply:

```json
{"ok":true,"id":"saved-download-id","error":null}
```

The native host connects to a same-user ACL-protected Windows pipe, starting UDM if it is absent. There is no localhost HTTP listener, global network interception or browser DLL injection.

Run `unregister-host.ps1` to remove only UDM's native messaging registry entries. Remove the development extension in the browser separately. User download data is retained.

## Rebuild without changing capabilities

Run `node browser/prepare.cjs` from the project root to synchronize Firefox assets and compute the existing Chromium extension ID. Chromium's manifest is the canonical configuration: preparation preserves its version, public identity key, host permissions and content scripts. It does not revert to the older YouTube-only setup. Reload an already-running unpacked extension and refresh its video pages after updating.

## Recorded audio and subtitles

Use **Audio only (M4A)â€¦** on supported HLS/DASH video panels to choose an audio rendition and download it without video. MP4 offers with WebVTT subtitles show a **Subtitles** selector; **None** preserves the usual video output. These controls do not currently apply to YouTubeâ€™s separate player/SABR path. Existing developer installations must reload the extension and refresh video pages after updating.

## Browser request proxy inheritance

In UDM Options > Proxy, **Use the proxy captured with a browser download** defaults to enabled. Firefox provides the route of a completed request: direct, HTTP, or SOCKS4/5 with remote DNS. The extension passes only a validated endpoint bound to that exact download URL. UDM saves it with Windows account encryption before acknowledging the handoff, then uses it for that job and its metadata preview after restart. An approved browser link refresh can replace the route. The route is not stored in extension storage.

Chrome and Edge do not supply this request metadata through the current integration and continue using the desktop defaults. Encrypted HTTP proxies, SOCKS local DNS, ambiguous routes and browser proxy usernames are unsupported and keep automatic downloads in the browser. Proxy credentials are not imported. Explicitly saved desktop credentials can be reused only for an exact matching protocol and endpoint. A new redirect needs a fresh capture instead of silently switching routes. Adaptive media handoffs continue using desktop proxy settings.

Normal catalog exports omit the protected route and require recapture before a routed job can run; explicit protected-credential exports retain it for the same Windows account. Turning this preference off is an explicit override to use desktop proxy settings.

## Recorded DASH indexes

The extension supports clear, single-period MP4 DASH using SegmentBase whose SIDX references lead to separately ranged child indexes in the same media file. It validates child track identity, timing and byte ranges before sending the selected media ranges to UDM. Opening the format menu does not fetch indexes; only the chosen video/audio tracks are expanded. Audio-only avoids video requests.

Expansion is limited to eight child levels, 64 index reads, 2 MiB of total index data, 512 KiB per index range, 1,200 final segments and a 12-second selection deadline. Changed validators, wrong ranges, overlaps, inconsistent timing and navigation cancel the handoff. Independent top-level indexes, multiple SIDX boxes in a same-file child range, partial overlaps between fetched external index ranges, live/DRM content and arbitrary manifests remain unsupported. The index resource size and available strong ETag or Last-Modified are checked on every native initialization/media response. Only a matching strong ETag permits reuse of protected cached parts; date-only and size-only sources re-fetch parts. These weaker sources cannot detect every same-size or same-second change.

Separate RepresentationIndex resources can supply one rooted SIDX tree through a whole-file fetch or declared byte ranges. Child references may include their descendants; mixed child-index/media references retain separate byte positions. Child track identity, exact rational timing, media continuity and all fetched index boxes are checked before submission. A selected media file is probed with bytes=0-0, then its own size and validator bind the native plan. External media offsets use the media-file origin; child index references begin immediately after the containing SIDX box in the index file. Already fetched child bytes are reused only when their complete range is present, avoiding mixed overlapping snapshots. Each host needs permission; optional captured authorization is scoped to the exact index or media request. External fetches and probes share the 64-request/2-MiB/12-second budget and the 512-KiB index limit. Index redirects, invalid responses and unsupported hierarchies create no native transfer. The index file itself is never included as downloadable media.

## Site Grabber browser sign-in (0.41.0)

Start manual sign-in from Site Grabber’s Advanced authorization settings. Sign in in a regular browser window, return to the project website, open UDM’s popup and choose **Use this signed-in session**. Cookie permission is requested for this explicit handoff. Values are sent to the desktop for Windows-protected storage and are not saved in extension storage. The request expires after 15 minutes. Firefox uses the selected tab’s container; private-window sessions are not saved. Reload existing unpacked extensions after updating.
