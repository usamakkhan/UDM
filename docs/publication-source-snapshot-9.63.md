# Captured source proof: UDM 9.63

UDM 9.63 fixes the source metadata query that serialized the intended final-body/intent overlap in 9.62. It captures information and stable identity on the writer-excluding file object before starting flush, carries the capture in a move-only capsule and verifies the actual named source after the body joins. Recovery now requires any recorded source identity even when the digest is known, preserving a foreign replacement with identical bytes.

Five native products build at 9.63; both source manifests advance to 0.73.27. All 1,277 affected assertions pass. The normal comparison still fails the no-regression and IDM first-availability gates, so this candidate is **not promoted for installation**. Installed app 8.68 and extension files 0.72.32 remain unchanged. This increment changes native finishing behavior; it adds no extension runtime capture logic.

## Actual blocker and implementation

The [9.62 attribution](publication-source-query-attribution-9.62.md) found 34–97 ms inside GetFileInformationByHandle on the duplicated synchronous body object while its flush was active. None of nine observed owned-path runs overlapped body writeback and intent writing. The new PublicationSourcePin captures its information and stable identity before the duplicate performs flush. Capturing uses write access plus attribute access, with read/delete sharing and no data-read access. The same file object excludes other writers until exact acceptance.

PublicationPreparation adopts that capsule and uses its measured size and identity without querying the actively flushing object. The capsule has no public constructor accepting arbitrary metadata. The early intent records both the digest and StagedIdentity, plus the existing FlushBeforePublish barrier. Named-file validation is deferred until after successful owned writeback; it then checks the live name, file identity, size and stable identity. Directory, cancellation and model-destination checks, non-overwriting rename, published-name protection and exact durable catalog proof remain. No flush or final checkpoint is removed.

Unknown hashes join/release the writeback pin and retain the original strict path. Ineligible transfers retain their existing synchronous behavior. Failure to capture a suitable identity or launch the task falls back without a detached worker. Transfer and prepared-owner unwinding still join body work before cleanup. Replacements, copied output and internal media publication keep their existing protocols.

## Safety and clock proof

The eleven suites pass 1,277 checks, including 107 publication/ownership/failure assertions, 145 actual HTTP completion assertions and 31 independent finishing checks. New coverage confirms that recovery refuses a different object with the same hash and preserves both objects and the recovery journal. Initial prechecks are retained separately and are not counted again. This batch exercises exception interruption/recovery; abrupt process termination and power-loss qualification of the new binding remain open.

Three private diagnostic downloads use the frozen normal 9.63 App entry object and instrumented Core/Queue/Transfer/Duplicates. Trace initialization is in the private Manager constructor; this changes observation coverage and is not a normal-product speed comparison. All three outputs and saved records match the independent size/hash oracle. QueryPerformanceCounter records prove:

| Case | Metadata capture before flush | Cached preparation | Body/intent overlap |
| --- | ---: | ---: | ---: |
| 4 MiB sequential | 0.180 ms | 0.002 ms | 42.627 ms |
| 64 MiB sequential | 0.172 ms | 0.002 ms | 111.485 ms |
| 64 MiB split | 0.142 ms | 0.003 ms | 24.169 ms |

Every capture ends before body writeback starts; no constructor source-information query runs during it; named-source validation follows body completion. Each trace passes PID/frequency/readiness/drop checks and is reconstructable from its retained event bytes and SHA-256. The strategy flag alone is not proof of overlap; these actual intervals supply the narrow ordering proof.

## Normal-product performance: every slower observation retained

Twenty-seven downloads rotate 9.63, frozen 9.62 and IDM through all three positions in three trials per case. They use the unchanged normal comparison harness, normal products and deterministic loopback data. No compiler runs concurrently. All files match size/SHA-256; UDM's saved Complete records match. Exclusive availability, all observed app process exit, total elapsed and a symmetric post-exit file flush are distinct endpoints.

