# Reception prefix hashing: 8.68 / browser 0.72.32

UDM 8.68 is installed. It hashes safe cached write prefixes during reception and continues that same hash after the writers join, removing a measurable post-flush hash wait. In a matched four-way D: split comparison, median finishing improves by **56.9 ms** against 8.67 while median total time improves by **109.6 ms**. The actual installed executable then wins all nine paired IDM cases at each measured completion/elapsed endpoint. These observations exceed the requested 17 ms median reduction without a measured median total-time regression in this run; they are not a universal timing guarantee.

## Evidence that selected the change

The retained 8.67/8.66 development comparison had almost identical median final file-flush durations, around 158 ms, but different remaining advisory-hash waits: roughly 38 versus 22 ms. The difference was approximately the reported 17 ms finishing gap. An entire 64 MiB SHA-256 pass could outlast the flush even though the complete written body was already retained in the bounded cache.

This increment targets that remaining hash work. It does not change writeback thresholds, introduce pacing, disable the final durability barrier or move completion reporting ahead of unmeasured work.

## Safe early access and joined lifetime

`WrittenBodyCache::copyWrittenPrefix` returns at most **1 MiB** of independently owned bytes from successful contiguous writes. It requires the same Manager lock as cache writes/resets and refuses gaps, invalid storage and invalid cursors. The hash worker reserves its bounded buffer before taking that lock and performs CNG hashing after releasing it. No second full-body allocation or file reread is introduced for this path.

A cache revision changes on reset, disable or overlap with previously covered bytes. The worker checks revisions between copies; after all writers and the hash worker join, the caller checks the final revision again. A rewrite after the last snapshot therefore also invalidates the partial digest. Invalidity or optional allocation/hash failure retains conservative full-hash fallback. The complete latest cache remains available after a rewrite for that fallback; the early consumer never keeps an alias to mutable body storage.

The existing stop/join machinery owns the new work. A cancel stops reception hashing, and the join completes before the cache or hash state can be destroyed or reused. `ReceptionHash` records source, valid hashed bytes and actual join duration. The existing advisory continuation consumes the remaining bytes using the joined hash state; required digest/publication checks, generation-bound disk credit and the final full file flush remain in place. The private ordered-writer path remains gated.

## Build and correctness

All five production outputs carry verified **8.68** resources/hashes. Both browser manifests advance to **0.72.32**. Bridge and normal Transfer rebuild; unchanged qualified core objects are preserved. **1,580 assertions pass across eighteen C:/D: suite runs.** Thirteen additional cache assertions per drive cover gap exclusion, contiguous copies, a SHA-256 known answer, rewrite invalidation, independent snapshot ownership, latest-body fallback, disabled storage, bounded reads and cursor boundaries. The existing concurrency/budget, writeback, checkpoint and recovery assertions also pass.

Real 32 MiB HTTP transfers on C: and D: pause after approximately 12 MiB, join, exit normally, reload their saved catalogs in fresh processes and resume to the independently expected complete hashes. [Build identities](validation/reception-hash-8.68/build.json), [all assertions](validation/reception-hash-8.68/checks.json), [actual HTTP recovery](validation/reception-hash-8.68/http-recovery.json).

## Matched development comparison

Four D: 64 MiB split trials rotate 8.68, frozen normal 8.67, frozen normal 8.66 and installed IDM through every order position. Each uses the same synthetic origin seed, 20 ms response delay, 8 MB/s per-connection rate and eight configured connections. Real request scheduling and unrelated machine/storage activity can differ. All sixteen files match expected length/hash and all apps exit normally. Original observations and losses remain in the report.

| Median | UDM 8.68 | UDM 8.67 | UDM 8.66 | IDM |
| --- | ---: | ---: | ---: | ---: |
| Last origin body send to actual exit | 382.5 ms | 439.4 ms | 407.4 ms | 445.8 ms |
| Last body send through post-exit flush | 418.3 ms | 476.4 ms | 433.4 ms | 1,122.3 ms |
| Full elapsed through actual exit | 2.700 s | 2.810 s | 2.932 s | 3.879 s |
| Full elapsed through post-exit flush | 2.739 s | 2.842 s | 2.954 s | 4.603 s |

Every 8.68 trial records valid reception hashing: **18,943,600–45,546,676 bytes** are hashed before the joined transfer handoff, with measured hash joins of 0.8–2.5 ms. Median hash work left after the file flush falls from **44.58 ms to 0.042 ms**. These phases support removal of the identified hash wait; they do not attribute every millisecond of the total-time difference to that change.

One development trial finishes **3.35 ms behind IDM**, and individual comparisons against 8.67 also include losses. Neither those samples nor the larger gains are discarded. Medians improve against both frozen UDM versions at all four endpoints. [Every development observation and paired result](validation/reception-hash-8.68/development-comparison.json), [explicit installation decision](validation/reception-hash-8.68/development-decision.json).

## Installation and installed IDM comparison

The approved transactional update replaces five native binaries and both manifests, verifies all seven hashes and updates product registration to 8.68. Both personal catalogs remain byte-for-byte unchanged. All 94 installed extension payloads match source. The manifests are installed; a live browser-extension reload/capture test is outside this CLI qualification. [Installation record](validation/reception-hash-8.68/installed.json).

The installed executable completes three alternating paired IDM trials for each case below. Measurement includes actual application-process exit, joined application background work and `/q` shutdown. A symmetric exclusive completed-output open/flush/close supplies the separate durable endpoint. It does not measure GUI animation, prove hardware power-loss resilience or attribute unrelated shared OS/service work exclusively to either app. IDM's two connection settings are restored after each group.

| Installed median | UDM exit tail | IDM exit tail | UDM durable tail | IDM durable tail | UDM total | IDM total |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 4 MiB sequential | 238.4 ms | 501.9 ms | 290.5 ms | 530.7 ms | 1.273 s | 2.262 s |
| 64 MiB sequential | 273.6 ms | 422.2 ms | 309.0 ms | 1,070.4 ms | 9.122 s | 10.177 s |
| 64 MiB split | 333.1 ms | 447.2 ms | 373.1 ms | 1,111.1 ms | 2.464 s | 3.838 s |

UDM wins **all nine installed pairs at all four endpoints**, including full durable elapsed. Full durable medians are respectively 1.318 / 2.356 s, 9.158 / 10.838 s and 2.504 / 4.524 s for UDM / IDM. Every file matches its independent expected hash and length and exits normally. [All installed observations](validation/reception-hash-8.68/installed-comparison.json).

## Cleanup and delivery boundaries

All thirty-four development/installed comparison payloads were deleted after verification: **1,904,214,016 logical bytes**. Both HTTP recovery payloads, eighteen suite roots and generated comparison metadata were removed: **73,380,665 logical tree bytes**, including the two 32 MiB recovery files. Twenty temporary test executable/object files were removed: **66,876,767 bytes**. The temporary four-client source copy was deleted; the existing small reproduction patch is reused rather than duplicated. [Cleanup](validation/reception-hash-8.68/cleanup.json), [harness-copy cleanup](validation/reception-hash-8.68/harness-copy-cleanup.json).

These are logical byte counts, not measured physical space or drive-wear savings. Installed/development products, required cores, compact evidence and the verified installation rollback backup remain. No setup package, release, commit or push was created. The supported outcome is this installed improvement and the recorded comparisons; universal inside/out IDM parity remains unproven.
