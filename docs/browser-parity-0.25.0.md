# UDM browser integration update — 26 September 2026

Native UDM 0.25.0 and extension 0.23.0 share one browser implementation across Chromium and Firefox. This update implements independently written counterparts to observed IDM integration behavior. It does not establish 99% IDM parity, universal video support, or identical performance.

## What changed

| IDM integration behavior | UDM implementation and evidence |
|---|---|
| Persistent browser-to-desktop connection | Versioned native port, correlated acknowledgements, reconnect on the next action, separate player-retrieval lanes. Real host framing and Chrome/Edge/Firefox downloads passed. |
| Browser request context reaches the downloader | Exact URL/tab/frame/document correlation; Accept, language, Origin, Referer and User-Agent. Cookie and Authorization require the existing authenticated-download opt-in. Real Chrome/Edge downloads requiring Authorization and Origin passed SHA-256 comparison. |
| Early observation of playback | Bounded, document-start fetch/XHR observation of clear HLS/DASH responses on HTTP/HTTPS sites. Captured response bodies can avoid an extension refetch rejected by the server. |
| Player-adjacent format menu | Existing compact, movable panel, resolution/container/bitrate choices, Download all, refresh/reset/hide, full-screen and embedded-player handling. Duplicate child HLS playlists no longer appear as unknown-quality alternatives. |
| Download selected/all page links | Context-menu commands open a filterable, checkable link-selection window. Real Chrome/Edge submission saved two Download later jobs. |
| Disable integration on a tab | Session-persistent toggle suppresses video/selection panels and handoff; verified across page refresh. |
| Ordinary download interception | Browser transfer is paused, accepted by native UDM, then cancelled only after acknowledgement. Failures attempt browser resume. Real Chrome/Edge checks passed. |
| Force/bypass shortcuts and file rules | Existing Alt/Ctrl behavior, configured extensions and site exclusions retained; actual browser gesture tests passed. |
| Multiple browser packages | Chromium is canonical; Firefox generation copies the same scripts while using its supported background format and fixed add-on ID. Both package identities remain unchanged. |

Native header validation excludes transport-owned headers such as Host, Range, Content-Length and Connection. Redirects drop credentials and Origin across origins. Adaptive media headers are scoped to origins in the validated segment plan. Native state encrypts captured credentials with current-user Windows DPAPI. Request-context buffers are bounded and expire in memory.

Lost native acknowledgements are not retried automatically: the download might already have been saved. Older one-shot host clients remain supported. Browser downloads that require an original POST are declined explicitly instead of being silently converted to GET.

## Validation

- **453 native checks passed**, including downloads, pause/resume, state recovery, headers, queues and the native pipe.
- **7 actual native-process protocol checks passed**: persistent frames, fragmented input, one-shot compatibility, command errors, malformed JSON, oversized messages and truncated input.
- **Chrome 153.0.8010.53 and Edge 154.0.4258.37**: real extension/native MP4 and HLS downloads, authenticated byte-range downloads, link selection, per-tab controls, capture, force/bypass gestures and iframe navigation passed in isolated profiles.
- **Firefox 156.0.1**: actual temporary MV3 add-on installation, native connection, trusted MP4/HLS panel clicks and persistent connection passed. The MP4 matched the fixture hash.
- Browser regression suites cover capture, stream identity, current-video format selection, Dailymotion association, native transport handoff, manifest preparation and early response observation. The video-menu tests check eight entries in a small viewport, bulk handoff and error recovery. Selected-link tests check per-tab hide/restore and selection boundaries.

Local tests use synthetic media and separate desktop history. They do not establish live YouTube/Dailymotion success for every session. An initial `.bin` fixture request was intercepted before reaching its server; using a test-only suffix resolved the interference. No production browser protection was disabled.

## Browser support and remaining gaps

Chrome, Edge and Firefox have actual integration evidence. Brave, Vivaldi, Opera and other compatible Chromium browsers use the same Chromium folder; this release has not been exercised in each of those browsers. Registration adds explicit Brave and Vivaldi entries as well as Chrome, Edge and Chromium. Firefox remains a temporary/development add-on until Mozilla signing and distribution are completed.

IDM's desktop-supplied site rules and native site parsers are broader than UDM's present implementation. UDM does not yet implement arbitrary POST-body replay, cookie-container/partition identity, every site-specific player, live/DRM downloads, or a signed store/update channel. Its existing YouTube native retrieval/SABR paths remain subject to the previously documented live-service limitations. No claim of matching IDM's transport or speed on all sites is made.

The general panel offers clear recorded streams associated with the current player/frame. Unknown embedded ad systems cannot be classified perfectly. Existing YouTube and Dailymotion identity checks are retained; broad all-site ad equivalence is unverified.

## Install/update

Reload the unpacked extension in each installed Chromium browser, then refresh existing video pages. Load `browser/firefox/manifest.json` temporarily through Firefox's developer extension page. Run `browser/register-host.ps1 -Browser Both -ExtensionId kahfappnpjdcboccpnhinkcobcdgbdpl` for this Windows user. Keep `release/UDM.exe` and `release/Udm.NativeHost.exe` together. The extension version is 0.23.0; native diagnostics report 0.25.0.

The new all-site observers request HTTP/HTTPS page access for video coverage; automatic file capture and authenticated cookie/Authorization transfer retain their separate opt-ins. Per-site exclusions and the new tab switch remain available.

## Technical references

[Chrome native messaging](https://developer.chrome.com/docs/extensions/develop/concepts/native-messaging) documents persistent ports and host registration. [Mozilla connectNative](https://developer.mozilla.org/en-US/docs/Mozilla/Add-ons/WebExtensions/API/runtime/connectNative) documents the Firefox port API. [Mozilla's Firefox 128 update](https://blog.mozilla.org/addons/2024/07/10/manifest-v3-updates-landed-in-firefox-128/) documents MAIN-world injection. [Bitwarden's native host registration source](https://github.com/bitwarden/clients/blob/main/apps/desktop/src/main/native-messaging.main.ts) provides an independent deployed example of the Brave/Vivaldi Windows registry locations.
