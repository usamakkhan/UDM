> Backend 0.2 supersedes the implementation state below. See [driver-backend-0.2.md](driver-backend-0.2.md) for new features and acceptance results. The report below is retained as the 0.1 history.

# UDM / IDM driver behavior: measured progress, 2026-09-27

**There is no basis for certifying 99.99% identity.** IDM's complete runtime policy and private driver/client contract remain unknown. Matching imported APIs or passing a finite test suite cannot establish an identity percentage.

The new optional UDM signed-network backend is original C++ code using the unmodified WinDivert 2.2.2-A driver. It is separate from UDM's existing C WFP monitor and is not yet connected to the desktop Network integration window or download interception workflow.

## Implemented and actually tested

- Microsoft-signed driver loaded successfully under normal Windows Code Integrity, with Test Mode off before and after.
- Process-scoped passive flow/socket monitoring, exact IPv4/IPv6 endpoint formatting, connection close events, bounded queues and explicit loss counts.
- IPv4/IPv6 TCP and UDP traffic preserved exact payloads under passive copying and unchanged packet reinjection.
- Both IP families/protocols also passed bidirectional loopback port redirection, including checksum repair. Clients contacted a reserved original port and received the expected content from a different fixture server.
- An excluded child process completed its UDP exchange; the selected process's event stream did not include the child's PID.
- Eight monitor start/stop cycles and idle receive cancellation completed.
- Queue pressure from 700 short-lived UDP sockets remained bounded and reported dropped observations.
- Runtime DLL/driver hashes are pinned and checked before loading. Both altered-file rejection tests passed.

Final build: MSVC C++17, /W4 /WX, no warnings. Final live suite: **38 passed, 0 failed**. Separate normal-user/argument/tampered-runtime validation: **8 passed, 0 failed**. Earlier overlapping runs are not added to this total. An IPv4/IPv6 word-order formatting error was found by the live endpoint check and fixed before the passing final run.

The installed copy independently passed the same 38 live checks. The total remains 46 unique checks, not 84. All 16 installed component files were hash-verified, the existing application and 23-record user history remained byte-identical, and IDM's installed driver hash and running status were unchanged.

The final live report records 216 monitored events and 17 successful fixture transfers. TCP payloads were 256 KiB each direction; UDP fixtures sent 32 distinct 1024-byte payloads and checked every echo. Recorded timings are tiny local functional fixtures, not an IDM/UDM Internet speed comparison.

IDMWFP was running before and after this testing. That establishes bounded coexistence during these fixtures, not exhaustive compatibility with IDM, VPNs or other security software. Memory Integrity was off. Driver Verifier, Secure Boot-on acceptance, suspend/resume, physical network changes and long-duration stress were not run.

## Behavior matrix

| Area | IDM evidence | New UDM backend result | Remaining work |
|---|---|---|---|
| Normal signed-driver loading | Installed Microsoft-signed driver running | Live pass with Test Mode off | Test additional Windows security configurations |
| Process/flow attribution | Imports and native integration evidence | Explicit-PID flow/socket monitoring passed | Desktop broker/UI integration; newly spawned browser workers |
| IPv4/IPv6 TCP/UDP handling | WFP layer/import evidence | Exact-content local traffic tests passed | Remote routes, fragmented traffic and real QUIC sessions |
| Stream inspection/copying | TCP stream-copy imports | Packet-copy fixtures passed | Ordered stream API/reassembly and browser-session correlation |
| Reinjection | Stream/transport injection imports | Unchanged packet reinjection passed | Arbitrary traffic, asynchronous failure handling and sustained load |
| Connection redirection | ALE redirection evidence | Loopback port rewrite prototype passed | General process-scoped proxy/redirection; loop prevention; fail-open recovery |
| Private client/device protocol | Present but not completely understood | UDM's own interface only | Functional behavior must be specified independently |
| Video URLs, quality, advertisement separation | Multiple IDM native/browser components | Existing UDM extension remains responsible | Driver access alone does not provide these decisions |
| Download acceleration | Separate app/transport behavior | No acceleration claim | Matched-source, matched-route IDM comparisons |
| Lifecycle and resource pressure | Production reference available | Repeated handle start/stop and queue bound passed | Kernel load/unload stress, verifier, power transitions, service recovery |

Local reference evidence: [component inspection](deep-inspection.md), [WFP layer map](reference/idm-wfp-layer-map.json). Imports/GUIDs show available mechanisms; they do not prove exactly how IDM uses them for a particular request.

## Integration state

Installed as an optional component under release/network, with source under drivers/signed-network. The existing UDM application and saved download history are preserved. No driver or private code from IDM was copied.

The signed helper supports explicit monitoring and self-tests. General browser interception is not enabled, and the production transfer path is unchanged. The old development-driver installer still has its separate Test Mode behavior; it has not been rebuilt here. Consequently this is not complete driver parity or a completed installer migration.

The meaningful next acceptance gate is a process-scoped desktop broker and general redirect/stream path, followed by real-browser tests and failure/recovery tests. A percentage should not be attached until the required behaviors, workloads and scoring method are explicitly defined.

## Evidence and sources

The complete local evidence is retained in the visualization workspace's driver-parity-20260927 folder: live-verified-results.json, validation-results.json, build-final.log, package-manifest.json and deployment-verification.json. These contain local fixture data, not browser cookies or video playback URLs.

WinDivert binary archive SHA-256: 63CB41763BB4B20F600B6DE04E991A9C2BE73279E317D4D82F237B150C5F3F15.
Corresponding source archive SHA-256: 8E904FBEEF2A6180FFC533E9B636AFD14282AF7510E421A9CB381D978302ACD9.

[WinDivert API](https://reqrypt.org/windivert-doc.html) documents process metadata, packet interception and the required capture flags. [Microsoft's WFP redirection documentation](https://learn.microsoft.com/en-us/windows-hardware/drivers/network/using-bind-or-connect-redirection) describes the distinct ALE redirection model. The [WinDivert FAQ](https://reqrypt.org/windivert-faq.html) distinguishes signed prebuilt binaries from custom builds needing signing.
