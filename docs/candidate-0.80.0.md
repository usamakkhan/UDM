# UDM 0.80.0 candidate — HTTP/2 transport negotiation

Verified 30 September 2026. **Source and installer are delivered; the personal desktop still runs 0.78.0.** Browser files remain 0.53.1. This candidate includes the 0.79 import/export and explicit-Resume changes. Full IDM parity remains unestablished.

## Behavior

Ordinary WinHTTP sessions now request HTTP/2 negotiation, keeping HTTP/1.1 available. Unsupported optional protocol selection on older Windows is tolerated; other configuration failures remain errors. The explicit curl transport now offers HTTP/2 for HTTPS and retains HTTP/1.1 for cleartext and HTTPS servers without h2. It includes pinned nghttp2 1.70.0, built statically with the existing MSVC runtime. No extra DLL or global runtime install is introduced.

`Http::protocol()` reports the negotiated protocol for both paths. The implementation follows [Microsoft's protocol options](https://learn.microsoft.com/en-us/windows/win32/winhttp/option-flags#winhttp_option_http_protocol_used) and [curl's HTTP version selection](https://curl.se/libcurl/c/CURLOPT_HTTP_VERSION.html). Peer and hostname verification remain enabled. This work did not add a trusted certificate or modify drivers, browsers, proxy configuration, or the hosts file.

## Actual acceptance

- **2,465 native checks passed** after recompiling the changed transport and tests.
- **45 checks across 27 actual transfer cases passed**. Both the explicit local-DNS SOCKS route and ordinary WinHTTP negotiated HTTP/2. Deterministic full/ranged bodies, a 2,018,996-byte release archive across repeated reads, form POST, HEAD, repeated cookies, and cancellation after a 200 HTTP/2 response were verified. Both paths fell back to HTTP/1.1 at an HTTPS origin without h2 and rejected self-signed/wrong-host certificates.
- Local HTTP/1.1 tests retained exact POST context with one POST, preserved redirect bytes, rejected truncated data, and canceled a stalled request. The temporary SOCKS listener bound only loopback and restricted destinations to the fixture and enumerated public test endpoints.
- The initial ordinary WinHTTP control reported HTTP/1.1 before the protocol option was added; the final control reports HTTP/2 with identical expected bytes. No IDM speed comparison was run.
- Personal history remains byte-identical with 25 records. Runtime executables in the installed release folders were not replaced. All new large build outputs are on D:.

The first public endpoint, nghttp2.org/httpbin, timed out in the new adapter, direct raw curl HTTP/1.1 and HTTP/2, and the existing WinHTTP route. Those failures are retained. The successful matrix used [go-httpbin's documented public fixtures](https://httpbingo.org/) and badssl certificate/fallback endpoints. The fixture rejected the requested 2 MiB range body with an explicit HTTP 400 and a 512 KiB maximum; the large-body case therefore uses the official nghttp2 release archive and its independently verified published SHA-256 instead. The rejected size-limit run is retained. These results do not identify why the original endpoint failed.

The first full native run found two timing failures in 0.79's Resume tests: they stopped waiting at file completion while the worker still held its active slot. The harness now waits under the manager lock for completion **and** worker cleanup before checking the temporary intent and next-slot scheduling. Production scheduling code was not changed in 0.80.

## Limits

Negotiated HTTP/2 support does not establish identical transport implementation or higher speed than IDM. CurlHttp still owns a separate multi handle per request, so this change does not introduce shared multiplexing across its separate transfers. The real route matrix here covers ordinary direct WinHTTP and explicit local-DNS SOCKS; encrypted-proxy authentication, PAC variants, HTTP/2 GOAWAY/reset recovery, and broader route combinations still need qualification. HTTP/3/QUIC is not implemented by this change.

The prior 0.79 MFC checks are retained evidence for unchanged GUI behavior; no new GUI parity claim is made. Personal browser activation, video/ad association, automatic driver handoff, signing, clean-machine lifecycle and matched-route IDM comparisons remain open in the [gap register](idm-research-current-status-2026-09-29.md).

Installer: [UDM 0.80.0 / browser 0.53.1](D:/UDM/installer-out/UDM-0.80.0-Browser-0.53.1-Setup-x64.exe)

SHA-256: `ee9b2d76d32b3b068a14d0c906f55f8a35b6ed149dcb91f2f05800510c9906e9`

Both dependency licenses are included. Recoverable source backup: `D:\UDM\backups\source-080-http2-20260930`. No Git commit/push or personal app restart was performed.
