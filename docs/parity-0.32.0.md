# UDM 0.32.0: FTP through SOCKS proxies

UDM can now download FTP files through SOCKS4/4a and SOCKS5. Previously selecting a SOCKS proxy caused FTP downloads to fail before connection. Both the FTP control channel and passive data channels now use the selected proxy, including parallel ranges, resume, credentials and File Info metadata preview.

## Using the feature

Select SOCKS4/4a or SOCKS5 in **Options → Proxy**, then enter its host, port and optional login. Leave **Use passive FTP** enabled under **Options → Downloads → Advanced transfer settings** (the default). FTP server credentials remain in the download's login fields; they are separate from proxy credentials. The Proxy tab's help text includes passive FTP support.

Explicit bypass rules continue to opt a matching host into direct connections. Proxy failures never silently fall back to direct access. Active FTP through SOCKS reports a message pointing to the passive-mode setting before opening a connection. HTTP proxy/PAC FTP modes remain unsupported and fail explicitly.

## Implementation and boundaries

- One shared SOCKS CONNECT implementation serves native FTP sockets and the existing authenticated WinHTTP loopback bridge. FTP does not traverse an extra local HTTP bridge. HTTP(S) retains WinHTTP certificate validation and its loopback-bypass guard.
- SOCKS4a and SOCKS5 receive the destination hostname for proxy-side lookup. UDM resolves the proxy host itself through cancellable Windows asynchronous DNS. DNS moved out of synchronous HTTP-session construction. Cancellation during actual DNS-server delay was not separately fault-injected; stalled handshakes and metadata requests were tested.
- Passive EPSV/PASV connections use the original FTP hostname and validated high port. PASV-supplied foreign IPs are ignored. Direct FTP retains its control-peer address pinning. With proxy-side DNS, the proxy controls destination resolution; UDM cannot prove that repeated hostname lookups reach the same IP.
- The established FTP engine still validates SIZE/MDTM before reusing partials, checks segment lengths, preserves unverifiable data and verifies SHA-256 before publication. These FTP metadata validators are not cryptographic proof of remote identity; a configured expected SHA-256 supplies that final content check.
- Transient SOCKS network failures in transfer workers use the existing bounded retry budget. Authentication, protocol and policy rejections are permanent. The initial FTP metadata probe still has no retry loop.
- SOCKS5 supports no-auth or username/password; SOCKS4/4a supports an optional user ID, without a password. This is not support for every SOCKS5 authentication method. FTP remains plain FTP; FTPS/SFTP and literal-IPv6 FTP URLs are outside this release.

Protocol references: [SOCKS5 CONNECT](https://www.rfc-editor.org/rfc/rfc1928), [extended FTP passive mode](https://www.rfc-editor.org/rfc/rfc2428). The implementation is original UDM code.

## Validation

| Suite | Passed | Coverage |
|---|---:|---|
| Complete native regression | 601 | Core, queue, transfer, UI models and preview eligibility |
| New FTP/SOCKS fault suite | 31 | Eight-way transfers, remote DNS, independent logins, PASV host handling, restart/resume, changed-file preservation, transient retry, control/data rejection, malformed/fragmented replies and cancellation |
| Direct FTP fault suite | 32 | Existing active/passive, resume, corruption prevention and metadata behavior |
| Independent pyftpdlib server | 7 | Direct active/passive; parallel and process-restart resume through SOCKS4a and SOCKS5 |
| HTTP metadata preview | 34 | HEAD/range fallback, authentication, redirects, TLS rejection, excluded jobs and unchanged state |
| HTTP SOCKS4/4a | 12 | Exact ranges, POST, redirects, preview, TLS validation and cancellation |
| HTTP SOCKS5/offline | 19 | Authenticated proxy transfer, preview, cancellation and offline archive rendering |
| Native launch | 7 | Real File Info metadata request, queued transfers and command-line handoff |
| Native messaging | 7 | Framing, persistent messages and malformed-message rejection |

**750 checks passed before installation.** Overlapping development runs are not added. The final build includes an error-message correction and Proxy-tab help text after the transport suites; the native, launch and messaging suites use the final release build. Tests use isolated profiles, local servers and test-only credentials. They establish transfer correctness and bounded cancellation in tested scenarios, not an Internet speed advantage over IDM.

## Installation

Installed and running as **UDM 0.32.0**. All **27 installed files** match their deployment hashes. All **23 download records** match the pre-installation snapshot exactly, including after browser testing. NativeHost reports 0.32.0 and the correct history directory/count. Replaced files and history are backed up under `parity-ftp-proxy-20260927/backup-before-0.32.0`.

The installed Edge integration passed **13 additional checks**: persistent host handoff, authenticated range transfer, byte-identical MP4, recorded HLS with audio, Download Later, modifier gestures and stale iframe exclusion. The final total is **763 passed checks**, excluding overlapping reruns. Testing used a separate Edge profile and isolated UDM history; normal browser settings were not changed.

## Remaining work

The [103-workflow audit](idm-parity-0.32.0.md) still has **87 implemented code paths and 16 open rows**. Proxy coverage improved within a partial row; this is not full IDM parity. Open work includes remaining proxy modes, wider video-site coverage, live/subtitle/alternate-track workflows, localization, signed distribution and native accessibility/visual/performance acceptance. Native dialog visual QA remains unverified. Browser extension source remains 0.24.0; this release changes its desktop backend.

Artifacts and backups are retained in the writable visualization workspace under `parity-ftp-proxy-20260927`.
