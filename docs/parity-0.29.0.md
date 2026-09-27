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

## Verification

Final build completed for UDM, NativeHost, Monitor and NativeTests. Fresh checks before installation:

- **576 native checks passed**, including 57 new connection, scope, cancellation, clipboard, FTP-parser and actual concurrent rate-wait checks.
- **10 SOCKS4/4a checks passed** against a real loopback proxy fixture: parallel ranges with exact SHA-256, IPv4/user ID, remote DNS, rejection without direct fallback, malformed replies, POST, redirects, certificate validation, cancellation, and User-Agent precedence.
- **2 FTP checks passed**, comparing saved bytes and hashes and verifying the actual PASV versus PORT commands.
- **18 SOCKS5/offline checks passed**, including exact parallel downloads, TLS rejection, cached resume, bounded capture, actual desktop queue completion and offline Edge rendering with zero network requests.
- **6 native launch checks passed**, including another queued download completing while File Info remains open.
- **7 native messaging protocol checks passed**.

Total before installation: **619 checks passed**. The separate targeted run overlaps the full native suite and is not added again. Initial failures and subsequent passing logs are retained. They exposed the scope-fragment validation defect and the FTP parser defect. An initial sandboxed fixture run also hit Windows access error 5; the authorized isolated run passed outside that restriction.

The dial retry/concurrency/cancellation cases use an injected test backend. The real Windows test uses a unique nonexistent entry and verifies that no HTTP request is made after connection failure. This does not establish successful dialing against a modem, VPN server or every EAP provider.

The native Computer Use helper still fails before initialization with `failed to write kernel assets: The system cannot find the path specified. (os error 3)`. New native controls compiled; live native appearance, focus order and modal clipboard behavior are not claimed as visually verified. Edge fixture testing is independent of that helper.

## Installation and remaining acceptance

Installed and running as **UDM 0.29.0**. All **43 installed files** match the tested deployment hashes. All **23 pre-existing download records** match the pre-installation snapshot exactly. The installed native host reports the correct version, history directory and download count. Backups are retained in `parity-connections-20260926/backup-before-0.29.0`.

The **installed Edge extension/native integration passed 13 additional checks**, including isolated-history routing, byte-identical MP4, recorded HLS video/audio assembly, authenticated range downloads, Download Later, capture modifiers and stale iframe playlist exclusion. Final total: **632 checks passed**; earlier overlapping targeted runs are not added again.

Full parity remains gated by the specific open rows in the [current comparison](idm-parity-0.29.0.md): notably live/subtitle/alternate-track media workflows, wider real-site capture coverage, resumable/parallel FTP and SOCKS FTP, browser-specific proxy inheritance, rendered/authenticated site mirroring, localization/skins, production signed distribution, comprehensive native visual/accessibility/DPI tests and repeatable Internet performance trials. Passing these fixture tests does not establish IDM speed equivalence or support for every video platform.

Builds, initial failures, final results and deployment backups are retained under the local `parity-connections-20260926` artifact directory. Current Chromium/Firefox extension source remains 0.24.0; this release updates the desktop and native host.
