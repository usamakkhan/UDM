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
3. **Reads sized to the speed limit.** Under a speed limit every read is capped
   at a quarter second of the limit (64 KiB at least): the limiter charges after
   each read, so a large read at a low limit stalled the connection for seconds,
   which a server can take for an abandoned connection, and it made the limited
   rate bursty. `Manager::effectiveLimitKbps` exposes the limit the download is
   charged against. The first version of this pass also read 1 MiB at a time on
   a single connection; a per-read trace showed that to be a regression (below)
   and it was reverted the same day, so reads stay at 256 KiB.
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

Two further rounds of the committed binary alone (same loaded machine; the
single-8 and no-range-1 swings show the noise):

| case | binary | wall s | transfer s | finish s | requests | rejected | wasted MiB | notes |
|---|---|---:|---:|---:|---:|---:|---:|---|
| single-8 | final round 1 | 9.0 | 7.6 | 0.2 | 51 | 0 | 0.7 | segments=50 splits=2 |
| single-8 | final round 2 | 13.7 | 12.7 | 0.2 | 64 | 0 | 7.5 | segments=63 splits=15 |
| uncapped-8 | final round 1 | 5.0 | 4.1 | 0.1 | 63 | 0 | 9.0 | segments=62 splits=14 |
| uncapped-8 | final round 2 | 5.4 | 4.4 | 0.1 | 63 | 0 | 9.7 | segments=62 splits=14 |
| outage-8 | final round 1 | 26.9 | 26.1 | 0.1 | 78 | 25 | 8.6 | segments=69 splits=21 |
| outage-8 | final round 2 | 15.9 | 15.0 | 0.1 | 76 | 53 | 10.5 | segments=68 splits=20 |
| no-range-1 | final round 1 | 6.9 | 5.8 | 0.4 | 2 | 0 | 0.5 | segments=1 splits=0 |
| no-range-1 | final round 2 | 6.2 | 1.5 | 3.9 | 2 | 0 | 0.2 | segments=1 splits=0 |

The first outage round made only 25 refused attempts and took 26 s: the
workers that failed first saw the others as still healthy (their reads had
not failed yet), handed their range back and slept the 1 s then 2 s retry
delay before probing. While a download is in the probing window the first
three such releases now pause one second each; a server that refuses extra
connections still gets the full backoff after them.

- **outage-8** (A/B rounds): 13.1 and 15.3 s against 18.5 and 20.6 s. The changed binary made
  54 to 57 refused attempts during the 5 s blackout (two per second per worker)
  against 24, and resumed as soon as the origin accepted connections. The
  loaded machine still puts both well above the 11.3 s ideal (6.3 s transfer
  plus the 5 s blackout); the 10-18 run of the unchanged code on an idle
  machine was 15.7 s.
## Same-day A/B on an idle machine (afternoon)

With the CPU at 7 to 40 % (the user's own applications only) and the work
folder on D:, the pre-change commit 3b65492 built in a separate worktree and
the committed binary were run back to back. The capped case sits at its ideal
(single-8: 6.9 and 7.5 s transfer against the 6.3 s ideal) and the outage case
reconnected after 57 to 58 refused attempts in each round. The uncapped
eight-connection case was 8 to 10 s for both binaries (2.5 s on 10-18): that
is this machine today, not the code. The sequential case exposed a regression:

| binary | no-range-1 transfer s | per 1 MiB (trace) |
|---|---:|---|
| pre-change 3b65492 | 1.1, 1.3 | |
| committed, 1 MiB reads | 6.1 (and 4.9 to 12.8 in earlier rounds) | read 11 ms, write 8.5 ms |
| committed, 256 KiB reads | 1.1, 1.6 | read 1.3 ms, write 0.5 ms |

A 1 MiB `WinHttpReadData` waits for the whole buffer before completing and a
1 MiB cached write of a cold buffer is far slower per byte than four 256 KiB
ones, so the larger unit cost five times the transfer time. Reverted; the
change list above reflects the final code. Raw results:
`benchmarks/speed/results/2026-10-23-idle-{1,2}`, `-idle-d-{1,2}`,
`-ab2-base-{1,2}`, `-ab2-new-1` (1 MiB) and `-ab2-new256-{1,2}`.

Two more rounds of the final binary alone (`-clean-{1,2}`), still with the
user's applications at 40 to 80 % CPU: single-8 11.0 and 13.5 s transfer (6.9
and 7.5 s in the quieter morning rounds, 6.3 s ideal), uncapped-8 2.5 and 8.4 s,
no-range-1 1.4 and 1.2 s transfer with a 2 to 3 s hash tail on the loaded CPU,
outage-8 15.7 and 16.6 s with 56 to 57 refused probes. Every outage round of
the final code today was between 15.0 and 17.3 s against 18.5 to 20.6 s for
the unchanged code in the morning A/B; the other cases need a machine that is
actually idle, which this one was not at any point today.

A late back-to-back run of the two binaries on the multi-connection cases
(`-quiet-{1,2,3}` final, `-quiet-base-{1,2}` pre-change, CPU 20 to 100 %
from other applications throughout):

| case | pre-change 3b65492 transfer s | final transfer s |
|---|---|---|
| single-8 | 11.6, 13.2 | 11.2, 11.8, 11.4 |
| uncapped-8 | 9.6, 9.5 | 9.5, 9.4, 8.4 |

Under the same conditions the two are indistinguishable: on loopback the
lock changes neither cost nor gain anything measurable for eight
connections, and the day's gain is confined to the outage case. The capped
case running at nearly twice its 6.3 s ideal for both binaries is the Python
origin starved of CPU, not the client.

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
