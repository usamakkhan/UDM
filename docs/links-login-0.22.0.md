# UDM 0.22.0 — media addresses and server logins

## Changes

UDM's File Info and File Properties dialogs previously displayed the parent YouTube page as the download address even when the browser had captured a different media endpoint. They now display the actual captured media address and keep the web page in a separate field. A new **Links...** dialog provides individually copyable video/audio URLs, an Open page button, and requested/resolved addresses for ordinary HTTP files.

This fixes what the address field represents. It does **not** make UDM and IDM use the same transport or produce identical signed URLs. Captured direct video can use two separate media URLs. UDM's experimental SABR path uses a shared streaming endpoint plus captured request state; its URL alone is not a standalone file download. HLS/DASH jobs are assembled from segments; where the existing capture plan contains no manifest URL, Links explicitly labels its first video/audio segments. Existing captures are not rewritten into a different protocol.

Username/password controls already existed in UDM's Add URL and Properties dialogs, but the initial Download File Info dialog lacked them. This update adds:

- Use login and password, Show password, and Remember for this HTTPS site in File Info.
- Automatic login-and-retry after an HTTP Basic or Digest challenge from the download's own origin.
- Bounded WinHTTP Digest challenge-response support, including authenticated range downloads.
- Windows-account-protected per-download credentials and optional exact-origin saved site logins.
- Consistent username validation in Add URL, File Info, and Properties; colons remain valid inside passwords.
- A distinct authentication error for HTTP 401 instead of the misleading expired-link message.
- Actual server addresses after redirects, protected at rest, with stale addresses cleared when the user changes the download URL.

Captured video keeps the browser sign-in workflow. Entering a video site's account password as HTTP Basic authentication would not reproduce browser cookies or session requests, so those login controls are disabled for captured media. Cross-origin redirects do not receive the source site's credentials. Remembering credentials is limited to HTTPS; manually supplied HTTP/FTP credentials remain per-download.

## IDM inspection and related gaps

Read-only PE inspection of the installed `IDMan.exe` examined English dialog and string resources. No executable code, artwork, activation logic, or installer contents were copied. Binary SHA-256 and matched resource IDs are recorded in `../benchmarks/links-login-20260926/idm-dialog-resources.json`.

| Observed IDM resource | UDM behavior / remaining gap |
| --- | --- |
| File properties: Address, Referer, Login, Password (135) | Present, with corrected media address and separate Links dialog. |
| Add URL / authorization fields (148, 280) | Present; validation now shared across dialogs. |
| Login prompts and Save password (185, 186, 279) | HTTP Basic/Digest login-and-retry and exact HTTPS site remembrance added. |
| Different logins for directories on one site (146) | Still a gap: UDM saved logins match an entire exact origin, not path-specific rules. |
| Separate HTTP/HTTPS/FTP proxy and SOCKS controls (140, 369) | UDM has Windows/PAC or a configured proxy; this complete per-protocol matrix is not implemented. |
| Browser-assisted Grabber login (257) | UDM's static site explorer does not implement the same authenticated Grabber wizard. |
| URL exceptions and repeat-cancel behavior (303, 304) | UDM already has address-pattern exceptions; IDM's complete automatic repeat-cancel workflow has not been reproduced. |

Resource labels establish available controls, not the complete behavior of IDM's closed-source backend. Documentation corroborates [file properties](https://www.internetdownloadmanager.com/support/properties.html), [password requests and saved logins](https://www.internetdownloadmanager.com/register/new_faq/password-request.html), and [Options](https://support.internetdownloadmanager.com/support/options.html). The Digest implementation follows [Microsoft's WinHTTP authentication API](https://learn.microsoft.com/en-us/windows/win32/winhttp/authentication-in-winhttp).

## Validation

The native fixture uses loopback servers and synthetic credentials. It verifies exact output bytes, Basic and cryptographically checked Digest responses, incorrect passwords, bounded retries, same-origin redirects, cross-origin credential stripping, protected storage, exact site matching, storage rollback, and media-address semantics. It does not use or print the user's passwords or signed playback URLs.

All three installed binaries report 0.22.0 and their SHA-256 hashes match the tested build. The running application's host diagnostic confirms 0.22.0 and the intended `D:\UDM\user-data` directory. All **19 download records and settings are unchanged** after installation and startup. No existing transfer was active during deployment.

Build output, installed-binary hashes, history verification, and resource-inspection evidence are in `../benchmarks/links-login-20260926/`. Previous binaries and the original state are backed up in the C: staging directory, `links-login-0.22.0`.

Live UI verification remains blocked: after the user unlocked the PC and requested Edge, the browser connector reported **Browser is not available: edge**, and the installed Edge launch through desktop control returned **GetCursorPos failed: Access is denied (0x80070005)**. No new real Edge video completion or simultaneous IDM comparison is claimed for this update. Browser extension code remains 0.20.3; these changes are in the native application and host.
