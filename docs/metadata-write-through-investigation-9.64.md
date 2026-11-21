# Metadata write-through investigation on UDM 9.64

The normal app remains **9.64**, with source extension versions **0.73.28**. This investigation changed isolated experimental copies and added reproducible benchmark fixtures; it did not change production code, install an update or qualify an IDM speed win. Installed app 8.68 and extension files 0.72.32 are unchanged. All losing observations are retained.

## What was investigated

The existing metadata transaction writes a temporary file, calls `FlushFileBuffers`, closes it, then replaces or moves the file. Publication intent and final catalog persistence remain substantial work after the 9.63 source-query fix. The private prototype used aligned, space-padded JSON written through `FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH`, with checked write/close and the existing replacement protocol. It did not change the download body, its final flush, publication ordering or source ownership.

Microsoft documents write-through and no-buffering behavior, including hardware-cache support limitations and NTFS metadata flushing resulting from write-through requests. That supports investigating this policy, but does not establish its power-loss behavior on this PC. Alignment must follow the actual storage requirements. [CreateFileW](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew), [File buffering](https://learn.microsoft.com/en-us/windows/win32/fileio/file-buffering).

The small-file control used 8,191-byte and 32,769-byte JSON in opposing orders. All twelve byte-prefix, padding and parsed-JSON checks passed. Cached primary writes took about 20–23 ms, versus about 2.9–3.7 ms for unbuffered write-through. Including a symmetric post-publication flush narrowed these ranges to about 34–41 ms and 19–27 ms. These are small metadata fixtures, not a whole-app or IDM result. The additional flush is explicitly counted rather than treated as free background work. [Small-file evidence](validation/metadata-write-through-investigation-9.64/small/owned-files/results.json).

## Whole-app controls and failed performance gate

Three isolated source epochs reused the same normal App entry and resource objects. Each compared two policies in the same executable, ran both orders over 4 MiB sequential, 64 MiB sequential and 64 MiB split downloads, and counted process exit plus a symmetric post-exit target flush. Exact byte count, SHA-256 and saved Complete records passed for all 36 transfers. There were two trials per case; this is an exploratory screen, not a statistical qualification. First-launch samples remain in the record.

| Epoch | Finishing wins / 6 | Full-process wins / 6 | Through post-exit flush finishing wins / 6 |
|---|---:|---:|---:|
| Intent only | 3 | 4 | 5 |
| Intent and catalog, initial cache handling | 2 | 5 | 2 |
| Intent and catalog, exact physical text cached | 4 | 5 | 4 |

The corrected final epoch still lost its first split trial by **69.562 ms** at process exit and **48.655 ms** in full-process time. Its second sequential 64 MiB trial lost **1.829 ms** at process exit and **4.866 ms** through the extra flush, despite a faster total time. Those samples prevent a no-regression claim. No new IDM run was performed here, and the earlier normal-build IDM availability gaps remain open. [All paired endpoints and losses](validation/metadata-write-through-investigation-9.64/comparison.json).

## Exact checkpoint reuse

Space padding preserves JSON but changes the exact file text. The initial combined prototype cached the unpadded serialization, so stop and destruction could not reuse the existing checkpoint: one save became three catalog writes. A focused same-source fixture reproduced this failure and then verified the correction. The corrected private writer returns the actual padded text for `checkpointText`, while retaining the unchanged parsed checkpoint state and strict text equality. Both normal and corrected candidate controls perform one total write. This is a prototype integration requirement, not a bug introduced into normal 9.64. [Before/after negative control](validation/metadata-write-through-investigation-9.64/checkpoint-reuse/results.json).

## Controlled crash recovery

Sixteen fresh-process controls cover both normal and corrected candidate policies, on C: and D:, at four publication points: temporary intent persisted, published intent, final file moved, and catalog written. Each seed process terminates itself with exit code 197; a fresh Manager then recovers the transaction. All sixteen recoveries passed. The unpublished temporary-intent case retains the original stage and uncommitted state; later points recover the exact 14-byte final file and Complete record. The first compile failed because the probe named a nonexistent recovery enum; the retained log and corrected fixture make that correction explicit.

This exercises real metadata format and recovery code with a small synchronous publication. It does not test hardware power loss, an interrupted asynchronous body flush, every failure syscall or all filesystems. [Recovery evidence](validation/metadata-write-through-investigation-9.64/crash-recovery/results.json).

## Admission requirements and next work

The private runtime selector identifies files by pathname and is unsuitable for production. Before this policy can be considered for an app increment:

1. Introduce an explicit metadata persistence interface for publication intent and canonical catalog, with the exact physical text returned to checkpoint caching. Preserve all catalog proof and owner-retirement ordering.
2. Qualify supported local storage; obtain alignment before writing; define a safe fallback before any partial transaction. Cover unsupported filesystems, compression/encryption, network volumes, zero/large inputs, padding overflow and the catalog size limit.
3. Exercise short/failed writes, close failure, replacement conflicts, interrupted rename, incomplete temporary files and catalog proof failures. Preserve existing retry and recovery behavior.
4. Measure normal successful and failed transactions against the current build and IDM, including first exclusive availability, process lifetime and all attributable background work. Retain losses; reject regressions rather than choosing a favorable sample or size threshold.
5. Adopt only if correctness and performance gates pass; then increase both source versions and build all native products. Installation remains a separate authorized action.

The next useful attribution is the interaction between body flush and metadata persistence on the same volume, using operation timings and attributable disk work. The current evidence does not establish that a disk queue, filter driver or hardware cache caused the remaining variance. More scheduling delays or hidden deferred work would not resolve that uncertainty.

## Evidence and cleanup

[Export manifest](validation/metadata-write-through-investigation-9.64/export-manifest.json) preserves sanitized records, private source patches, helper variants, drivers and fixture hashes. All 680 normal build inputs were verified unchanged; eleven protected installed/personal/helper files matched their pre-investigation hashes. The normal build and its referenced core objects are retained. The 36 download payloads totalled 1,584 MiB and were removed after verification. Exact private-root cleanup and retained-product checks are recorded separately in the [final audit](validation/metadata-write-through-investigation-9.64/final-audit.json).

The recursive folder deletion was blocked by automatic approval review. A narrower follow-up removed sixteen individually hash-verified generated EXE/OBJ/RES files without recursion: 174,143,066 logical bytes (about 166 MiB). Small private records and directories remain. The original audit records the blocked attempt; the [follow-up receipt](validation/metadata-write-through-investigation-9.64/binary-cleanup-follow-up.json) records the successful bounded cleanup.
