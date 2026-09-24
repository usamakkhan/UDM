# IDM and UDM transport investigation

Analysis performed 24 September 2026 UTC / 23 September Pacific time.

The tools are installed and have been used against both running applications. The investigation identifies concrete differences and a promising UDM read-size change. It does **not** establish a complete causal explanation for the earlier 17.0% Microsoft ISO throughput difference, or show that UDM now beats IDM.

## Tools installed and used

All tools are project-local under `D:\UDM\.media-cache\reverse-tools`.

| Tool | Version | Work performed |
|---|---|---|
| Ghidra | 12.1.3, already installed | Revisited the saved socket, TLS and segment-scheduler decompilation |
| Frida / frida-tools | 17.18.0 / 14.10.4 | Attached to both production processes and observed socket, HTTP, TLS, file-write and flush APIs |
| Capstone | 5.0.9 | Independently disassembled IDM's socket setup at virtual address `0x005964d0` |
| pefile | 2024.8.26 | Inspected PE architecture, import tables and image layout |
| Microsoft Sigcheck | Official Sysinternals download | Checked app signatures, versions and hashes; tool signature verified as Microsoft Corporation |

Python packages live in the isolated `instrumentation` environment. Pip's installation report records package sources and hashes. Sigcheck was obtained from Microsoft's download service; no VirusTotal/upload options were used. Ghidra was reused rather than reinstalled.

