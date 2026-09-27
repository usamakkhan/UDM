# UDM 0.28.0 — offline websites and SOCKS5

Implemented in the original C++/MFC UDM code and tested on this PC. This release closes two concrete functional gaps; it does not establish complete IDM parity.

## Offline websites

Site Grabber has an **Offline website (ZIP)** template. The archive is a normal download job with queue scheduling, pause/resume, history, properties, catalog export/import and redownload. Extract it and open `index.html`.

The capture follows the selected page depth and page count, saves HTML/CSS dependencies, rewrites relative/base URLs, fragments, srcset, CSS imports and image/font/media URLs, and uses response MIME types for assets without file extensions. External CDN assets are an explicit option. Verified cached resources are reused after pause; generated cache files are removed after successful archive publication.

Limits: depth 0–5, 1–100 pages, 2,000 resources, 2 MiB per HTML/CSS document, 32 MiB per other resource, and a configurable total budget of 1–2,048 MiB (default 256). The ZIP includes `UDM-offline-report.json` with omitted resources and errors. Scripts and forms are disabled in saved pages. This is static website capture, not a clone of an interactive web application. Uncaptured navigation links retain their original addresses; omitted embedded assets cannot fetch remotely.

## SOCKS5

Options > Proxy now offers **Use a SOCKS5 proxy**, using the existing encrypted proxy credentials. HTTP/HTTPS downloads keep the WinHTTP range engine and Windows certificate validation. A private authenticated loopback bridge implements SOCKS5 CONNECT, remote destination DNS, IPv4/IPv6 addresses and username/password authentication. HTTP POST downloads also use the selected proxy.

On this Windows installation, loopback destination URLs bypassed WinHTTP's proxy routing even with explicit proxy settings. UDM now blocks this implicit direct path in SOCKS5 mode. A user can deliberately allow a direct local download by entering that host in the Bypass list; ordinary proxied hostnames do not fall back to direct download after rejection. FTP through SOCKS5 is not implemented and fails before connecting directly. SOCKS5 is not automatically enabled for existing users.

Protocol references: [SOCKS5 CONNECT and address formats](https://www.rfc-editor.org/rfc/rfc1928), [SOCKS5 username/password authentication](https://www.rfc-editor.org/rfc/rfc1929), and [Microsoft WinHTTP proxy configuration](https://learn.microsoft.com/en-us/windows/win32/api/winhttp/ns-winhttp-winhttp_proxy_info).

## Verification

- Native suite: **519 passed, 0 failed**, including 30 new offline/proxy checks.
- Live SOCKS5/offline suite: **18 passed, 0 failed**.
- Native launch and modal/queue scheduling regression: **6 passed**.
- Native-host framing/protocol: **7 passed**.
- Installed Edge extension/native regression: recorded after deployment in the installation verification.

Live checks verified SHA-256-identical parallel transfers through a fixture SOCKS5 server, successful/rejected authentication, no direct fallback after rejection, exact POST body, IPv6 destination bytes, rejection of an untrusted HTTPS certificate through the tunnel, prompt cancellation, explicit loopback policy, offline asset rewriting, external asset selection, redirect boundaries, bounded transfer budget, cached resume, actual desktop queue completion, and offline Edge rendering/navigation with zero HTTP(S) requests.

The initial stalled proxy test stopped in 485 ms with a 400 ms cancellation deadline. This is cancellation responsiveness, not an Internet speed benchmark. No new claim of speed superiority over IDM is made.

The native computer-control helper failed before desktop inspection with “failed to write kernel assets: The system cannot find the path specified.” A reset returned the same error. The new native controls compiled and the queue ran; their final visual appearance could not be inspected. Isolated Edge successfully rendered the extracted offline archive.

Initial fixture failures and the later passing runs are retained. Those failures exposed WinHTTP loopback routing and unrewritten missing assets. The final code blocks implicit local bypass and substitutes inert URLs for omitted embedded resources.

## Remaining differences

| Area | Remaining work or limit |
|---|---|
| Video capture | Broader platform coverage and unsupported ciphered streams remain. No DRM support is claimed. |
| Browser form capture | Existing bounded POST support remains; arbitrary file uploads, reconstructed multipart bodies and bodies over 64 KiB are not replayed. |
| Website capture | Static snapshots are now functional; script-driven sites, authenticated browser-session mirroring and complete recursive copies beyond the configured limits are not covered. |
| Network compatibility | SOCKS5 HTTP/HTTPS is now functional. SOCKS4, SOCKS FTP and dial-up connection setup/automatic connection workflows remain absent. |
| GUI | Languages, skins, some Windows-standard control styling and broader accessibility/mixed-DPI testing remain. New native controls await visual inspection. |
| Distribution | Public code signing and production driver signing require a distribution identity. The existing WFP monitor remains diagnostic, not an acceleration engine. |
| Performance | Prior two-minute ISO measurements remain a single near-equal pair. General speed equivalence has not been established. |

No IDM executable code, branding assets, activation logic or license checks were copied or modified. Deployment uses SHA-256 checks, C: backups, and an exact comparison of all existing download records.

