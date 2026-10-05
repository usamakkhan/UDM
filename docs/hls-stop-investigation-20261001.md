# Live HLS stop/save investigation — 1 October 2026

The earlier intermittent native stop/save failure did not reproduce. It remains unresolved. This change adds a focused reproducer and timing evidence; production downloader code and the installed application are unchanged.

| Run | Trials | Total completion time | Median |
|---|---:|---:|---:|
| Ordinary load | 16 | 453–687 ms | 508 ms |
| Two bounded SHA-256 workers on four logical processors | 16 | 469–1,390 ms | 633 ms |

All trials reached a verified stalled-header or stalled-body state, then saved successfully under the unchanged 1,500 ms assertion. The slowest loaded trial approached that bound. These measurements show sensitivity to workload in this fixture, but do not identify the cause of the historical failure. The recorded merge/probe/hash/publication interval reached 910 ms in the loaded run.

The existing live-HLS suite also passed 132 checks, including rolling recording, separate tracks, pause/restart, source expiry, interrupted publication, timestamp changes and output decoding. Repetition is not additional feature coverage.

Added --live-hls-stop-checks runs only the targeted cases, with unique output folders. LiveHlsChecks.hpp emits stop-request, merge and total durations on every trial. No production timeout or test bound was relaxed. Previous source files are backed up under backups/hls-stop-instrumentation-20261001.

The separate actual-browser audio-only timeout has not been reproduced by these native tests and remains open. Next investigation should use its actual Edge fixture and preserve player, preparation, handoff and native acceptance timing.

[Reproduction](../tests/live-hls-stop/README.md) · [Raw measurements](evidence-hls-stop-20261001/summary.json)

No release version bump, installed binary, browser, certificate, driver or personal download-catalog change was made.
