# UDM 0.9.0 transfer-engine improvements

UDM now redistributes unfinished ranges to idle workers and shares an explicit WinHTTP session/connection context across the requests belonging to a download. This update improves the ordinary HTTP/HTTPS engine used by the native desktop.

## What changed

- Keep the existing initial queue of roughly four chunks per configured connection. After the queue is exhausted, an idle worker takes half of the largest unfinished active range, provided each half retains at least 256 KiB. The configured worker limit remains unchanged; plans are bounded to 4,096 segments.
- Each part retains a stable file ID. The new range boundaries are saved atomically before a worker can write under the new layout. Writes and splitting share a lock so in-flight bytes beyond a reassigned boundary cannot enter the wrong part. Such extra network bytes are included in transfer accounting.
- Assemble parts in file-offset order, validate saved plans for full coverage without overlaps/gaps, and reuse the verified partial files after restart. Existing 0.8.0 fixed-chunk histories resume in 0.9.0. Use 0.9.0 for jobs already split by 0.9.0; the older engine assembles by part ID and cannot interpret the new order correctly.
- Share one HTTP session per download and retain origin-specific WinHTTP connection handles. Fully consume the one-byte probe so its connection can return to the pool. Cookies and authorization remain request-local; cross-origin redirects still strip sensitive headers. A server may still close connections.
- Preserve range/length validation, retries, changed-resource recovery, SHA-256 checks, speed limits, destination collision protection, and atomic publication. No driver or system networking changes were required.

The driver presence in the previous IDM observation is not evidence of a download-speed advantage. IDM documents dynamic segmentation and connection reuse, which motivated these original UDM changes. [IDM documentation](https://www.internetdownloadmanager.com/support/segmentation.html).

## Correction to the earlier connection diagnosis

Creating a WinHTTP session per request did **not** prove that UDM made a new TCP/TLS connection for every chunk. Microsoft documents anonymous connection pooling across sessions, and the old engine used **five TCP sockets for seventeen requests** in this fixture. The new engine used four sockets, including reuse of the probe connection. The benefit of explicitly retained sessions is not equivalent to eliminating a handshake per chunk. [Microsoft WinHTTP overview](https://learn.microsoft.com/en-us/windows/win32/winhttp/about-winhttp).

## Controlled before/after measurement

Both builds downloaded the same complete 32 MiB deterministic file from a loopback HTTP/1.1 fixture with four workers and persistent connections. Each accepted connection waited 40 ms; each 64 KiB response block waited 1 ms. In the slow-connection case, the range starting at zero waited 120 ms per block instead. Actual Windows scheduling affects these waits. Each test used fresh state and verified the whole-file SHA-256. Timing includes transfer, assembly, verification and publication, and excludes fixture preparation. These are synthetic local timings, not Internet throughput measurements.

The three pairs ran serially in alternating order: old/new, new/old, old/new. No other UDM benchmark or native test ran concurrently.

| Scenario | UDM 0.8.0 median | UDM 0.9.0 median | Change in completion time |
| --- | ---: | ---: | ---: |
| Steady connections | 3.509 s | 3.437 s | 2.0% shorter; small difference within observed variation |
| One slow connection | 4.831 s | 4.263 s | **11.8% shorter** |

| Scenario | Version | Run 1 | Run 2 | Run 3 |
| --- | --- | ---: | ---: | ---: |
| Steady | 0.8.0 | 3.508603 s | 3.355447 s | 3.623459 s |
| Steady | 0.9.0 | 3.395326 s | 3.524375 s | 3.436992 s |
| Slow connection | 0.8.0 | 4.775411 s | 4.830792 s | 4.840703 s |
| Slow connection | 0.9.0 | 4.392644 s | 4.230085 s | 4.263109 s |

All twelve files passed SHA-256 verification. In the new slow-connection runs, one or two dynamic splits occurred; the old engine performed none. The new engine made 18–19 requests versus the old engine's 17. Helping a slow range can require another request and can discard a small amount of already in-flight data.

Three repetitions on a PC in ordinary use do not establish universal improvement. The prior simultaneous IDM/UDM ISO test used different CDN endpoints and remains unsuitable for claiming an isolated speed win. No fresh Microsoft ISO/IDM race was performed for 0.9.0.

## Validation and files

**91 native checks passed, zero failed.** New checks cover real socket reuse, live splitting, hash-correct offset assembly, worker limits, orphan tail replacement, pause after a split, persisted state reload, exact resume, and compatibility with old fixed-chunk partials. Existing checks cover non-range/unknown-length/empty servers, invalid ranges, truncated responses, redirect credentials, hash mismatches, destination preservation, media assembly, browser handoff and IPC.

The regression fixture uses a stronger 240 ms slow-block delay to make the split/resume condition reliable. Performance measurements above use 120 ms in both builds.

Source: `native/Transfer.cpp`, `native/Core.hpp`, `native/TransferChecks.hpp`, `native/TransferFixture.hpp`. Local benchmark sources, old executable, raw results and source backups are retained under `benchmarks/engine-2026-09-20`; private benchmark data is excluded from the portable archive. `release-native/native-test-evidence.json` records the final native suite result.

Release: `release/UDM.exe` (0.9.0). Browser extension code remains 0.8.0 because its code and protocol did not change. Download history, browser registration and media helper paths remain compatible.
