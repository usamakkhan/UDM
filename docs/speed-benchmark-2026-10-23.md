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

## Head to head with IDM, afternoon: finish phase and stale connections

With the machine idle, UDM and the installed IDM 6.43 ran the seven loopback cases back
to back (`benchmarks/speed/results/2026-10-23-vs-idm-1`). IDM finished every case
within 0.5 s of its last byte; UDM needed 2.5 to 12.8 s, and on the dropping-connection
case its transfer took 13.5 s against IDM's 7.5. New timing fields on the record
(`FinishPhases`, `NetworkTicks`, `NetworkTailSeconds`, printed by the harness) pinned
each second:

| Cause | Measured | Change |
|---|---|---|
| The trailing hash thread kept hashing after the last byte, inside the network phase | 1 to 4 s (uncapped: workers joined at 5.6 s, network phase ended at 9.7 s) | The thread stops when every worker has returned |
| Clearing the sparse attribute on the finished file before publication | 2.3 s (256 MiB, SSD) to 12.4 s (384 MiB, SSD 95 % full); 5 s on the hard disk | Cleared after publication, with the flush |
| The remaining digest computed before publication | up to 5 s when the download outran SHA-256 (about 190 MB/s on this Haswell CPU) | When no expected checksum must be verified, the file is published first and its SHA-256 recorded a moment later (`HashMode` deferred); an `ExpectedSha256` still verifies before publication |
| A kept-alive connection the server had closed answered the next request with nothing, which counted as an outage probe (500 ms wait, 1 s release) | 1 s dips to two active connections after every drop; 35 empty requests | Like a browser, one immediate retry on a fresh connection when the previous request on that worker delivered bytes |
| 48 chunks of 8 MiB per 384 MiB file | 52 to 102 requests per file against IDM's 5 to 59 | Three chunks per connection, 4 to 32 MiB: 24 to 38 requests |

Final build, work folder on the SSD (`2026-10-23-ssd-vs-idm-3`):

| case | IDM wall s | UDM wall s | IDM finish s | UDM finish s | notes |
|---|---:|---:|---:|---:|---|
| single-8 | 8.9 | 7.6 | 0.6 | 0.1 | 8 connections capped at 8 MB/s; ideal 6.3 s transfer |
| uncapped-8 | 2.9 | 3.5 | 0.6 | 0.1 | loopback, no cap: the Python origin and UDM's read path both run near CPU limits |
| resume-8 | 8.5 | 5.1 | 0.3 | 0.1 | killed at 45 %, relaunched; IDM re-downloaded 204 MiB, UDM 44 MiB |
| outage-8 | 13.7 | 13.4 | 0.3 | 0.1 | 5 s blackout at 45 % |
| drop-8 | 17.9 | 9.7 | 3.6 | 0.2 | server closes every connection after 8 MiB |
| no-range-1 | 2.0 | 3.6 | 0.1 | 0.1 | single connection, no ranges: UDM reads 256 KiB at a time at about 100 MB/s |
| slow-start-2 | 20.7 | 19.3 | 0.2 | 0.2 | server allows two connections |

The two cases IDM still won were read-path ceilings rather than stalls: on a single
connection UDM alternated a 256 KiB WinHTTP read with the disk write (1.3 ms plus 0.5
ms per 256 KiB), and 1 MiB reads measured slower, not faster. The worker now issues
the next read before writing the previous buffer (`Http::beginRead` / `finishRead`,
two buffers owned by the worker so an outstanding read never outlives its memory).
With that (`2026-10-23-overlap-{1,2}`, SSD, against IDM in the same runs):

| case | IDM wall s | UDM wall s | UDM transfer s | notes |
|---|---:|---:|---:|---|
| no-range-1 | 3.4, 2.0 | 2.3, 1.4 | 1.3, 0.4 | 256 MiB on one connection; was 3.6 s |
| uncapped-8 | 12.6, 4.3 | 8.9, 4.0 | 7.8, 3.0 | both clients swing widely on this machine; IDM's first run spent 8.8 s finishing |
| single-8 | 11.0 | 8.1 | 7.1 | capped case, 24 requests after the chunk change |

The dropping-connection case moved from 14.5 s to 8.3 to 9.7 s; IDM's own run varied
between 8.9 and 17.9 s.

