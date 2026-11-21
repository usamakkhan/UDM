# Finishing latency investigation, 8.18â€“8.20

The acceptance target is UDM finishing no later than IDM in the three previously slower cases: 4 MiB sequential, 64 MiB sequential and 64 MiB split. Correctness and installation alone do not meet this target. Earlier failures remain in the linked validation records.

## Measurement and its limits

Use installed executable hashes, three alternating cold-launch paired trials, the same deterministic in-memory loopback origin, 8,000,000 bytes/s per connection, 20 ms response latency and aligned configured connection limits (1 or 8). Temporarily changed IDM registry values and types must be restored after each run and in the outer finally. The other Windows session is not stopped. UDM uses an isolated initially empty catalog; IDM uses its existing profile. Actual split request schedules differ.

The timer starts at launch and finishing tail is last origin body send to the first observed expected output length plus successful exclusive read-open, sampled every 2 ms. SHA-256 is checked independently afterward. This observation can catch a transient gap before post-publication work reopens the file. It must not be described as dialog visibility, stable final availability, crash durability or Internet performance. Track finish phases separately; do not improve this score by dropping verification, recovery, security metadata or final flushes.

## Observed failures

| Version | 4 MiB UDM / IDM median ms | 64 MiB sequential | 64 MiB split |
| --- | --- | --- | --- |
| 8.18 | 79.479 / 20.956 | 232.791 / 28.532 | 359.581 / 116.088 |
| 8.19 | 65.969 / 21.913 | 97.507 / 30.213 | 710.981 / 139.676 |
| 8.20 | 41.948 / 24.515 | 39.362 / 19.041 | 51.836 / 98.307 |

All 54 outputs match their expected hashes. Required publication intent and catalog writes, final data flush, sparse normalization and advisory hashing account for remaining latency and outliers. These are observations, not a claim that any single phase is always dominant. The 8.19 split writeback experiment did not improve its median and is narrowed in 8.20.

## 8.20 changes

1. Reserve regular files for known-size parallel HTTP layouts up to 64 MiB on fixed volumes during preparation. Pay allocation while transfer is active and avoid post-EOF sparse conversion for fresh files. Keep larger, unknown-size and network layouts sparse-aware. Existing sparse files retain their actual sparse state. Credited segment Done, not preallocated length, continues to bound resumable bytes.
2. Query actual finished length before requesting a write handle. Only a needed correction reopens a writer. PublicationPreparation still checks and pins source identity and size; metadata queries alone never prove publication ownership.
3. Restrict opportunistic writeback to sequential transfers. Parallel workers no longer join a competing whole-file flush at EOF. Required final writeback remains.

Native build, tests, installation and repeated installed measurements are separate records. See [qualification](validation/finishing-8.20.json) when available. No universal zero-delay or GUI parity claim is made.

## Next architectural work if the target still fails

Instrument ordinary publication intent, rename, catalog snapshot/write/ack and journal cleanup separately, without adding synchronous logging to timed paths. Determine whether a generation-bound intent can be prepared during transfer while preserving cancellation, destination changes and exact source identity. No rename may precede a durable matching intent; no accepted completion may rely only on a runtime ID. Keep existing crash/recovery and superseding-snapshot guarantees.

Evaluate advisory hash overlap against publication only with explicit ownership of its thread, immutable credited ranges, share modes compatible with rename and a join before digest access or source cleanup. Do not let post-publication hashing block user access unnecessarily. Required server/user/replacement hashes stay before publication. Measure stable file availability and actual completion-dialog visibility separately when desktop control works again.

## 8.20 outcome

Installed and verified all seven changed payloads; 1,742 checks passed. Both personal catalogs were preserved. The two sequential median gaps remain 17.433 and 20.321 ms. The split first-exclusive observation improved, but it catches a gap before later flush/hash operations in trials 1 and 3. Trial 2 stalls both apps: UDM total 11.879 s and IDM 17.532 s. No trial was excluded and the cause of the variance is not isolated. The target remains unmet, including worst-case split behavior. [Complete results](validation/finishing-8.20/comparison.json).

All 18 comparison payloads and 677 generated objects/superseded fixtures/owned test payloads were individually checked and removed. Current binaries, core cache, shared tools, historical fixtures, catalogs/journals, rollback copies and result records remain. No installer, commit, push or release was produced.
