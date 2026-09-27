# UDM 0.10.0: IDM transport analysis and measured improvements

Recorded 2026-09-20. UDM now redistributes slow ranges before the initial queue runs out. On the controlled local slow-response fixture, median full-file completion improved from 5.279 seconds in UDM 0.9.0 to 4.206 seconds in 0.10.0 (20.3% less time). Live IDM tests also completed with matching hashes. This establishes useful local progress, not general Internet or feature parity.

## What the IDM inspection establishes

The inspected x86 IDMan.exe reports version 6.43.10.2 and has SHA-256 `03cc62e9adb77a380f9dc12f67ccaaee5106f12844aa73ce32c914ddd16d607c`. Addresses below are preferred virtual addresses at image base 0x400000; ASLR can relocate them. Inspection was read-only. The UDM implementation remains independently written C++/MFC.

The PE inspector previously covered ordinary imports. It now also reads delay imports, an important omission because this executable delays loading its socket functions. Inspection of ten installed files completed without parser errors; IDMan contains 105 dialog resources.

| Finding | Evidence | Interpretation and limit |
| --- | --- | --- |
| Direct socket transport | WS2_32 delay imports include socket, connect, select, recv, send and ioctlsocket. Receive references occur around 0x5A4380, 0x5A5D88 and 0x5A6186; send around 0x5A39CD. | Strong evidence of a custom socket path; imports alone do not identify the path used by every download. |
| Nonblocking TCP | The function beginning at 0x5964D0 calls socket with AF_INET/SOCK_STREAM and invokes ioctlsocket with FIONBIO (0x8004667E) and a nonzero argument. | This particular path creates a nonblocking IPv4 TCP socket. It does not recover the complete event loop or scheduling policy. |
| OpenSSL binding and calls | SSL_new, SSL_connect, SSL_read and SSL_write are resolved dynamically around 0x5F0149 onward. SSL_read is stored at 0x788AC0 and called around 0x5F1437; SSL_write references occur around 0x5EFB16 and 0x5F2E30. | Supports a custom socket/OpenSSL implementation. One inspected read requests 4 KiB; this does not establish a universal download-buffer size. |
| HTTP ranges | Range-header format strings are referenced by executable code, including the bounded-range string at 0x6CE980 and references around 0x58B66E and 0x5AEF76. | Consistent with the range requests directly observed in the local test. Exact scheduling thresholds remain unknown. |
| WinHTTP proxy handling | Code around 0x4B8950 resolves WinHttpGetProxyForUrl, WinHttpOpen and WinHttpCloseHandle. Additional strings cover automatic proxy detection and current-user proxy configuration. | WinHTTP strings do not establish that IDM downloads file bodies through WinHTTP. They are used for proxy handling in this inspected path. |

The evidence supports a custom socket/TLS path. It does not prove that replacing UDM's WinHTTP layer would improve speed, nor that a kernel driver explains IDM's download performance. No IDM executable, extension code, driver, activation data or extracted artwork was incorporated into UDM. No driver, Test Mode or system network setting was changed.

Structural metadata and address references are in [reference/](reference/README.md). The full application source and exact internal algorithms have not been recovered.

## Live observation of IDM

IDM was configured for eight connections, with no server-specific exception visible. Six fresh 32 MiB files were downloaded serially through its normal queue. Request timestamps confirm there was no overlap between these six downloads. Every published file matched SHA-256 `66b984d4671b3299be00d015d20e04e59412d99fcc8bf6ee933a6038632044f5`.

In the first steady run, IDM opened a normal GET followed by ranges beginning at 116800, 16851999, 25203215 and 8517167. About 1.54 seconds after the initial request it added four more ranges. Several original requests extended to the end of the file and were closed early as ownership was redistributed. Peak outstanding responses was eight. This is observable behavior on this file, not a reconstructed universal algorithm.

IDM made nine requests per steady file and 9-11 per slow file. UDM 0.10 made 33 and 36-37 respectively. IDM therefore used fewer requests in this test. UDM reused connections across its smaller queued chunks, so request count is not the same as connection count: UDM used eight distinct sockets for steady runs and nine over each entire slow run; IDM used 9-11 over each entire run. Lifetime socket counts are not concurrent worker counts.

An earlier IDM batch accidentally ran with overlapping files because editing the queue field did not apply it before Start now. Those results are excluded from the comparison. The serial repeat explicitly applied the setting. The original four-file queue preference was restored afterward. The expired-trial notice was dismissed through its normal UI without changing registration; the observed downloads completed.

## UDM changes

