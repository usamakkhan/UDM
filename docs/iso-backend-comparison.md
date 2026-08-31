# Microsoft ISO comparison — 19 September 2026

IDM transferred more data during the concurrent test. This is evidence about bandwidth sharing on this PC, not yet an isolated, full-download speed ranking.

Tested builds: UDM 0.8.0 x64 transfer engine and installed IDM 6.43 build 10 (file version 6.43.10.2). Both downloaded the same Microsoft-generated Windows 11 25H2 English x64 v2 URL, with a total size of **8,471,603,200 bytes (7.889 GiB)** and **eight active connections each**. The server supported byte ranges and supplied an ETag. The ISO was not installed or mounted.

## Measured transfer rates

| Interval | Duration | IDM | UDM |
| --- | ---: | ---: | ---: |
| Before UDM started | 18.18 s | 4.45 MiB/s / 37.35 Mbps | Idle |
| Both downloading, same measurement window | 72.52 s | **3.07 MiB/s / 25.77 Mbps** | **1.47 MiB/s / 12.36 Mbps** |
| After UDM paused | 50.32 s | 4.53 MiB/s / 37.99 Mbps | Paused |

IDM received about **2.08 times** UDM's throughput during the shared interval. Their combined rate was approximately **4.55 MiB/s**, close to IDM's rate without UDM running. This is consistent with competition for limited shared bandwidth; it does not locate the bottleneck or prove that UDM would be 2.08 times slower alone.

Earlier, IDM's visible dialog showed approximately **4.865–5.304 MB/sec**. A separate 46.87-second measurement using its cumulative displayed download count averaged **4.17 MiB/s**. Instantaneous display rates and averages over different intervals need not match. At 20:07:37 UTC, IDM's persisted speed counter was 5,382,898 bytes/sec, approximately **5.13 MiB/s / 43.06 Mbps**, with 6,022,968,691 bytes received (71.10%). This was a point-in-time reading, not another timed average.

UDM's entire bounded run lasted approximately **91.04 seconds**, including startup, and received **131,989,504 bytes (125.875 MiB)** at **1.38 MiB/s** on average. It paused cleanly with no reported transfer error and retained resumable parts. IDM's user-started download was left running.

## Method and limitations

- UDM ran through a small console harness linked to the **existing production Core, Transfer, Streaming, Adaptive, Bridge and Network object files**. The transfer implementation was unchanged. The harness omitted the MFC interface and browser handoff; this was an engine test, not a complete desktop-app benchmark.
- Both used eight actual TCP connections. UDM's eight observed connections reached **151.101.22.172:443**. IDM's snapshot included **2.16.168.46**, **2.20.245.171**, **23.15.3.140** and **146.75.38.172**. The same URL therefore did **not** mean the same CDN endpoint. Endpoint selection is a material confounder.
- IDM's existing persisted download counters were read without changing its registry settings. Those counters update in batches. UDM sampled its cumulative byte count about once per second; its count was linearly interpolated onto the selected IDM sample timestamps. Rates are approximate.
- The common window was **20:01:25.816–20:02:38.332 UTC**, excluding the first roughly ten seconds and the end of the UDM run. Pre-run and post-run IDM measurements are separate intervals, not controlled repetitions.
- The computer remained in ordinary use. Background traffic and server conditions were not controlled. No unrelated applications were stopped.
- An isolated UDM run was not completed because IDM continued downloading and desktop input was unavailable while the user was active. A request to pause IDM remained pending. This report does not substitute simultaneous results for a sequential comparison.
- Neither full-file completion time nor assembly/hash time was measured. UDM's partial file cannot be checked against Microsoft's whole-ISO SHA-256. No complete ISO hash verification is claimed.
- CPU/memory logs were captured, but are not presented as an app-to-app efficiency ranking because UDM's UI was absent and measurement intervals differed.

## Follow-up on 20 September

The per-request WinHTTP sessions observed below did not establish a TCP/TLS handshake per chunk: Windows pools anonymous connections across sessions, and the controlled 0.8.0 fixture subsequently demonstrated reuse. UDM 0.9.0 adds explicit session sharing and dynamic splitting. See [the implementation and measured before/after results](performance-0.9.md). The ISO measurements below remain observations of 0.8.0.