Destination disk: D: here is a 250 GB WD hard disk and C: the SSD. The morning runs
wrote to D:, where the final round of chunks ran four times slower than the earlier
rounds for UDM while IDM was unaffected; on the SSD the same case meets the ideal.
The hard disk interplay (write-back plus the trailing hash's reads) is left measured,
not fixed.

## Public origins against IDM (2026-10-24)

The two public cases (`public-alpine-8`: the Alpine 3.21.3 standard ISO from
dl-cdn.alpinelinux.org, `public-kernel-8`: linux-6.12.tar.xz from cdn.kernel.org, eight
connections, this link about 4 MB/s) ran head to head with IDM, two rounds per build
(`benchmarks/speed/results/2026-10-24-public-*`). `--udm-settings` on the harness
merges settings into UDM's private catalog, and the record now stores the negotiated
`Protocol`.

| build | Alpine IDM / UDM wall s | kernel IDM / UDM wall s | UDM splits (kernel) |
|---|---|---|---|
| morning build (HTTP/3 on for every download) | 68.4 / 142.8, 62.8 / 156.2 | 40.2 / 114.8, 46.1 / 117.4 | 25, 24 |
| same build, `{"UseHttp3": false}` | - / 64.1 | | |
| HTTP/3 only for single-connection downloads | 105.1 / 91.1, 91.3 / 90.0 | 42.9 / 73.6, 39.3 / 47.9 | 29, 36 |
| plus a larger minimum per half for speed-blind tail splits (a 64th of the file per connection, 256 KiB to 2 MiB) | 64.5 / 64.1, 61.9 / 61.3 | 39.7 / 40.8, 38.2 / 39.4 | 13, 31 |
| final binary (the minimum scaled with the file so small files still split), confirming round | 73.9 / 77.7 | 41.9 / 44.2 | 29 |

The first row was the real-network regression of the day: both CDNs speak HTTP/3, and
WinHTTP carried all eight ranges over one QUIC connection, which on a shared link gets a
single connection's share. The protocol is now left to single-connection transfers. The
third row's kernel runs still lost up to half their time to tail splitting: 36 splits at
a 256 KiB minimum on a 3 MB/s link, each a new TLS connection plus the owner's in-flight
bytes. With the scaled minimum (2 MiB per half for these files; a 3 MiB file keeps 256 KiB so it still spreads over its connections) both files are level with IDM within a second per run,
and the loopback cases hold (single-8 7.9 s with no splits, uncapped-8 2.7 s and 4.4 MiB
wasted against 14 MiB before, drop-8 9.5 s). The shared link swings by 50 % between rounds
(IDM's own Alpine runs span 61.9 to 105.1 s), so only same-round pairs are comparable.

## Hard-disk destinations (2026-10-24)

With the benchmark's work folder on D: (the WD hard disk) every round of chunks was slow,
not only the last: 16 MiB chunks took 3 to 6 s where the cap allows 2.1, and on the
uncapped case the first ten chunks vanished into the write cache in 0.1 s each, then
each write stalled 3 to 5 s (`2026-10-24-hdd-{1,2}`). Eight ranges written in place
make eight interleaved streams for the lazy writer to seek between. IDM is immune only
because its parts land in its temp folder on C: and the file is assembled once,
sequentially. UDM's default parts folder is its data folder, normally on the system
drive, so an ordinary install behaves the same way; the benchmark's `--data-dir` on D:
was the artifact. Pointing UDM's `TemporaryFolder` at the SSD (`--udm-settings`) gave
the ideal 6.9 s network phase on a D: destination, but publication then cost 3.7 to
7 s: `CopyFileW` copies large files unbuffered and waits on the disk, where IDM's
assembly is a buffered write the cache absorbs.

The cross-volume staging copy is now a buffered 1 MiB-block copy, flushed after
publication like the same-volume case. D: destination, UDM parts on the SSD, IDM
temp on C: (`2026-10-24-hdd-copy-{1,2}`):

| case | IDM wall s | UDM wall s | UDM publish s |
|---|---:|---:|---:|
| single-8 | 29.4, 16.1 | 10.5, 18.3 | 2.7, 2.8 |
| uncapped-8 | 10.0, 9.6 | 7.5, 5.6 | 2.6, 2.6 |
| no-range-1 | 4.7, 3.8 | 10.0, 3.9 | 2.1, 1.3 |

Both clients now pay about the same to land 384 MiB on the hard disk (IDM's finish
2.2 to 2.6 s). The swings between rounds (IDM's 19.6 s startup, UDM's 14 s network
phase) are the cache draining the previous file to the disk: Windows throttles every
writer while dirty pages drain, so a case run right after another suffers whichever
client it is. Parts on the same hard disk as the destination remain slow for UDM;
the Options > Save to temporary folder is the control for that case.

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