A worker that finishes a chunk can now help a sustained slow worker before taking another queued chunk. Decisions require at least 0.4 seconds and 64 KiB of observations, a helper rate at least twice the owner's rate, and at least 0.75 seconds of estimated remaining work. Each resulting half retains at least 64 KiB. Without a reliable rate difference, the previous post-queue fallback retains its larger 256 KiB minimum per half.

The existing eight/sixteen-worker configuration limit, 4,096-part bound, validated range responses, resource validators, retry handling, speed limits and SHA-256 checks remain in force. Splits and writes share the manager lock. New boundaries are saved before writing, and parts assemble by byte offset. Existing 0.9 plans and earlier fixed-chunk partial downloads remain compatible.

The server trace confirms that helpers joined the first slow chunk after 0.684-0.889 seconds in the three new runs, before the final queued chunk began at 2.443-2.662 seconds. The older build never reassigned this particular first chunk in these runs. The native regression test separately verifies an early helper request and an exact full-file hash.

## Controlled UDM before/after measurements

Both builds use eight workers and the same complete 32 MiB deterministic payload. The HTTP/1.1 loopback server emits 32 KiB blocks with an 8 ms timer; the slow case applies a 125 ms timer to the non-probe response starting at zero. Windows timer scheduling affects actual intervals. Each run uses fresh state and a fresh output name. The production transfer engine runs through a headless harness; timing includes transfer, assembly, SHA-256 verification and publication.

Three pairs ran serially in alternating version order: old/new, new/old, old/new. Within each version, steady precedes slow. No competing benchmark ran concurrently.

| Scenario | UDM 0.9 median | UDM 0.10 median | Completion time change |
| --- | ---: | ---: | --- |
| Steady responses | 3.856 s | 3.801 s | 1.4% shorter; too small to establish improvement |
| Slow first response | 5.279 s | 4.206 s | **20.3% shorter** |

| Scenario | Version | Run 1 | Run 2 | Run 3 |
| --- | --- | ---: | ---: | ---: |
| Steady | 0.9 | 3.856380 | 3.687501 | 4.135481 |
| Steady | 0.10 | 3.800875 | 3.699578 | 3.852497 |
| Slow | 0.9 | 5.255977 | 5.501478 | 5.279428 |
| Slow | 0.10 | 3.859737 | 4.230518 | 4.206252 |

All twelve UDM files passed hash verification. Raw samples, harness hashes and request summaries are in [performance-0.10.json](reference/performance-0.10.json).

## IDM versus UDM on the local fixture

The same server measured the interval from each file's first request to closure of its last response. This provides a common clock and definition for the network comparison; it excludes client-side publication and is not end-to-end application completion.

| Scenario | IDM median server interval | UDM 0.10 median server interval |
| --- | ---: | ---: |
| Steady | 3.506 s | 3.018 s |
| Slow first response | 3.892 s | 3.361 s |

For context, a separate 100 ms file poll observed IDM's hash-correct output after medians of 3.727 seconds (steady) and 4.262 seconds (slow), measured from its first server request. UDM's internal full-engine medians were 3.801 and 4.206 seconds. These differently measured completion estimates are close, but cannot establish a precise winner: IDM ran through its GUI, UDM through a headless harness, and their state/output locations differ. The polling endpoint includes observer scheduling and hashing overhead. IDM's import-time metadata requests are excluded from the measured queue run.

The IDM batch ran after the UDM batch, rather than alternating applications. There are only three repetitions per scenario. This local HTTP fixture has no Internet latency, TLS handshake, Microsoft CDN throttling or multi-gigabyte disk load. The slow rule is attached to the response beginning at zero, not a permanent network-path defect. Results cannot establish broad parity or a Microsoft ISO speed win. Exact request histories and samples are in [idm-comparison-0.10.json](reference/idm-comparison-0.10.json).

## Validation and release

Validation covered real HTTP sockets, connection reuse, splitting before and after queue exhaustion, ordered assembly, pause/reload/resume, legacy partial plans, changed resources, malformed/truncated responses, credentials across redirects, destination preservation, media assembly and browser IPC. The early-help regression fixture uses stronger 240 ms slow/20 ms normal delays and four workers for a reliable observable condition; it is separate from the performance fixture above.

Sources: `native/Transfer.cpp`, `native/TransferFixture.hpp`, `native/TransferChecks.hpp` and `tests/inspect-reference.cjs`. Reproducible local benchmark sources are under `tests/performance-0.10`. Existing state, binaries and full local traces remain under `benchmarks/idm-protocol-2026-09-20` and are excluded from the portable package.

The desktop release is 0.10.0. The browser extension remains 0.8.0 because its code/protocol did not change. Complete IDM feature parity remains outstanding. The next evidence-based optimization is reducing unnecessary range requests while preserving resume validation, and measuring it against larger, latency-bearing fixtures before changing the transport library.
