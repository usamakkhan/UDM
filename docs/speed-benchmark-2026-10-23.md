# Transfer and catalog hot-path changes (2026-10-23)

Follow-up to [speed-benchmark-2026-10-18.md](speed-benchmark-2026-10-18.md),
which left three gaps after the single-file layout: the outage case recovered
3 s later than IDM, the uncapped eight-connection transfer phase was 1.3 s
behind, and the sequential path was still slower than IDM's single stream.
This pass removes work from the per-read path and makes outage recovery
immediate. It also stops the periodic catalog write from blocking the workers.

## Changes

1. **Disk writes leave the catalog lock** (`native/Transfer.cpp`). Each
   worker computed its write offset, wrote the block to the output file and
   credited the catalog all under `Manager::mutex`, so eight connections
   serialized their disk writes behind one another and the window's 500 ms
   refresh waited on them (a write to a saturated page cache can stall for tens
   of milliseconds). The offset and the byte count are now taken under the lock,
   the write runs without it, and the credit is taken under the lock again. A
   split never lands inside a worker's in-flight bytes (`inflightEnd`, kept
   under the lock), so no two workers ever own the same byte and the credit
   after the write is always what was computed before it.
2. **Outage probing.** After a download was receiving, a connection that fails
   without a byte is treated as a network outage for 20 s: the worker reconnects
   every 500 ms without spending its retry budget instead of waiting 1, 2, 4 s
   and counting each attempt. The budget still applies to a server that was
   never reachable, and a connection refused while other workers are receiving
   still hands its range to them. IDM reconnects continuously in the same
   situation (1,974 attempts in a 5 s blackout in the earlier runs; UDM made
   24).
3. **1 MiB reads for a single connection.** A sequential (no-Range) download
   reads in 1 MiB units instead of 256 KiB, a quarter of the WinHTTP round
   trips and lock acquisitions per megabyte. Parallel ranges keep 256 KiB.
   Under a speed limit every read is capped at a quarter second of the limit
   (64 KiB at least): the limiter charges after each read, so a large read at a
   low limit had stalled the connection for seconds (8 s for 1 MiB at
   128 KiB/s), which a server can take for an abandoned connection, and it made
   the limited rate bursty. `Manager::effectiveLimitKbps` exposes the limit the
   download is charged against.
4. **The periodic catalog save no longer holds the manager lock while it
   writes** (`native/Core.cpp`). `Manager::save` serializes the catalog under
   the lock, then writes and flushes `state.json` under a separate write mutex,
   ordered by a sequence number so a snapshot older than one already on disk
   is skipped. `Manager::tick` releases the manager lock before saving. A caller
   that already holds the lock (every explicit save) behaves as before. During
   a download the catalog changed every tick, so every 1.5 s all workers and the
   window had waited on a file write plus `FlushFileBuffers`.

## Checks

`Udm.NativeTests.exe --transfer-checks` (new selector) runs the transfer group
alone. `TransferFixture` gained an outage mode (`outageAt`, `outageMs`): once
that many body bytes were served it resets every open connection and refuses
new ones for the blackout, and records the delay from the end of the blackout
to the first accepted connection. The new check downloads 32 MiB over four
connections through a 1.5 s outage at 8 MiB and requires the exact hash plus a
reconnection within one second of the blackout ending; it measured 94 ms
(before the change the first attempt after a blackout waited 1 s, the second
2 s, and the 1.5 s blackout ended during the second wait).

## Measurements

The harness runs here were taken while other work was using the machine
(another build and other applications at 40 to 100 % CPU, and the system
drive was within a few gigabytes of full), and the loopback origin is a Python
process, so single runs swing by a factor of three; the
rows below alternate the unchanged and the changed binary, two rounds each,
and only the outage case is outside that noise. Columns: wall s, transfer s,
finish s, requests, rejected, wasted MiB.

| case | binary | wall s | transfer s | finish s | requests | rejected | wasted MiB | notes |
|---|---|---:|---:|---:|---:|---:|---:|---|
| no-range-1 | baseline round 1 | 2.6 | 1.3 | 0.6 | 2 | 0 | 0.2 | segments=1 splits=0 |
| no-range-1 | baseline round 2 | 8.1 | 1.3 | 5.7 | 2 | 0 | 0.5 | segments=1 splits=0 |
| no-range-1 | changed round 1 | 3.4 | 1.1 | 1.6 | 2 | 0 | 0.2 | segments=1 splits=0 |
| no-range-1 | changed round 2 | 14.9 | 13.8 | 0.3 | 2 | 0 | 0.2 | segments=1 splits=0 |
| outage-8 | baseline round 1 | 21.6 | 20.6 | 0.1 | 62 | 24 | 2.3 | segments=53 splits=5 |
| outage-8 | baseline round 2 | 19.2 | 18.5 | 0.1 | 63 | 24 | 4.2 | segments=54 splits=6 |
| outage-8 | changed round 1 | 14.6 | 13.1 | 0.2 | 67 | 57 | 3.6 | segments=58 splits=10 |
| outage-8 | changed round 2 | 16.3 | 15.3 | 0.1 | 71 | 54 | 6.3 | segments=62 splits=14 |
| uncapped-8 | baseline round 1 | 12.1 | 11.0 | 0.4 | 56 | 0 | 4.1 | segments=55 splits=7 |
| uncapped-8 | baseline round 2 | 4.1 | 3.0 | 0.1 | 55 | 0 | 4.1 | segments=54 splits=6 |
| uncapped-8 | changed round 1 | 8.1 | 7.1 | 0.2 | 58 | 0 | 5.4 | segments=57 splits=9 |
| uncapped-8 | changed round 2 | 4.3 | 3.3 | 0.2 | 58 | 0 | 5.5 | segments=57 splits=9 |

- **outage-8**: 13.1 and 15.3 s against 18.5 and 20.6 s. The changed binary made
  54 to 57 refused attempts during the 5 s blackout (two per second per worker)
  against 24, and resumed as soon as the origin accepted connections. The
  loaded machine still puts both well above the 11.3 s ideal (6.3 s transfer
  plus the 5 s blackout); the 10-18 run of the unchanged code on an idle
  machine was 15.7 s.
- **uncapped-8** and **no-range-1** are within the noise of these runs (the
  1.3 s and 1.1 s sequential transfer phases are the two quiet runs; the 13.8 s
  one coincided with a build). Rerun `--cases uncapped-8,no-range-1,single-8`
  on an idle machine before quoting a number for these two changes.

Raw results: `benchmarks/speed/results/2026-10-23-ab-{base,new}-{1,2}`. The
first passes (taken while two builds were compiling, with wall times up to
three times these) were discarded.

## Reproducing

```powershell
.\native\build.ps1 -AppOnly -OutputRoot <folder>
python benchmarks\speed\bench.py --udm <folder>\release-native\UDM.exe --work $env:TEMP\udm-speed-bench --out benchmarks\speed\results\<label> --clients udm --no-idm-registry --cases uncapped-8,no-range-1,outage-8,single-8
python benchmarks\speed\report.py benchmarks\speed\results\<label>
```
