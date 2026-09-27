# UDM driver backend 0.2
Date: 2026-09-27

The driver backend was improved before desktop integration, following the requested order. This release adds original C++ functionality above the unchanged signed WinDivert driver. It does not reproduce IDM's private binary/device protocol or establish complete driver parity.

## Delivered functionality

1. Process trees: selected root processes plus live descendants, process handles and creation times, parent identity and bounded tracking. Metadata retains WinDivert timestamps, direction, loopback and parent endpoints.
2. Scoped TCP redirection: IPv4/IPv6 destination mapping, original connection ownership lookup, unique proxy-peer aliases and checksum repair. The helper's own upstream sockets are excluded, avoiding self-redirection. Connections with unknown/unselected owners bypass unchanged.
3. Ordered stream transport: separate Windows TCP sockets provide ordering and retransmission handling. Queues are bounded, backpressure is applied by select/read readiness, half-closes are propagated, cancellation and idle timeout are implemented.
4. HTTP/1 inspection: split headers, fixed/chunked/close-delimited bodies, chunk extensions/trailers, ranges, content length/type/disposition, request/response pairing, redirects, HEAD and informational responses. Ambiguous framing disables interpretation while preserving forwarding.
5. Controlled interception: an explicit C++ callback can replace the first eligible GET response with a 204 response and close the upstream stream. It is exercised only with owned fixtures; the CLI observes candidates without taking over downloads.
6. Encrypted/upgraded traffic: TLS and unsupported protocols pass as opaque streams. The gateway does not decrypt HTTPS or install a certificate.
7. Bounded failure handling: unknown ownership or full capacity bypasses new connections; timeouts and errors are reported; client resets are distinguished from upstream failures; unaccepted/completed route entries expire.

The TCP socket approach supplies stream behavior without adding a custom kernel TCP reassembler or modifying the signed driver. It is not WFP STREAM/ALE API identity.

## Final validation

| Suite | Passed | Failed | Meaning |
|---|---:|---:|---|
| Backend core and live gateway | 43 | 0 | Includes 34 parser/relay/identity checks and 9 elevated checks, including IPv4/IPv6 transfers and four concurrent children |
| Existing driver regression | 38 | 0 | Signed loading, TCP/UDP local copying/reinjection/redirection, attribution, queue bounds and handle lifecycle |
| CLI and runtime validation | 11 | 0 | Input validation, normal-user privilege boundaries and tampered-runtime rejection |
| External HTTPS fixture | 1 | 0 | Public example.com HTTP 200 through the gateway with normal Windows TLS validation |

These suites contain some overlapping guards. They are not an identity percentage; the standalone core run is not added again. All runs use the final /W4 /WX build without compiler warnings.

Local gateway transfers verified exact HTTP and opaque content in IPv4 and IPv6, plus synthetic interception responses. The parallel fixture verified four completed routes and four candidates. An excluded child transferred unchanged with no redirect.

One earlier run exposed a short-lived child attribution race. Direct parent lookup from a held process object, with Toolhelp fallback, replaced repeated whole-process snapshots on the common path. A subsequent external test exposed client disconnects being counted as upstream failures. Separate reset counters and explicit client/upstream reset tests fixed that classification. Final review found and fixed a 100-Continue forwarding stall; a real socket fixture now waits for the interim reply before sending its request body.

The HTTPS fixture's child exit code is 0, the server returns 200 and bytes are read under Windows certificate validation. Any recorded client reset is explicitly retained as a client termination, not silently removed. HTTPS content produces no plaintext download candidates.

Test Mode remained off and Code Integrity remained on in the driver regression. Memory Integrity was off. The unchanged signed runtime remains subject to the security configurations on which it has actually been tested.

## Remaining before claiming full IDM driver parity

- No reconstruction of IDM's complete private driver/native control contract, legacy RTMP/RTMPT/RTSP handling or proprietary local WebSocket process-header behavior.
- Multipart byte-range bodies are forwarded, but their individual parts are not parsed. HTTP/2, HTTP/3 and QUIC are not inspected; this gateway redirects TCP only.
- HTTPS decryption and browser media identity/quality/ad classification are not supplied by the driver backend.
- No claim of crash-transparent continuation for already redirected TCP connections. A helper crash/shutdown can end those connections.
- Proxy/VPN/security-product compatibility, interface changes, scoped/link-local IPv6 routing, fragmented traffic, suspend/resume, Driver Verifier, sustained pressure, Secure Boot-on and Memory Integrity-on acceptance remain incomplete.
- Very short-lived children can disappear before ownership discovery. Existing connections are not retroactively redirected. Matching process handles reduces PID-reuse risk; this is not a kernel process-notification API.
- Distribution remains x64. IDM's other architecture packages are not matched here.
- The main desktop, extension, download engine and old installer are not migrated. The GUI still uses the old UdmWfp device; its existing installer still has development-driver/Test Mode behavior. That integration is the next stage after the remaining backend acceptance work.

## Resource/transport contract

Explicit 1-32 roots and 1-32 TCP destination ports, up to 256 tracked process identities, 64 active/retained routes, 512 pending metadata events, 64 pending HTTP requests, 64 KiB headers, 128 header fields and 64 KiB per-direction relay buffers. CLI sessions last 1-600 seconds; open idle relay streams time out after 60 seconds. Completed route entries expire after 30 seconds and unaccepted entries after 10 seconds.

Public commands: --status, --watch, --watch-tree, --redirect-watch, --core-test, --gateway-test, --self-test, --internet-test. Redirect-watch has no automatic download takeover.

Candidate URLs/redirect locations remain in memory for callbacks; authorization/cookie fields and bodies are not retained in metadata or printed by these commands. Original traffic bytes still flow to the original destination. A future desktop broker must supply an explicit, authenticated handoff contract.

## Files and evidence

Sources are in drivers/signed-network: ProcessScope.hpp, HttpStream.hpp, StreamRelay.hpp, TcpGateway.hpp, NetworkBackend.hpp and NetworkMain.cpp. CoreTests.hpp and InternetTest.hpp contain executable acceptance fixtures.

Runtime: release/network/Udm.Network.exe. Existing WinDivert runtime, source archive and licenses are unchanged.

Accepted reports are under drivers/signed-network/tests with a -0.2 suffix. Full logs, earlier failing diagnostics, staged sources and deployment backups are retained in:
C:/Users/Abuzar/.codex/visualizations/2026/09/18/01a0b440-f9cc-7b50-9c2a-369a977ea681/driver-core-20260927

## Primary technical references

- [WinDivert API](https://reqrypt.org/windivert-doc.html): packet layers, PID metadata, injection and limitations.
- [WinDivert stream example](https://github.com/basil00/WinDivert/blob/master/examples/streamdump/streamdump.c): documented packet-reflection/proxy approach; UDM's scope, routing, parser and relay implementation are original.
- [HTTP/1.1 framing, RFC 9112](https://www.rfc-editor.org/rfc/rfc9112.html): response and body framing.
- [Microsoft process information](https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntqueryinformationprocess): dynamically resolved basic process information, with fallback.
- Local IDM call-site evidence is documented in the preceding driver gap audit. No IDM code or driver binary is part of this release.

