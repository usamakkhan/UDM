# Live Stop and save: phase measurements

Compile-time-only tracing now splits finalization into recorder sealing, catalog checkpoint, saved-segment verification, assembly, probing, hashing, publication and cleanup. Normal builds omit the trace output; the normal binary and run log were checked independently.

| Run | Trials | Minimum | Median | Maximum |
|---|---:|---:|---:|---:|
| ordinary | 16 | 265 ms | 320.5 ms | 1079 ms |
| loaded | 16 | 313 ms | 383.0 ms | 594 ms |

All 32 trials stayed within the existing 1500 ms bound, and all 32 files were independently fully decoded. The slowest trial took 1079 ms: 1000 ms was measured in finalization, including 562 ms in assembly, 203 ms in probing and 157 ms in publication. The remaining 79 ms includes stop-request saving, cancellation, scheduling and diagnostic overhead; it cannot be attributed entirely to network cancellation. Timer resolution is coarse, so a reported zero phase is not zero work.

The normal build passed **132 live-HLS checks** with tracing disabled. Installed binaries, both personal catalogs, all 132 accepted installer inputs and the installer itself are unchanged. The compiler flag is `UDM_LIVE_TIMING_TRACE`; its isolated build and runner are retained in the candidate control folder.

The earlier 2672/2188 ms full-suite failures remain unresolved. These tests did not recreate the concurrent compilation/memory conditions of that run, and do not prove an IDM speed advantage or fix. Next investigation should reproduce those conditions while separating media-helper process startup from execution and recording system I/O pressure.

[Measurements, hashes and limits](D:/UDM-Workspace/candidates/release-084-055/control/live-phase-acceptance.json) · [Reproducible runner](D:/UDM-Workspace/candidates/release-084-055/control/run-live-phase.py)