Documentation: [Frida installation](https://frida.re/docs/installation/), [Frida JavaScript instrumentation API](https://frida.re/docs/javascript-api/), [Microsoft Sigcheck](https://learn.microsoft.com/en-us/sysinternals/downloads/sigcheck).

## What was observed

| Behavior during the Microsoft ISO diagnostic | IDM 6.43 build 10 | UDM 0.15.0 |
|---|---|---|
| Application transport | Direct Winsock calls and a select/nonblocking-socket path | Asynchronous WinHTTP |
| Active TLS implementation | Windows Schannel | Windows Schannel beneath WinHTTP |
| Data connections / requests | Eight long-running HTTP/1.1 data GETs | Eight long-running HTTP/1.1 data GETs after one validation request |
| Initial request on resume | Data-bearing, open-ended range | `Range: bytes=0-0`, then bounded data ranges |
| Body delivery to application | Predominantly TLS plaintext in the 16 KiB histogram bucket | Requests 64 KiB; normal saved blocks were exactly 64 KiB |
| Request churn after startup | No additional data GETs in the observed minute | No additional data GETs in the corrected 44-second run |

**Correction to the earlier static inference:** IDM contains an OpenSSL loading path, but this ISO run used Schannel. The trace observed 15,480 successful Schannel decrypt calls in IDM and no OpenSSL reads. The mere presence of `libssl` strings or WinHTTP.dll in a process does not identify the active transfer implementation.

The Capstone disassembly confirms the `0x8004667e` nonblocking ioctl setup and socket arguments in the routine identified by the earlier Ghidra analysis. This is inspection of compiled machine code; it does not recover IDM's original C++ source.

Neither application's observed socket setup set `SO_RCVBUF`. IDM made no observed `setsockopt` calls during these newly established connections. This does not prove which options might have been set before attachment. Kernel drivers were not instrumented, so these results neither prove nor disprove every driver role. They provide no evidence that a special driver explains the speed difference.

## Specific UDM costs and candidates

1. **A serial validation step before data transfer.** The first trace showed 450 ms between starting the one-byte probe and the first data GET; the corrected repeat showed 493 ms. This includes DNS/connection/TLS and validation work, not just one HTTP round trip. IDM's first GET already carries file data. A future implementation can validate headers on the first useful data request and then launch the other workers; it must retain Content-Range, size and entity-validator checks. Roughly half a second alone cannot account for the earlier 17% difference over two minutes if steady rates are otherwise equal.

2. **Read/callback/worker handoff.** UDM waits for each WinHTTP read callback, wakes its worker, writes that block, updates progress, then submits the next read. IDM directly handles its socket and decrypted data. This difference is verified; it is not proof that WinHTTP itself is inherently slower. UDM's condition-variable wait is notified by the callback: its 25 ms timeout is not a forced 25 ms delay per read.

3. **Shared locking and synchronous state persistence.** UDM holds the manager mutex across the normal partial-file WriteFile and progress update. Its GUI tick also saves state every third 500 ms tick, including while idle. The original trace observed 105 state writes and flushes over 155.5 seconds, mostly before UDM's transfer started; the repeat observed another 76 over 109.8 seconds. Snapshotting state under a short lock and reducing unchanged-state writes are justified candidates for profiling. Lock contention has not yet been timed, so its share of the ISO gap is unknown.

Source locations: `native\Transfer.cpp` (session, callback/read path and HTTP worker around line 274), `native\Core.cpp` (`Manager::tick`, line 89), `native\App.cpp` (500 ms timer, line 108).

## Controlled read-size experiment

Two isolated console binaries were built from the current native engine and current core objects. The only transport-source difference was changing the HTTP worker buffer from 65,536 to 16,384 bytes. Both variants ran without Frida. The production app, source and release executable were not changed.

The existing deterministic local HTTP fixture supplied identical 32 MiB payloads with eight connections. Three pairs per workload alternated baseline/candidate order: **18 completed downloads**, each verified by the engine and an independent SHA-256 of the resulting file. Total elapsed time includes final assembly and hashing.

| Fixture | 64 KiB median | 16 KiB median | Reduction in median elapsed time |
|---|---:|---:|---:|
| Delayed response headers | 2.668 s | 2.594 s | 2.8% |
| Steady paced body | 2.927 s | 2.663 s | 9.0% |
| One slow initial connection | 3.304 s | 3.265 s | 1.2% |

The steady workload favored 16 KiB in all three pairs. The other workloads had mixed pairwise results; the slow-connection profile also had a baseline outlier. Three short pairs are exploratory, not a robust confidence interval. This is local HTTP, not Microsoft's HTTPS CDN, and the console harness does not exercise the main GUI's periodic tick. The result supports retaining 16 KiB as an experimental candidate, not claiming a universal 9% gain or deploying it as the solution to the ISO gap.

Next causal tests should measure time between read completion and the next read submission, manager-mutex wait time, and TCP retransmissions/window behavior. Then compare candidate versus current UDM repeatedly on the same Microsoft endpoint in alternating order. Match headers and startup path as separate controlled variables. Replacing the entire transport or adding a kernel driver is not justified by the evidence collected here.

## Trace quality and interpretation

- IDM's diagnostic resume lasted 60.009 seconds. It was resumed from an existing partial download; its part-file writes include existing-data rearrangement, so disk bytes are not a valid network-throughput counter.
- The original UDM observer encountered a Windows sharing error while replacing its JSON snapshot. It captured approximately 11 seconds of UDM traffic; the application's download continued and was later paused. This is an observer failure, not a download-engine failure.
- The repaired observer uses append-only snapshots and independent cleanup. A separate UDM repeat captured a complete 44.367-second resume/pause interval and detached successfully. It confirmed nine requests and the same read behavior. Its 2,512 saved data writes were exactly 65,536 bytes each; some additional read completions occurred during cancellation without being committed to disk.
- Trace runtimes also include idle time before resume and after pause. Histograms use power-of-two upper bounds, not exact sizes unless otherwise established from byte counts/source.
- Frida adds overhead, with very different callback counts in the two applications. These diagnostic transfers were not pinned to the previous benchmark endpoint and must not be compared as a new speed race.
- The original uninstrumented same-endpoint benchmark remains [documented separately](../benchmarks/iso-same-endpoint-20260923/comparison.md). It found 159.51 versus 136.31 MiB after approximately 120 seconds, one pair only. Network variation, order and UI entry path remain uncontrolled.
- IDM displayed an expired-trial prompt after its diagnostic run. No later IDM run or license-state change was attempted.

## Verification and retained artifacts

Both ISO jobs are paused. IDM now has 416,438,277 saved bytes; UDM has 571,867,136. These include additional diagnostic downloads and do not replace the earlier timed benchmark totals.

All 571,867,136 bytes in UDM's eight retained parts match SHA-256 of the corresponding ranges in the existing local Microsoft ISO. This checks partial payload correctness, not completion or a fresh signature verification of that reference.

The observer has detached and its process has exited. Both production executable hashes are unchanged. The hosts file still matches its original SHA-256 `2d6bdfb341be3a6234b24742377f93aa7c7cfb0d9fd64efa9282c87852e57085`.

Evidence directory: `D:\UDM\benchmarks\transport-analysis-20260924`.

- [Machine-readable findings and partial hashes](../benchmarks/transport-analysis-20260924/findings.json)
- [Tool versions and PE inspection](../benchmarks/transport-analysis-20260924/static-tool-report.json)
- [Artifact hashes](../benchmarks/transport-analysis-20260924/tool-manifest.json)
- [IDM trace](../benchmarks/transport-analysis-20260924/idm-trace.json)
- [Corrected UDM trace](../benchmarks/transport-analysis-20260924/udm-repeat/udm-final.json)
- [All 18 experiment results](../benchmarks/transport-analysis-20260924/read-size-experiment/results/results.json)
- [Reproducible experiment build](../benchmarks/transport-analysis-20260924/read-size-experiment/build-experiment.ps1)
- [Repaired observer](../benchmarks/transport-analysis-20260924/trace-v2.py)

The observer records selected request metadata and aggregate API sizes/timing. It does not save response payloads, cookies, authorization headers or signed URL query strings.
