# Owned publication file object: UDM 9.62

UDM 9.62 schedules asynchronous final body writeback while keeping one writer-excluding file object alive until exact completed-catalog acceptance. A later common-clock follow-up finds that a source query serializes intent preparation behind that flush, so the intended overlap is not demonstrated by the original strategy flag. A duplicated handle performs writeback; the publication owner adopts the original handle. It does not open an unprotected metadata snapshot or add another full-file read. This replaces the [rejected 9.61 rehash candidate](publication-overlap-9.61.md).

Both source manifests advance to 0.73.26. No extension capture behavior changes in this native increment. The installed app and extension remain unchanged. Five native products build successfully and 1,275 affected backend assertions pass. The no-regression and IDM first-availability performance gates remain open, so this build is not promoted for installation.

## Ownership and transaction

The existing prepared ordinary transfer eligibility is retained: a known positive body up to 64 MiB, fixed volume, no sparse normalization, copied-volume output, replacement, POST or FTP. All network writers are already joined. The file opens with write-only access and read/delete sharing; that object excludes other writers. A duplicated handle refers to the same object and sharing protection. Creating the worker or acquiring the pin may fail; the original serial path remains available.

The owned body task captures only its handle, path and opening duration. It flushes and closes its duplicate and returns actual operation/error values. The original pin remains alive; there is no reopen gap between final body writeback and publication source protection. Transfer unwinding joins the task before staging cleanup. Prepared publication owns it after a successful transfer handoff. There is no detached worker. Cancellation cannot abandon a live flush outside the Manager worker's measured lifetime.

Known-digest publication adopts that write-only pin and measures its size/identity. It writes an early intent with `FlushBeforePublish: true` and the existing exact before/after commit identity. Rename requires successful body writeback, unchanged source identity/name, directory checks, cancellation and unchanged destination inputs. The existing trusted transfer digest is retained; a write-only pin cannot silently supply a missing hash. Unknown hashes join and close the write pin first, then use the original strict preparation path.

The published-name read pin shares the owner's write access but denies deletion. Combined with the still-live write-only source object, this excludes foreign writers and renames before acceptance. Both pins close only after exact durable catalog proof, before journal cleanup, retaining the accepted-file availability improvement from 9.60. An error preserves parts and any durable recovery record. Recovery hashes and flushes early-intent bytes before completion; the prior recovery safeguards remain.

Redundant automatic catalog IO remains coalesced through the owned body barrier. Explicit saves and required receipt/catalog proofs are not suppressed. Scan, completion notification and part cleanup retain their acceptance gates.

## Honest timing

Body open includes the actual initial open/duplication cost. Worker flush and duplicate-close durations are recorded, along with the owner's join. Final release of the adopted object occurs after exact acceptance; duplicate close is not the end of that file object's lifetime. The first persisted PublicationPhases snapshot ends before the final catalog save and does not include final pin release or cleanup. The independent process-exit and post-exit-flush measurements include that later work. Overlapping regions must not be added as serial wall time.

The comparison uses normal uninstrumented products, frozen 9.60 and IDM, rotating all three positions for 4 MiB sequential, 64 MiB sequential and 64 MiB split. It measures exclusive output availability, actual application process exit, full elapsed time and a symmetric post-exit file flush. Final size, SHA-256 and UDM's saved Complete record must match. This is not an installed/browser or visible-dialog timing test, and does not prove full IDM parity.

## Qualification

The eleven suites pass 1,275 assertions: 105 owned publication/pin/failure checks, 145 actual HTTP completion checks, 31 independent finishing ownership checks and 994 existing preparation, commit, availability, ownership, recovery and launch checks. Early prechecks repeat the two new functional suites and are retained without double-counting. Both a write-handle conflict and a source-name replacement are exercised, along with failed/throwing flush, unknown-hash fallback, cancellation, changed destinations and interruption before/after rename. This batch tests exception interruption/recovery; it does not emulate power loss during a running flush.

All 27 normal-product downloads match independent size and SHA-256 checks; UDM's durable Complete records also match. Three trials rotate all three clients through each position. No measurement runs alongside the compiler. Median values:

