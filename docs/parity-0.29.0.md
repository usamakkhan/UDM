# UDM 0.29.0 — connection and capture workflows

This release implements seven missing or different workflows in the original C++/MFC app and fixes an FTP defect found by a new live test. The [updated 103-workflow comparison](idm-parity-0.29.0.md) records 85 implementation paths, 13 partial workflows, three unverified requirements and two missing workflows. These counts are an inventory, not a parity percentage or proof that every IDM feature has been enumerated.

## Changes

| Workflow | Behavior and where to find it |
|---|---|
| Folder-specific site logins | Options > Site logins accepts HTTPS sites and folders. The longest matching folder wins. Saved credentials are selected at request time, so editing a login reaches existing downloads. Same-site redirects re-evaluate the folder; cross-origin redirects strip sensitive headers. Explicit per-file/browser authorization takes precedence. Removing a per-file login suppresses automatic saved credentials for that file. Passwords remain Windows-encrypted at rest. |
| SOCKS4 / SOCKS4a | Options > Proxy adds SOCKS4/4a beside SOCKS5. HTTP(S) requests retain the parallel range engine and certificate checks. IPv4 uses SOCKS4; names use SOCKS4a remote DNS. Supports an optional user ID. Unsupported IPv6, configured passwords, rejected handshakes and implicit loopback bypass fail instead of connecting directly. FTP through SOCKS remains unsupported. |
| Windows dial-up / VPN | New Options tab selects a Windows phone-book entry and opens native New, Properties and Connect dialogs. Opt-in automatic connection gates manual/queued downloads, synchronization, previews and transfer requests. Workers share one dialing operation, reuse a connected entry, retry with configurable bounds, and cancel through Pause. Interactive authentication is delegated to Windows; UDM uses saved Windows credentials/EAP identity for unattended attempts. Connections UDM establishes automatically are released on exit; reused connections are not owned by UDM. No real user connection was dialed during validation. |
| Manual User-Agent | Options > Downloads > Advanced transfer settings. A captured browser User-Agent takes precedence. Empty uses UDM's default. Header-control characters are rejected. |
| Passive / active FTP | The same Advanced dialog offers passive FTP, enabled by default. Both data modes now pass live local transfers. The new test discovered that WinHttpCrackUrl accepted an FTP scheme, after which UDM rejected it; FTP now takes its own parser, with validated ports. FTP remains sequential and does not resume saved partial bytes. |
| Global speed-limit semantics | Options > Connection selects Total across downloads or Each download. Per-download mode uses the stricter global/per-file cap and shares that cap across the file's workers. Existing aggregate behavior is retained by default. |
| Clipboard capture | Options > General > Clipboard options selects a suggestion or automatic Download File Info. Eligible file extensions and excluded hosts are checked. Extensionless addresses can be explicitly allowed. Monitoring stays opt-in; native modal interaction still needs visual acceptance. |

The network behavior is informed by [IDM's documented options](https://support.internetdownloadmanager.com/support/options.html), Microsoft's [RAS dialing](https://learn.microsoft.com/en-us/windows/win32/api/ras/nf-ras-rasdialw) and [connection cleanup](https://learn.microsoft.com/en-us/windows/win32/api/ras/nf-ras-rashangupw) APIs, and the original [SOCKS4](https://www.openssh.org/txt/socks4.protocol) / [SOCKS4a](https://www.openssh.org/txt/socks4a.protocol) specifications. UDM's implementation is independent; IDM executable code and artwork were not copied.

## Release validation

Final builds completed for UDM, NativeHost and Monitor. Validation covered connection scope, cancellation, clipboard handling, FTP parsing and transfers, SOCKS4/4a and SOCKS5 behavior, protocol messaging, launches, and offline Edge rendering. The work exposed and resolved scope-fragment validation and FTP-parser defects.

Dial retry, concurrency and cancellation use an injected backend. A real Windows attempt with a unique nonexistent entry confirmed that no HTTP request was made after connection failure. This does not establish successful dialing against a modem, VPN server or every EAP provider.

New controls compiled; live appearance, focus order and modal clipboard behavior are not claimed as visually verified.

## Installation and remaining acceptance

Installed and running as **UDM 0.29.0**. All **43 installed files** match the tested deployment hashes. All **23 pre-existing download records** match the pre-installation snapshot exactly. The installed native host reports the correct version, history directory and download count. Backups are retained in `parity-connections-20260926/backup-before-0.29.0`.

Installed Edge integration covered isolated-history routing, byte-identical MP4, recorded HLS video/audio assembly, authenticated range downloads, Download Later, capture modifiers and stale iframe playlist exclusion.

Full parity remains gated by the specific open rows in the [current comparison](idm-parity-0.29.0.md): notably live/subtitle/alternate-track media workflows, wider real-site capture coverage, resumable/parallel FTP and SOCKS FTP, browser-specific proxy inheritance, rendered/authenticated site mirroring, localization/skins, production signed distribution, comprehensive native visual/accessibility/DPI tests and repeatable Internet performance trials. Passing these fixture tests does not establish IDM speed equivalence or support for every video platform.

Builds, initial failures, final results and deployment backups are retained under the local `parity-connections-20260926` artifact directory. Current Chromium/Firefox extension source remains 0.24.0; this release updates the desktop and native host.
