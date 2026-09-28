# UDM 0.53.0 / browser 0.37.0

This release adds custom proxy configuration scripts, TLS protocol control and a modification-date override for ordinary HTTP/FTP downloads. The controls are connected to native transfer behavior. It does not establish full IDM parity or a percentage of parity.

## Changes

- Options > Proxy / Socks now accepts an HTTP/HTTPS automatic configuration script URL. Advanced supports separate scripts for HTTP, HTTPS and FTP. Script retrieval/evaluation uses Windows' asynchronous resolver and cancels promptly. Domain credentials are not sent automatically to retrieve scripts.
- Each HTTP redirect gets its own route evaluation. PAC alternatives are tried in order for connection failures; POST bodies are not automatically replayed on another route. HTTP authentication rejection, origin errors and TLS validation failure do not authorize proxy fallback. Direct routing comes from the resolver or explicit bypass, including Windows' implicit localhost bypass. Failed script retrieval/evaluation does not create an extra direct route.
- PAC supports HTTP proxy and SOCKS4/4a results. Native passive FTP carries control and data through the selected HTTP CONNECT/SOCKS route. It preserves the underlying error when every route fails. FTP through a TLS-encrypted proxy is rejected explicitly; it is never downgraded to a plaintext proxy.
- Options > Connection exposes TLS 1.3. Enabled offers TLS 1.2/1.3 where Windows supports them; disabled restricts this engine to TLS 1.2. Older Windows that reject the TLS 1.3 flag use TLS 1.2. OS policy and certificate validation remain effective. Live tests inspect actual ClientHello versions, including normal TLS 1.2 fallback.
- Options > Downloads exposes Ignore file modification time when resuming downloads. It is off by default. When enabled, ordinary HTTP/FTP transfers tolerate inconsistent modification dates, including resuming a saved partial file. Valid byte ranges, file length, strong ETags and source identity restrictions remain. A refreshed link still needs identity validation. Media cache and stream-specific identity policies remain separate.
- Browser-captured fixed proxy routes clear a stale global PAC address. Existing per-protocol settings and protected passwords retain their behavior.

The PAC radio/address field, TLS checkbox and modification-date checkbox use their measured reference dialog positions. Existing nine-tab Options and download workflows remain available. The date override cannot prove that a same-sized file without a strong ETag is unchanged; enable it only for a source known to report unreliable timestamps.

## Verification

- 1,024 native checks passed, including 16 added PAC/settings checks.
- 24 real transport scenarios passed: PAC routing/redirects/failover/cancellation, no POST replay, SOCKS, FTP, partial-file identity, date/ETag/size changes and TLS ClientHello capture. Finished files were checked byte-for-byte and with SHA-256.
- 127 existing live regression scenarios passed across FTP resume, FTP SOCKS/CONNECT, protocol routing and metadata preview (including certificate rejection).
- 8 native-host protocol checks and 6 isolated running-app/host checks passed against 0.53.0. The app checks use a Node parent; they are not browser-profile acceptance tests.
- 112 selected control rectangles match the installed reference resources, including 5 newly matched transport controls. This is a static geometry check, not a rendered screenshot or click test.

Initial testing identified invalid assumptions in test fixtures: WinHTTP's implicit loopback bypass, legitimate TLS 1.2 retry, and querying a set-only TLS option. The fixtures were corrected and TLS behavior is tested on the wire. An FTP regression caught the loss of the underlying proxy error; the implementation now preserves it. Final receipts are in [evidence](evidence-0.53.0/summary.json).

## Remaining limits

Desktop automation still fails before initialization with a kernel-assets path error. Native rendering, button/keyboard behavior and mixed-monitor appearance need visual acceptance. Browser JavaScript stays at 0.37.0; current-profile Chrome/Edge/Firefox acceptance and current public YouTube/video-platform compatibility are not newly established here.

This is not a speed win over IDM: no new controlled public ISO benchmark was run. Driver equivalence, broader video workflows, quota-warning UI, direct dial-up credentials, completed-duplicate behavior, publisher signing and clean-machine installation remain outside the verified result. The signed network runtime and browser sources are unchanged.

Reference: [IDM Options](https://www.internetdownloadmanager.com/support/options.html), [Microsoft proxy resolver](https://learn.microsoft.com/en-us/windows/win32/api/winhttp/nf-winhttp-winhttpgetproxyforurlex), [WinHTTP protocol settings](https://learn.microsoft.com/en-us/windows/win32/winhttp/option-flags).

## Current-PC deployment

[UDM 0.53.0 installer](../installer-out/UDM-0.53.0-Browser-0.37.0-Setup-x64.exe) was built. 50 files were deployed with SHA-256 verification and backups. Installer SHA-256: `005AC96E0A2F4BD853E74D2BAB8B5434301C40AFC921F297965B0568AD88458F`.

The running native bridge reports 0.53.0 and the correct D:/UDM/user-data catalog. All existing download records, all queues and every existing preference remain unchanged. First startup saved the new defaults: UseTls13=true, IgnoreLastModified=false and ProxyAutoConfigUrl empty. The state file therefore has a new hash; the original state is retained in the deployment backup. Browser files and the signed network runtime remain unchanged. No extension source update or reload is required specifically for 0.53.0; prior current-profile activation is still unverified.
