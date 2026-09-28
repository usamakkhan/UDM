# UDM 0.67.0 / browser 0.45.0 — extended Edge proxy handoff

Edge captures now preserve encrypted HTTPS proxies and SOCKS4 local DNS. Firefox's request-bound metadata can also retain HTTPS and explicitly reported local or remote SOCKS DNS settings; fresh browser acceptance here focuses on Edge. Native transfers maintain that route for metadata, parallel ranges, POST and resume, including destinations outside loopback.

The desktop keeps proxy credentials separated by both endpoint and encryption protocol. The browser checks native compatibility before accepting extended routes. Older 0.44.0 extensions remain compatible with this native update for their existing route types.

**2,586 passing checks:** 1,680 native regressions, 787 browser unit checks, eight native-protocol checks, 43 isolated Edge handoff checks, 20 extended native proxy checks, 24 existing proxy checks and 24 PAC/options checks. The live tests verify complete output hashes, observed proxy traffic, local versus remote DNS, TLS certificate rejection, credentials, POST, pause/resume and ordered failover. [Evidence](evidence-0.67.0/summary.json), [test limits and corrections](evidence-0.67.0/DEVELOPMENT.md).

The temporary localhost TLS test certificate was removed and its absence verified. No new network driver or boot/signing change is part of this release. These results establish the tested transfer paths, not IDM speed parity or universal video-site support.

Browser PAC/auto-detect routes not exposed by Chromium still remain browser-owned. QUIC proxies, ambient proxy authentication, active proxied FTP and FTP gateways remain open. Captured routes remain tied to their exact URL; changed redirects require browser recapture. [The full workflow inventory](idm-parity-0.67.0.md) remains incomplete.

## Current-PC deployment

Native 0.67.0 is installed and running. The bridge reports extended proxy support, including HTTPS and explicit SOCKS DNS policy. All 23 records, queues and preferences are unchanged, and installed file hashes were verified. The signed network runtime is unchanged. Reload UDM Browser Integration in Edge to activate browser 0.45.0 in the personal session.

[Installer](../installer-out/UDM-0.67.0-Browser-0.45.0-Setup-x64.exe), SHA-256 `b882ac82ce12e609efe4e4c417dc13e2717e2e82943a48a2ab4d619ac38a1c63`. [Deployment receipt](evidence-0.67.0/deployment.json).
