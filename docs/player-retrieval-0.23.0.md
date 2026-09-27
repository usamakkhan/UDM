# UDM 0.23.0: native player retrieval and browser integration 0.21.0

UDM can now request YouTube player metadata from its native C++ host and use a validated direct video/audio pair with the existing parallel range downloader. The browser starts a bounded prefetch when its format list is read. A download selection waits at most 1.2 seconds for it, then retains the existing browser-captured transport. No yt-dlp dependency was added.

**The current live video did not succeed through this new path.** Before the last two request fields were aligned, YouTube returned HTTP 200 with playability `LOGIN_REQUIRED`, zero formats and no matching video details in 1,002 ms. The final probe with all observed fields timed out at the eight-second budget (8,307 ms including host overhead). An earlier probe also timed out. These outcomes are saved separately. This is not proof of IDM transport parity or a speed improvement. UDM's existing SABR capture remains the fallback; no fresh full-video speed comparison was completed in this update.

## Observed IDM behavior

A temporary observer attached to the installed `IDMan.exe` while the user refreshed `BewnzhHlQuk` in regular Edge. It recorded allowlisted request fields, status codes and counters, without logging cookies, authorization values, proof tokens or signed media URLs. It detached after the bounded observation period.

- IDM sent `POST /youtubei/v1/player` over HTTP/1.1 using the Cobalt user agent.
- Its JSON identified client `TVHTML5`, version `5.20260114`, and signature timestamp `20717`.
- A second observation confirmed timezone `UTC`, offset `0`, `HTML5_PREF_WANTS`, and `contentCheckOk: true` / `racyCheckOk: true`. UDM now sends these values too; the fixture verifies the same 365-byte JSON body shape with a supplied timestamp. JSON field order is immaterial.
- No visitor data or proof-of-origin token was present in the captured TV request.
- The TV request received HTTP 200. The observer did not assemble that response body, so its playable status and formats are unknown.
- A following request used a Safari user agent and received HTTP 400 through Schannel; IDM also sent that request through OpenSSL. Its body was not decoded by this observer. This client identity, its successful response path, and any deciphering remain unconfirmed.

These observations demonstrate that IDM itself retrieves player information. They do not support the earlier assumption that a missing network driver explains this difference. The new implementation uses the observed TV client name/version and independently builds the request; it does not claim to reproduce every field or fallback.

Protocol references consulted: the primary [YouTube.js client constants](https://github.com/LuanRT/YouTube.js/blob/main/src/utils/Constants.ts) and [session implementation](https://github.com/LuanRT/YouTube.js/blob/main/src/core/Session.ts). This is an internal service protocol, not a documented public YouTube download API, and can change.

## Implementation

- `native/YouTubePlayer.cpp` performs the fixed HTTPS player request outside the desktop UI and its named-pipe listener. It reads the configured proxy preferences, bounds the operation to eight seconds, and limits the response to 2 MiB. It creates no download record and does not write response data to disk.
- A usable response must permit playback, identify the requested video, supply the exact selected format and nominal quality, and pair it with default AAC audio. Actual pixel height is retained for cropped video.
- Each direct GoogleVideo URL must be HTTPS, match its format and length, remain unexpired, and represent a complete file. Ciphered, DRM, SABR/UMP/segment, playback-range and untransformed `n` URLs are rejected for this path.
- Before handing a pair to the browser, the native host requires a one-byte HTTP 206 response with the exact Content-Range total and media content type for both streams. Redirects are not followed by these probes.
- JSON requests now use their supplied Content-Type and Accept headers. Existing SABR requests retain their protobuf defaults.
- Browser cache entries are tied to tab, frame, document, video, format and quality. Signed URLs remain in memory. Native requests are deduplicated, limited to two concurrent requests, and unsuccessful results are cached briefly. Tab navigation invalidates entries. Final page/format confirmation prevents stale or advertisement handoffs.
- A late native result cannot create a download after the browser has selected its fallback. Available browser direct links continue to use the matching browser user agent.
- This includes the prior 0.20.4 browser repair: final handoff now uses freshly confirmed direct video/audio URLs together, rather than an earlier SABR candidate or stale pair. Regular Edge was verified to have loaded 0.20.4 before this update.

## Validation

The final native build passed **436 checks** before installation. Added fixtures cover request boundaries, video/format identity, quality, cipher/DRM exclusion, audio selection, expiry and host validation, range verification, exact JSON POST body/headers, and cancellation of a stalled HTTP request.

The browser suites cover **157 checks**: 54 browser, 4 capture, 18 streaming capture, 6 UMP, 31 cross-site media, 24 transport handoff, 7 integration policy, 12 Dailymotion recovery, and one repeatable extension-preparation check. The new transport cases include native direct selection, invalid replies, unavailable hosts, cache invalidation, navigation, refreshed browser URLs, and bounded wait with no late duplicate download.

Synthetic checks validate the implementation's contracts; they do not establish download speed against YouTube or IDM. Installation and native-host verification are recorded separately in `deployment.json` and `installed-verification.json`. Edge UI verification succeeded: the existing UDM extension reloaded as version 0.21.0 from `D:\UDM\browser\chromium`. On the current YouTube video, its button appeared after playback began and opened an actual six-quality menu: 1080p, 720p, 480p, 360p, 240p and 144p. No download was started during this panel check. IDM and UDM panels can overlap while both integrations are enabled; complete placement parity was not claimed.

## Remaining work

Investigate the subsequent Safari/OpenSSL request and its response, implement any justified compatible retrieval/deciphering path, and then compare successful IDM and UDM downloads of the same video with full-duration byte and speed measurements. Complete IDM GUI and backend parity remains unverified; the additional gaps listed in the 0.22.0 report still apply.
