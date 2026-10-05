# UDM 0.81.0 candidate — shared HTTP connections

Source and installer delivered. The installed personal app has not been restarted or replaced. Browser files remain 0.53.1. Full IDM parity remains unestablished.

## Change

Each HttpSession now owns a lazy curl multi pool. Explicit-proxy GET/HEAD requests reuse eligible HTTP/1.1 connections and multiplex HTTP/2 streams within that session. Different sessions and resolved routes retain separate pools. Workers serialize attached-handle operations and callback access, release the lock between bounded polls, and dispatch completion to the matching request. Cancellation removes the affected request; other streams remain attached. Request destruction uses the same lock, including constructor failures.

POST keeps a dedicated pool and its own copy of submitted bytes. Headers, credentials, cookies and response buffers remain per request. TLS peer and hostname checks remain enabled. Global curl initialization precedes the version query. Both backends retain 0.80 HTTP/2 negotiation and HTTP/1.1 fallback.

This follows the documented [curl multi interface](https://curl.se/libcurl/c/libcurl-multi.html), [serialized handle ownership](https://curl.se/libcurl/c/threadsafe.html) and [multiplex connection waiting](https://curl.se/libcurl/c/CURLOPT_PIPEWAIT.html). It is independently implemented UDM code; IDM's private implementation is not established by these tests.

## Evidence

- 2,465 native checks passed on the final linked candidate.
- 45 transport checks across 27 requests passed with exact payload/range hashes, both HTTP/2 paths, HTTP/1.1 fallback, POST, redirects, cancellation and invalid-certificate rejection.
- 18 pooling checks across 48 actual requests passed. Seven HTTP/2 requests, including overlapping delayed streams, used exactly one observed TCP connection. Two four-second bodies overlapped; their concurrent group finished in 4375 ms. Its canceled stream returned in 2172 ms while peers completed with correct bytes.
- Sequential HTTP/1.1 GET/range/HEAD reused one connection. Independent sessions and POST used separate connections. Truncation, server close, cancellation, a delayed reader, and another origin's TLS failure left peers and follow-up requests intact. Sixteen simultaneous stalled requests canceled within the two-second bound. Synthetic authentication and cookies stayed request-local.
- The installed 25-record catalog remains byte-identical. No certificate trust, personal browser, driver, hosts-file or installed runtime change was made. New artifacts are on D:.

[Reproduction instructions](../tests/transport-pool/README.md) and the fixture source are included in the local project. No Git commit/push was performed. Initial and final runs are retained; repeated cases are not extra coverage. A version-preparation step encountered a text-decoding error, corrected using explicit UTF-8; final binaries use the completed sources.

## Limits

These are functional tests, not an IDM speed benchmark. Reuse avoids repeated connection setup but does not establish higher throughput. The explicit pool was qualified through local-DNS SOCKS; broader authenticated encrypted proxies/PAC, controlled HTTP/2 GOAWAY/reset recovery, HTTP/3 and real-site video behavior remain unqualified. The application bounds its own response chunk; libcurl may buffer additional paused HTTP/2 data internally. No new GUI parity claim is made.

Personal activation is pending. Browser/video, driver handoff, COM cold activation, signing and clean-machine gaps remain in the [gap register](idm-research-current-status-2026-09-29.md).

Installer: [UDM 0.81.0 / browser 0.53.1](D:/UDM/installer-out/UDM-0.81.0-Browser-0.53.1-Setup-x64.exe)

SHA-256: 7701e113fdad6bb736aef863a2cc8a33fa4b1a81436d73d097ed802cbb5a3342

Recoverable source backup: D:\UDM\backups\source-081-pool-20261001. Both dependency licenses are included.
