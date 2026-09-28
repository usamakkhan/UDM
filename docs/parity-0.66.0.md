# UDM 0.66.0 / browser 0.44.0 — Edge proxy handoff

UDM now reads Edge's effective fixed-proxy or direct configuration and binds the selected route to the captured download. Changing proxy settings invalidates older observations. Explicit link commands use the source tab's regular or InPrivate configuration; automatic InPrivate capture remains excluded.

WinHTTP silently bypassed loopback proxy destinations. A scoped native libcurl adapter now honors those explicit HTTP, SOCKS4/4a and SOCKS5 routes with Schannel certificate validation. Ordinary WinHTTP downloads retain their transport. The adapter supports bounded streaming, pause/resume, HTTP authentication, proxy credentials, metadata, POST and the existing redirect policy. The new extension checks native compatibility before handing off non-direct routes.

**2,509 fresh checks passed:** 1,669 native regressions, 779 browser unit checks, 8 native-protocol checks, 29 isolated Edge handoff checks, and 24 native proxy transport checks. [Machine-readable evidence](evidence-0.66.0/summary.json), [development record](evidence-0.66.0/DEVELOPMENT.md).

The Edge acceptance test verifies exact output hashes and observed routes through two HTTP proxies, direct mode, implicit and explicit bypass, SOCKS5 remote DNS, and a session-dependent explicit link. Native tests additionally cover login, POST, cross-origin credential stripping, cancellation, resumed ranges, malformed or truncated responses, and HTTPS certificate rejection. The TLS fixture verifies rejection, not a successful trusted TLS download through this new adapter. These are functional tests, not an IDM throughput benchmark.

Browser PAC and auto-detect routes that Chromium does not expose remain in Edge. Encrypted browser proxies, Chromium SOCKS4 local DNS, ambient proxy-authentication state, and changed or ambiguous capture contexts are not guessed. A captured route remains bound to its exact URL; redirects beyond it require a fresh browser capture. System mode retains native Windows proxy behavior. These boundaries mean F079 is still partial.

The driver and Windows boot/signing settings are unchanged. Native 0.66.0 and the 0.44.0 extension need to be used together for the new route support. [Earlier workflow inventory](idm-parity-0.65.0.md) remains the broader baseline; full IDM GUI, driver, site coverage, and performance parity are not established.

## Current-PC deployment

Native 0.66.0 is installed and running. The bridge reports explicit proxy support. All 23 records, queues and preferences are unchanged, and installed file hashes were verified. The signed network runtime is unchanged. Reload UDM Browser Integration in Edge to activate browser 0.44.0 in the personal session.

[Installer](../installer-out/UDM-0.66.0-Browser-0.44.0-Setup-x64.exe), SHA-256 `2ec20c0e5d28c89b5b60e8d84d3dc7ab3f6f780b2c2aabcb264292bcc0c6d3db`. [Deployment receipt](evidence-0.66.0/deployment.json).