| Case | Metric | 9.63 | 9.62 | IDM |
| --- | --- | ---: | ---: | ---: |
| 4 MiB sequential | Exclusive availability after last origin send | 121.033 ms | 122.326 ms | 54.432 ms |
| 4 MiB sequential | Process-exit finishing | 165.781 ms | 170.093 ms | 385.061 ms |
| 4 MiB sequential | Full process elapsed | 1036.331 ms | 1113.405 ms | 2086.252 ms |
| 4 MiB sequential | Post-exit output durability after last origin send | 189.268 ms | 183.917 ms | 431.814 ms |
| 64 MiB sequential | Exclusive availability after last origin send | 170.359 ms | 152.250 ms | 65.248 ms |
| 64 MiB sequential | Process-exit finishing | 220.109 ms | 199.279 ms | 562.216 ms |
| 64 MiB sequential | Full process elapsed | 8944.858 ms | 8901.844 ms | 10126.311 ms |
| 64 MiB sequential | Post-exit output durability after last origin send | 242.096 ms | 215.248 ms | 1201.464 ms |
| 64 MiB split | Exclusive availability after last origin send | 263.496 ms | 256.463 ms | 119.158 ms |
| 64 MiB split | Process-exit finishing | 309.387 ms | 304.074 ms | 395.765 ms |
| 64 MiB split | Full process elapsed | 2360.066 ms | 2436.707 ms | 3540.659 ms |
| 64 MiB split | Post-exit output durability after last origin send | 335.069 ms | 323.813 ms | 1091.027 ms |

9.63 wins three of nine previous-build availability/finishing pairs, four of nine full elapsed pairs, three of nine durable finishing pairs and four of nine durable elapsed pairs. Against IDM it wins eight of nine process-exit finishing pairs and all nine full elapsed/durable pairs, but loses all nine first-availability pairs. In the second split trial it takes 442.107 ms to exit after the last origin send versus IDM's 395.765 ms; this loss remains. Three samples per case do not prove statistical equivalence. Raw IDM exit codes remain 1 despite verified complete files; their meaning remains unverified.

The metadata serialization bug is fixed, but overlap does not guarantee a faster storage barrier. Median 64 MiB sequential finishing regresses by 20.8 ms versus 9.62, and median split finishing by 5.3 ms, while split full elapsed improves by 76.6 ms. The remaining body/intent/catalog flush variability and before-last-byte differences are not assigned to a disk queue, filesystem filter or scheduler without stronger evidence. No reruns are selected to turn the failed gates green.

[Build](validation/publication-source-snapshot-9.63/build.json), [1,277 backend checks](validation/publication-source-snapshot-9.63/suites.json), [ordering proof](validation/publication-source-snapshot-9.63/ordering-proof/summary.json), [all pair results and failed gates](validation/publication-source-snapshot-9.63/comparison/analysis.json), [every normal run](validation/publication-source-snapshot-9.63/comparison/results.json).

## Next admission work

Investigate the actual storage operations behind the remaining flush intervals before another policy change. Preserve the same availability, full elapsed and post-exit durability endpoints. A different intent persistence operation requires a documented OS durability contract, failure controls and matched normal-product evidence; moving waits outside the measurement, adding pacing or dropping a barrier cannot qualify a fix.

A faster durable completion-receipt/catalog architecture needs an explicit acknowledgment and recovery design, including crashes before/after rename, newer publication identity, user edits after acceptance and checkpoint/index failure. Early intent is pending work, not itself an accepted completion receipt. Releasing protection before acceptance is not an admitted shortcut. Abrupt-process crash cases for this new source binding and installed/browser/dialog comparisons also remain. Full inside/out IDM parity is unproven.

## Cleanup

All 45 diagnostic/comparison payloads (15 attribution, 27 normal comparison and three ordering proof transfers) are removed after verification, totaling 1,980 MiB logical payload bytes. Another 1,072 temporary artifacts are removed: 643,775,993 logical path bytes and 410,897,373 unique file-content bytes, with hard links counted separately. These are not claims about physical free space or drive wear. Sanitized evidence and reconstruction inputs remain; current and baseline products, required core objects, installed files, both personal catalogs and shared ffmpeg/ffprobe are preserved. No media helper copies, installer, installation, commit, push or release are made.

## Subsequent failure-lifetime correction

A later actual-worker control finds that the error handler can retire active ownership before the outer prepared scope joins its task. [9.64](publication-failure-owner-9.64.md) resets that scope before handling failure and passes the same regression fixture. The 9.63 speed results above are retained and do not establish 9.64 speed qualification.
