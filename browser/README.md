# UDM browser integration

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

For YouTube, run `setup-media.ps1` as described in the main README, reload the video page, and use **Download with UDM** on the player. Choose an MP4 quality and click **Download video**. UDM opens **Download File Info**: review the destination and choose **Start Download** or **Download Later**. This prompt can be disabled in UDM Settings. The menu lists only formats found in the current video metadata or actual player quality levels. Captured links must match that video; network observations additionally require the same opaque resource ID and format ID. If necessary, an explicit download request asks the player to prepare the selected available quality, then waits up to 3.2 seconds for capture. If no usable URL arrives, UDM reports this rather than invoking a resolver. There is no silent quality downgrade. See [capture status](../docs/browser-capture.md). After updating the unpacked extension, reload it and fully refresh the video page so the early observer can run.

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

The add-on ID is `udm@local.example`. Firefox 128 or newer is required for MAIN-world injection ([Mozilla announcement](https://blog.mozilla.org/addons/2024/07/10/manifest-v3-updates-landed-in-firefox-128/)). Temporary installation and regular signed distribution have different lifecycles; store signing and live browser verification are still outstanding.

## Features and boundaries

The link context menu works without global page access. The popup can inspect links and direct media elements in the active tab when the user asks it to. The player panel uses targeted YouTube and Googlevideo host access. Optional additional host permission enables observing network responses with audio/video MIME types elsewhere. Media observations use session storage and are removed on navigation or tab closure. An original document-start observer retains bounded player responses by video ID. The YouTube panel matches the current video identity and resource ID before using observed streams. This guards against mixing an ad or previous video into the selected download; broad live ad coverage remains unverified.

Automatic capture only handles chosen extensions, respects excluded hostnames and skips private-window downloads. It pauses the browser download, waits for a durable UDM acknowledgement, then cancels the browser copy. A failed handoff attempts to resume it. Some servers or browser downloads cannot pause/resume; those need manual transfer.

Site cookies are requested only when the user enables that option and grants the browser's cookie/host permission. Cookies are passed through the local native host and protected with DPAPI in UDM state. They are not sent to any separate service. Cookie partitioning, browser-container identities and cross-site redirect authentication need further integration work.

Direct `blob:` URLs, arbitrary JavaScript-generated streams, encrypted/DRM media and generic HLS/DASH playlist assembly are not supported. YouTube's supported public direct MP4/AAC streams are handled by the page workflow. Live videos, subtitles, account-only media, browser POST downloads and OAuth refresh remain unsupported. SABR/UMP response decoding is not implemented. A rejected or expired URL requires fresh browser capture; there is no automatic external resolution fallback.

## Protocol

`com.udm.download_manager` uses UTF-8 JSON framed by a four-byte little-endian length. The host enforces a 256 KB inbound limit. Supported actions are `ping`, `show`, `add` and `media`.

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
