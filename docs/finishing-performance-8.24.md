# Written-body hashing and finishing: 8.24

Installed desktop **8.24**, with Chromium and Firefox manifest files **0.71.88**. Browser runtime JavaScript is unchanged. This increment follows the adaptive writeback and advisory-hash overlap in [8.23](finishing-performance-8.23.md).

## Implementation

Fresh, same-volume parallel HTTP transfers larger than 4 MiB and no larger than 64 MiB can retain their successfully written body in one optional process-wide memory slot. Credited writes contribute actual intervals; a complete byte count alone cannot qualify a buffer with holes. Out-of-order and overlapping writes merge coverage. Invalid offsets, more than 4,096 disjoint intervals, allocation failure or an occupied slot disable caching and retain the file-reading fallback.

After all body writers join, the advisory SHA-256 task may read this immutable buffer while publication and final writeback proceed. Cancellation and future ownership remain as in 8.23. Completion still waits for the hash task, final required flush, cleanup and persisted complete record. Required checksum gates are unchanged. Sequential, resumed, oversized and ineligible transfers retain their existing paths. The independent post-exit hash of the published file checks the actual output, separately from the retained-body digest.

## Qualification and installation

All **1,802 native checks** passed, including 24 written-body coverage, fragmentation, ownership and fallback checks. The qualified build contains 25 binaries, 489 frozen inputs and 42 canonical core objects. Transfer and Bridge rebuilt; the other 40 core hashes were retained. Seven installed files were replaced and hash-verified; personal catalogs were preserved. [Qualification](validation/finishing-8.24.json) · [Installation](validation/finishing-8.24/installed.json)

## Original comparison and storage conditions

The [original comparison](validation/finishing-8.24/comparison-before-cleanup.json) failed the performance target. Median last-body-byte to full application exit, UDM versus IDM: 4 MiB sequential **386.882 / 394.120 ms**; 64 MiB sequential **449.582 / 319.104 ms**; 64 MiB split **1,171.374 / 511.846 ms**. All 18 independently hashed outputs and UDM final recorded hashes matched. All three split transfers used the written buffer; their remaining foreground hash wait was negligible, while publication/catalog and flush stalls remained.

C: had approximately 1.6 GB free, less than 1%. A preplanned cleanup removed 1,294 owned generated files and completed test payloads, 2,749,657,809 logical bytes, with length/SHA-256 and absolute-path checks before each individual deletion. Observed free space increased by approximately 2.75 GB to 4.37 GB. Logical sizes are not allocation measurements. Inspected media-helper aliases already shared hard links and were preserved, as were current binaries, canonical core, source, catalogs, journals, rollback records and required historical fixtures. [Cleanup receipt](validation/finishing-8.24/storage-cleanup.json)

One separately labelled comparison after this deliberate storage change uses the identical installed executable and the same three-case, three-trial protocol. Preserve both runs, every outlier and full elapsed times; this is not evidence that low free space alone caused the original stalls.

### Comparison after cleanup

All eighteen outputs and final UDM hashes matched; IDM connection settings were restored. All three parallel UDM trials used the written-body buffer. [Complete results](validation/finishing-8.24/comparison-after-cleanup.json)

| Case | UDM median finishing tail | IDM median finishing tail | UDM median full elapsed | IDM median full elapsed |
| --- | ---: | ---: | ---: | ---: |
| 4 MiB sequential | 125.374 ms | 421.462 ms | 1.000 s | 2.045 s |
| 64 MiB sequential | 145.169 ms | 323.527 ms | 8.921 s | 9.888 s |
| 64 MiB split | 287.233 ms | 423.377 ms | 1.996 s | 3.735 s |

Seven of nine paired finishing tails were faster. Two still failed: 4 MiB trial 2 was 670.700 versus 596.957 ms; sequential 64 MiB trial 2 was 464.429 versus 318.817 ms. **The no-slower-in-any-test target remains unmet.** Every split tail was faster in this run; this is bounded local evidence, not a guarantee. The sequential outlier includes approximately 190 ms in CommitOutput and 76 ms in final flush, while foreground hash wait is under 0.1 ms. The next investigation should trace durable catalog/publication I/O and writeback under load, preserving recovery guarantees rather than suppressing the final work or relabelling completion.

The eighteen new payloads were independently rechecked and individually removed afterward, another 830,472,192 logical bytes. [Cleanup](validation/finishing-8.24/comparison-after-cleanup-payload-cleanup.json). Reports, catalogs and all failed observations remain.

The [protocol](finishing-performance-8.21.md) measures actual process exit after background work and automatic shutdown, then verifies output size, independent SHA-256 and UDM's final saved hash. This includes shutdown overhead and is not visual completion-dialog timing. Native desktop-control initialization remains unavailable, so no fresh live GUI timing is claimed. Three loopback trials cannot establish universal parity, physical power-loss durability or real-network behavior.
