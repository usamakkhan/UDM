# UDM 0.10.1 stress-testing report

Recorded September 20, 2026 on this Windows development PC. Testing found and fixed two product defects: stalled HTTP requests could delay Pause, and native menu commands were disabled despite working toolbar buttons. Final automated checks passed in the coverage below. This remains a development build, not a claim of complete IDM parity.

| Coverage | Final result |
|---|---:|
| Native C++ engine, persistence, media and pipe regressions | 95 passed |
| Browser capture and format/identity logic | 61 passed |
| Engine stress groups | 24 passed |
| Protocol and damaged-storage groups | 11 passed |
| Real native host framing / desktop pipe | Passed |
| Desktop download and Pause/Resume, independently hashed | Passed |
| Optional driver C static analysis and INF validation | Passed; not loaded |

Counts describe assertions or scenario groups in their respective suites, not an equal number of independent application features. The 24 engine groups include 40 size/worker combinations, 12 queued files, 240 sequential files, and other transfers. The resource group was rerun separately after correcting its deadline; the full run's original failure is retained. [Machine-readable results](reference/stress-0.10.1.json) and [native evidence](reference/native-test-evidence-0.10.1.json) accompany this report.

## Fixes

**Pause on a stalled server.** The old synchronous WinHTTP call prevented workers from seeing cancellation while waiting for response headers or bytes. Both fault-injection cases exceeded the six-second test deadline before the fix. UDM now uses asynchronous WinHTTP beneath its worker interface; the owning worker cancels safely after the API returns and preserves callback context and buffers until handle closure. In the final engine run, stalled body cancellation took 3.7 ms and stalled header cancellation took 61.4 ms after the cancellation signal. These are local measurements, not universal latency guarantees. POST and 16 simultaneous stalled-transfer cases also passed. The design follows Microsoft's [WinHTTP concurrency](https://learn.microsoft.com/en-us/windows/win32/winhttp/concurrency-in-winhttp) and [handle lifetime](https://learn.microsoft.com/en-us/windows/win32/api/winhttp/nf-winhttp-winhttpclosehandle) guidance.

**Disabled menus.** Desktop testing found MFC disabled custom menu items because the app handled commands in OnCommand without registering update routing. The new selection-aware update handler makes applicable commands available and locks shared download state while deciding availability. Toolbar behavior remains covered by actual Start, Pause and Resume interaction.

Packaging now copies test sources instead of including locally compiled stress executables, object files and PDBs.

## Stress evidence

- Successfully downloaded and SHA-256 verified **4,295,032,833 bytes**, crossing the 4 GiB boundary, twice. The final repeat used **18,075,648 bytes peak working set** and took 170.45 seconds while other local validation was running. Expected hash: `f7c30baf71418e6eccb49c9296ce7f7317a9df3d988c8ca9c0ae473b8ae6329d`.
- **240 sequential downloads** in one process, with 65-second cleanup periods after each 120-file batch. Both settled samples were **237 handles**; final count was 236, versus 175 before initializing HTTP. The process completed in 223.06 seconds.
- Actual process termination during a 64 MiB download, then a new process resumed and produced the correct file. Five pause/reload cycles also completed correctly.
- Four queued downloads active at once across twelve files; global bandwidth cap checked across concurrent files.
- Connection drops, HTTP 503 retries, ETag/resource changes, missing validators, weak validators, Last-Modified validators, no-range servers, chunked bodies, malformed ranges, unexpected encodings and redirect loops.
- Missing, overlapping and oversized partial files recovered correctly. Deliberately altered bytes failed the expected SHA-256 check without publishing a final file. Malformed JSON and duplicate part IDs were rejected without overwriting the damaged state.
- One MiB POST echo, cancelled POST, rejection of an untrusted local HTTPS certificate, and a blocked destination path. Certificate trust was not changed.
- Real desktop transfer: 33,555,163 bytes. Pause at 16,253,112 bytes, Resume from retained progress, completion dialog and independent final hash matched: `13e38b5c3bd581857b66e94ac82b2c5c002bc9279f2f2cc8cc126f97a177d7f8`. Options and Scheduler dialogs were inspected.

## Test corrections, retained openly

A five-second resource check initially looked like a handle leak. Longer diagnosis showed release near one minute, including after confirmed session unload. The two-batch test above replaces that invalid leak inference; no unsupported resource workaround was added to production. Its first 180-second deadline ended during cleanup after all 240 files had completed, so the deadline became 300 seconds.

Time-based setup for partial-file corruption tests sometimes paused before receiving bytes while the PC was busy. Setup now waits for 128 KiB of actual progress. All eleven protocol/storage cases passed after that correction. Original failed runs remain under `benchmarks/stress-2026-09-20`; they were not erased or relabeled.

## Reproduce and limits

See [stress harness instructions](../tests/stress/README.md). Tests use isolated history and loopback fixtures. Download payloads, private state, compiler caches and generated certificate keys are excluded from the portable archive.

The measured rates are local fixture rates, not evidence that UDM beats IDM or Microsoft CDN performance. This work does not establish live YouTube capture compatibility, every video platform, live FTP, all proxy/authentication variants, all-day reliability, disk-full handling or power-loss recovery. A crash between final-file publication and saving its completed state still needs reconciliation work. Static driver checks do not validate a loaded kernel driver; no driver was installed, no Test Mode was enabled, and no restart was needed.
Final UI limitation: after the successful desktop Pause/Resume test, Windows input failed with access denied (0x80070005). The menu fix compiled successfully, but final interactive menu verification remains outstanding. Both completed test transfers and all ten existing user records were preserved; no user transfer was active during the update.
