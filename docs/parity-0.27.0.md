# UDM 0.27.0 / browser integration 0.24.0

This release closes two specific gaps in the IDM comparison: captured form downloads and per-download completion actions. Full IDM parity is **not complete**. Neither an identical proprietary engine nor support for every video platform has been established.

## Delivered

- Browser form downloads preserve the original POST method, body, content type and scoped request headers. Bodies remain in bounded, short-lived extension memory and are encrypted with Windows DPAPI in UDM history. They are never written to extension storage.
- Raw bodies and UTF-8 URL-encoded forms up to 64 KiB are supported, including repeated form fields. Capture requires a matched, successful attachment response and an actual browser download event. File-backed uploads, unavailable bodies, ambiguous submissions, reconstructed multipart forms and larger bodies stay in the browser.
- Direct POST responses stream once. UDM does not probe them with GET, split them into repeated POST workers, prefetch them in File Info, or automatically retry them after a failure. Explicit Start can resubmit a failed form. The File Info dialog describes this behavior.
- A 301/302/303 redirect converts the download to GET, where ordinary validated parallel ranges and resume can work. Same-origin 307/308 redirects preserve POST; cross-origin POST redirects are rejected. Sensitive headers are stripped when a GET redirect changes origin. Saved GET continuation addresses are encrypted.
- Duplicate matching includes the POST body and method. Redownload retains the encrypted request. Catalog exports omit form bodies by default and require browser recapture when the body is absent; optional encrypted exports preserve them for the same Windows account.
- Progress > Options on completion > Completion action offers opening the saved file, exiting UDM, disconnecting dial-up/VPN, sleep, hibernate, shutdown and restart. Actions are off by default, run only after successful completion, and have a cancellable 15–3600-second countdown. Waiting for other active/queued downloads is the default. Armed actions are consumed once and excluded from catalogs.
- CORS OPTIONS requests no longer obscure the GET/POST request used for capture. This regression was found by the actual authenticated Edge transfer test and fixed before deployment.

Browser capture follows the request-body representations documented by [Chrome webRequest](https://developer.chrome.com/docs/extensions/reference/api/webRequest). Redirect handling follows the method distinctions in [HTTP Semantics, RFC 9110](https://www.rfc-editor.org/rfc/rfc9110.html). These are UDM implementations, not recovered IDM source code.

## Validation

629 automated checks passed:

| Suite | Passed |
|---|---:|
| Native core, transfer, recovery, media, POST and completion actions | 489 |
| Browser component suite | 54 |
| Dailymotion recovery | 12 |
| Integration parity | 20 |
| Integration policy | 7 |
| POST request context | 15 |
| Chromium/Firefox package identity and shared sources | 1 |
| Actual Edge extension + native UDM regression | 13 |
| Actual Edge form-download handoff and browser fallback | 5 |
| Native-host framing/protocol | 7 |
| Native launch/handoff and scheduling | 6 |

Actual Edge transfers verified byte-identical POST and MP4 output, HLS assembly, encrypted authenticated headers, host acknowledgement before browser cancellation, shortcuts and iframe navigation. The oversized-form fallback completed in Edge without creating a wrong GET job.

The completion-action dialog was also operated visually in a separate UDM instance. A 1 MiB local download completed, its Exit UDM countdown appeared, Cancel kept the app running, and saved state showed `CompletionActionArmed: false`. Power or VPN actions were not executed on this PC.

The first ZIP fixtures were intercepted by installed IDM before Edge's download event. Those test dialogs were inspected and cancelled; the final fixture used a dedicated file extension to isolate UDM. Initial test runs also exposed missing media helpers in the staging folder and the CORS matching bug; the reported results are from passing reruns. The final binaries were rebuilt after the test process that temporarily locked the executable exited.

## Remaining parity gaps

| Area | Status |
|---|---|
| Arbitrary POST/download bodies | Partial: supported forms are now functional; file uploads, unavailable/multipart reconstruction and bodies over 64 KiB remain in the browser. |
| Video platforms and ciphered streams | Existing direct media/HLS/DASH/YouTube capture is retained. Comprehensive site coverage and ciphered-stream handling remain incomplete. No claim of DRM support. |
| Offline website mirroring | Static bounded link collection exists; complete offline page/asset rewriting is not implemented. |
| Network compatibility | HTTP(S), FTP, Windows proxy/PAC and named proxies exist. SOCKS and dial-up connection setup remain absent. Disconnect-after-completion is available. |
| Visual parity | Earlier compact File Info/progress/properties/menu work is retained. Some Windows menus, combo boxes and scrollbars still use standard styling. Languages, skins and broader accessibility/mixed-DPI testing remain. |
| Signed public distribution | No public code-signing identity or production driver-signing release has been supplied. Existing WFP diagnostics are not a replacement download engine. |
| Performance equivalence | The previous two-minute comparison remains one near-equal pair, with different network routes. This release makes no new claim of general speed superiority. |

UDM's name, icons and original code are retained. No IDM binaries, activation logic, icons or executable code were copied into UDM. Evidence, build logs, pre-update backups and the hash-checked deployment manifest are in the adjacent `parity-completion-20260926` work folder on C:.

Final UI regression: when a download completes while its options dialog is open, progress-window auto-close waits until that dialog is dismissed. A throttled 1 MiB fixture verified completion behind the open dialog and a responsive return to the main window. The completion-action button also disables when the file finishes.