## Backend comparison

| Area | UDM 0.8.0 | IDM reference | Practical implication |
| --- | --- | --- | --- |
| Language/UI | Original native x64 C++17/MFC; static CRT/MFC | Native x86 executable with C++ RTTI/MFC class evidence | Language and 64-bit addressing alone do not establish higher download speed. |
| HTTP transport | Source-confirmed synchronous WinHTTP requests, Windows TLS, 64 KiB read/write buffers | Running process loads Winsock, WinINet, WinHTTP, libssl and libcrypto | Loaded modules do not prove which APIs serve an IDM download. Its full transport implementation remains unknown. |
| Segments | Precomputed queue of roughly four chunks per connection, minimum 1 MiB; workers pick the next chunk | Vendor documents dynamic division of the largest remaining segment, with finished workers helping slower ones | UDM does not currently split a slow active chunk; a potential disadvantage near completion, not measured by this short run. |
| Connections | Up to 16 workers. Each chunk creates and destroys its own WinHTTP session/request/connection | Vendor documents connection reuse without another connect/login phase | Session/connection reuse is a concrete UDM improvement to investigate. This run did not isolate handshake overhead. |
| Resume | Validates range/total and ETag or Last-Modified; uses partial-file lengths; retries and handles changed resources | Documented persisted segment positions and error recovery; ISO dialog reports resumable | Range support must be confirmed per server. This test did not exercise every failure case. |
| Output | Assembles private part files, computes SHA-256, verifies an expected hash when supplied, and publishes without overwriting an existing destination | Vendor documents assembly of downloaded segments | Final assembly and hashing must be included in a future complete-download comparison. |
| Driver | Optional original WFP monitor; no UDM driver loaded | IDMWFP service observed running | Driver presence does not establish a speed boost for a direct HTTPS ISO. This test does not isolate driver effects. |
| State | Atomic JSON and current-user DPAPI for saved credentials | Proprietary settings/history implementation | UDM's source allows direct audit; undocumented IDM internals cannot be inferred as facts. |

UDM source evidence: `native/Transfer.cpp` (HTTP lifetime, chunk queue, resume and publication), `native/Core.cpp` (rate limits, scheduling and progress), `native/Bridge.cpp` (native host/pipe). Reference binary evidence: `docs/reference/pe-analysis.json` and `docs/language-and-signing.md`.

The clearest source-level gaps are **connection/session reuse** and **dynamic splitting of slow remaining segments**. They are candidates for separate, measured experiments. The current result does not establish that either gap, a driver, or the programming language caused the observed difference.

## Reproducibility and next comparison

Local evidence is under `benchmarks/iso-2026-09-19`, excluded from packages. It includes `comparison-summary.json`, `shared-idm-samples.json`, `udm-engine-c29160b0fedc4abea38fa2d6170ddd41.json`, `concurrent-connections.json`, the initial UI samples, and the bounded harness source/build script. The expiring Microsoft URL stays in the local input fixture.

A fairer follow-up should download sequentially with eight connections and no speed limit, record actual CDN endpoints, repeat in alternating order, and compare cumulative bytes over equal steady intervals. A full-completion test should separately record startup, network transfer, assembly, and SHA-256 verification. Server and background network variation must still be reported.

## Sources

- [Microsoft Windows 11 ISO page](https://www.microsoft.com/en-us/software-download/windows11). Published SHA-256 for the selected image: `768984706B909479417B2368438909440F2967FF05C6A9195ED2667254E465E3`.
- [IDM dynamic segmentation and connection reuse](https://www.internetdownloadmanager.com/support/segmentation.html).
- [IDM speed factors and configuration](https://www.internetdownloadmanager.com/register/new_faq/functions8.html).

Rates use **MiB/s = bytes/sec ÷ 1,048,576** and **Mbps = bytes/sec × 8 ÷ 1,000,000**. IDM's displayed MB/GB counts matched binary units for this file.