| Case | Metric | 9.62 | 9.60 | IDM |
| --- | --- | ---: | ---: | ---: |
| 4 MiB sequential | Exclusive availability after last origin send | 107.130 ms | 109.492 ms | 27.194 ms |
| 4 MiB sequential | All app processes exited after last origin send | 153.530 ms | 156.363 ms | 374.291 ms |
| 4 MiB sequential | Full process elapsed | 1008.828 ms | 1010.689 ms | 2068.183 ms |
| 4 MiB sequential | Post-exit output durability after last origin send | 191.211 ms | 177.575 ms | 492.875 ms |
| 64 MiB sequential | Exclusive availability after last origin send | 147.601 ms | 141.783 ms | 77.888 ms |
| 64 MiB sequential | All app processes exited after last origin send | 193.030 ms | 191.414 ms | 365.995 ms |
| 64 MiB sequential | Full process elapsed | 8898.784 ms | 8903.396 ms | 9933.468 ms |
| 64 MiB sequential | Post-exit output durability after last origin send | 207.057 ms | 204.064 ms | 1036.686 ms |
| 64 MiB split | Exclusive availability after last origin send | 207.351 ms | 235.206 ms | 125.032 ms |
| 64 MiB split | All app processes exited after last origin send | 255.698 ms | 282.572 ms | 402.809 ms |
| 64 MiB split | Full process elapsed | 2406.188 ms | 2331.103 ms | 3394.047 ms |
| 64 MiB split | Post-exit output durability after last origin send | 282.771 ms | 299.966 ms | 1072.471 ms |

9.62 wins all nine comparisons with IDM for process-exit finishing, full elapsed, durable finishing and durable elapsed, but loses all nine exclusive first-availability pairs. Relative to 9.60, it wins six of nine availability and process-exit finishing pairs, four of nine full elapsed pairs, five of nine durable finishing pairs and three of nine durable elapsed pairs. The failed pairs remain in the evidence. The median split finish is about 26.9 ms faster, but median split full elapsed is about 75.1 ms slower. Median sequential 64 MiB finishing is about 1.6 ms slower. These results do not satisfy the user's requirement of no slower case and must not be described as complete parity.

The initial 64 MiB sequential observation is 241.125 ms versus 191.414 ms for 9.60; the second split observation is 322.416 ms versus 281.844 ms. They are not removed as outliers. The remaining variance and full elapsed losses have not been attributed to a specific subsystem by this batch. Three samples per case are not a broad statistical equivalence proof. IDM exit codes remain 1 despite complete verified files; their meaning is unverified.

[Build and source inputs](validation/publication-owned-pin-9.62/build.json), [backend checks](validation/publication-owned-pin-9.62/suites.json), [all performance gates and pair results](validation/publication-owned-pin-9.62/comparison/analysis.json), [raw per-transfer records](validation/publication-owned-pin-9.62/comparison/results.json).

## Next work

Keep first exclusive availability, total elapsed, process-exit and post-exit durability as separate gates. Capture the remaining body/intent/catalog waits and final object release on a common clock for both the current and frozen paths. Attribute the sequential/whole-time losses before changing policy. Do not infer a global disk/filter cause from these application timings, add pacing or keep rerunning until the samples turn green. A faster durable catalog-acceptance design requires an explicit recovery/receipt design and crash qualification; simply releasing protection early is not an acceptable substitute.

Live installed/browser timings, actual completion-dialog presentation and full inside/out IDM parity remain unverified. This increment does not create an installer, install an app, commit, push or release. Sanitized build/failure evidence is retained and generated test data is removed after export.

## Subsequent source-query attribution

[Fifteen diagnostic downloads](publication-source-query-attribution-9.62.md) identify GetFileInformationByHandle waiting during body flush; all nine owned observations have zero actual intent/body overlap. [9.63](publication-source-snapshot-9.63.md) carries pre-flush metadata and recovery identity to remove this wait. Its normal performance gates remain open.
