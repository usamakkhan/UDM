# Reuse the pinned catalog for completion proof, 8.64 / browser 0.72.28

Completion and clean-shutdown proofs now read the actual catalog handle they already pinned. Previously, those paths opened a restrictive read handle, then called the pathname-based `readText`: a separate pathname size lookup and another stream open/read. The new reader removes those redundant pathname operations and uses bounded chunked reads of the pinned file object. It still parses and compares the entire catalog; it does not substitute a size, timestamp or identity-only shortcut.

This improves the catalog-proof component. It does **not** prove that the earlier multi-second finishing stall is fixed, that all application timings improve, or that UDM matches IDM in every case. Ordered body writes remain private after the earlier no-regression failure.

## Ownership and failure behavior

The two Core call sites retain their existing `GENERIC_READ` handle with only `FILE_SHARE_READ`, opened with `FILE_FLAG_OPEN_REPARSE_POINT`. The handle remains alive throughout read, parse and comparison. The [reader](../native/PinnedCatalogRead.hpp) requires a disk file, rejects directory/reparse attributes, checks measured length against the existing 32 MiB bound before allocating, seeks to zero and requires exact successful reads. Empty files and UTF-8 BOM handling preserve the old text-reader semantics; malformed JSON still fails the unchanged parser comparison.

Read/seek/size/type failures, malformed or different content, failed pins and existing storage/admission/publication conditions continue through the existing forced-save fallback. The old accepted-state equality checks, sequence checks, pending overlays and mandatory flush/publication barriers remain. Explicit saves are still forced. No cached identity is treated as proof of unchanged contents.

## Build and correctness

Native definitions advance to 8.64; both browser manifests advance to 0.72.28. Browser JavaScript is unchanged. The [native receipt](validation/pinned-catalog-read-8.64/build-result.json) and [normal receipt](validation/pinned-catalog-read-8.64/default-build-result.json) cover 526 frozen inputs, ten products, five retained product entry objects and 43 distinct core objects across both variants. [Dependency audit](validation/pinned-catalog-read-8.64/native-closure-audit.json) rebuilds Core and Bridge and retains forty qualified core objects. The unchanged separate normal Transfer object is reused without enabling the private ordered define. All [fifteen executable resources](validation/pinned-catalog-read-8.64/resource-versions.json), including the four backend fixtures and separate comparison controller before cleanup, report 8.64.

All **502 selected checks in eight runs** pass on both drives: seventeen new pinned-read checks, forty catalog-checkpoint checks, 145 actual completion-checkpoint checks and forty-nine Manager disk-credit checks per drive. [Suite results](validation/pinned-catalog-read-8.64/suite-results.json). This is affected qualification, not a rerun of every historical suite.

The [new fixture](../native/PinnedCatalogReadTests.cpp) verifies actual writer and rename exclusion, exact and oversized limits, repeated reads after EOF, retained-object reads when a deliberately weaker test pin permits pathname replacement, empty/BOM/partial-BOM/multi-chunk input, invalid/write-only/directory handles and pipe refusal before blocking IO. Existing Manager cases still verify malformed/changed catalog repair, failed-save retry, held/superseding completion checkpoints, no extra acknowledged completion write and interrupted-publication recovery. Reparse refusal is implemented; no privileged symlink-creation test is claimed.

Two normal App 4 MiB samples on C:/D: independently verify exact size/SHA-256, completed catalog, normal exit and separate post-exit output flush. [App records](validation/pinned-catalog-read-8.64/app-smoke.json). These small integration samples validate the changed proof path without repeating the earlier 64 MiB transfer matrix; they do not establish speed parity.

## Measured catalog proof cost

The [comparison controller](../benchmarks/speed/pinned_catalog_read_control.cpp) holds the same production-style read pin for both readers. It compares the unchanged old `readText(path)` function with the new pinned reader, including the same JSON parse, equality comparison and temporary destruction in the timed expression. Files contain representative deterministic download records and are written once per case. Each reader is warmed once, followed by ten alternating paired measurements. Common pin acquisition is outside the timing. This measures warm cached catalog proofs, not complete application startup, network transfers or cold disk performance.

All **120 measured proofs** return exact equality. Each row below is a median of ten samples, in milliseconds.

| Drive | Records / raw bytes | Path reader | Pinned reader |
| --- | ---: | ---: | ---: |
| C: | 128 / 25,978 | 1.004 | 0.704 |
| C: | 7,500 / 1,556,738 | 71.733 | 70.599 |
| C: | 30,000 / 6,296,738 | 231.510 | 188.294 |
| D: | 128 / 25,978 | 1.127 | 0.698 |
| D: | 7,500 / 1,556,738 | 60.548 | 47.226 |
| D: | 30,000 / 6,296,738 | 241.343 | 205.137 |

Median thread-cycle counts are lower for the new reader in all six cases. However, **seven of sixty individual pairs have a slower pinned wall sample**; all samples and outliers remain in the [C: measurements](validation/pinned-catalog-read-8.64/proof-C.json), [D: measurements](validation/pinned-catalog-read-8.64/proof-D.json) and [summary](validation/pinned-catalog-read-8.64/proof-comparison.json). No universal per-run win or overall millisecond guarantee follows from these medians.

## IDM access and remaining work

IDM's process was present and responsive, but Computer Use returned no targetable IDM window. Opening the existing app returned `GetCursorPos failed: Access is denied. (0x80070005)`; a fresh selection and one retry returned the same error. No process was forced closed, no permissions changed, and no administrator prompt opened. [Access record](validation/pinned-catalog-read-8.64/idm-access-attempt.json). No new IDM comparison is claimed.

The next latency work remains the outstanding full-lifetime body/optional-writeback/publication investigation using 8.63's wait boundaries, plus installed/browser/IDM comparisons when accessible. This source-level simplification removes proven redundant work without changing IO pacing or weakening final proof. The broader feature/design parity objective remains open.

## Cleanup and provenance

[Cleanup](validation/pinned-catalog-read-8.64/cleanup.json) removed 178 generated files / 54,970,568 logical bytes in twelve owned roots, including generated comparison catalogs, fixture executables and entry objects. The two verified App payloads, another 8,388,608 logical bytes, were removed during their runs. Deletion verifies ownership boundaries, reparse absence, per-file hashes and complete owned hardlink groups. Logical bytes are not physical allocation or wear measurements. An initial preflight compared lowercase and uppercase digest strings; it refused before deletion. Case-normalized hashes matched and cleanup then completed without repeating downloads or comparisons.

[Final verification](validation/pinned-catalog-read-8.64/final-verification.json) rechecks all qualified inputs, the comparison source, ten products, 43 core objects, five product entries and three protected installed/personal files. The [seven-file source delta](../benchmarks/speed/experiments/pinned-catalog-read-8.63-to-8.64.patch) reconstructs the five prior qualified inputs and adds the reader/test; reverse-application checking passes.

Installed UDM remains unchanged. No installer, installation, browser deployment/reload, administrator prompt, push or release occurred.

The subsequent [8.65 accepted-text proof](accepted-catalog-text-8.65.md) removes the repeat JSON parse and also updates the destructor proof left outside this increment. Actual Manager-stop comparisons show a further warm large-catalog gain; broader download/IDM parity remains unproven.
