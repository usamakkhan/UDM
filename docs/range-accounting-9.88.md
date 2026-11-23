# UDM 9.88: repeated split-range accounting

This is a diagnostic follow-up. Native production code, five built products and source extension versions remain **9.88 / 0.73.52**. The benchmark now saves each owned origin snapshot on both success and failure. A new read-only analyzer counts coverage and repeated socket-send intervals, with six passing tests for adjacent, nested, reordered, empty, gapped and invalid ranges. No installer or installation occurs.

## Three fresh split pairs

The same uninstalled normal UDM binary and installed IDM 6.43.10.2 download deterministic 64 MiB files on D: with eight configured connections. Client order alternates. All six payload hashes match, all UDM catalogs report Complete and every observed process exits. IDM again returns launcher code 1 despite correct files; its success-code meaning is not established. Connection settings are restored. These are CLI lifetimes, not GUI completion timings.

| Pair | Client | Total to exit, s | Last send to exclusive availability, ms | Last send to exit, ms | Through post-exit output flush, ms | Extra origin bytes | Requests |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | udm | 2.670 | 341.993 | 424.379 | 485.866 | 3,520,709 | 8 |
| 1 | idm | 3.624 | 125.694 | 429.332 | 1116.762 | 1,533,705 | 17 |
| 2 | idm | 3.682 | 148.011 | 454.686 | 1127.217 | 1,271,373 | 15 |
| 2 | udm | 3.203 | 630.478 | 728.175 | 751.880 | 4,363,165 | 9 |
| 3 | udm | 3.038 | 251.208 | 347.867 | 369.309 | 4,290,963 | 10 |
| 3 | idm | 3.853 | 124.038 | 457.408 | 1195.588 | 1,967,138 | 13 |

UDM has lower total process time in all three pairs, but first exclusive availability is slower in all three. Pair 2 also loses the exit-tail comparison: **728.175 ms versus 454.686 ms**. The 424.379 ms UDM exit-tail median versus 454.686 ms IDM median does not erase this loss. Three pairs do not establish broad speed parity or a meaningful tail percentile. [Results](validation/range-accounting-9.88/results.json).

## What the range evidence establishes

Every saved send-interval union covers all **67,108,864** output bytes without holes. Counts exactly reconcile with the harness origin totals. UDM repeats **3,520,709 / 4,363,165 / 4,290,963** bytes; IDM repeats **1,533,705 / 1,271,373 / 1,967,138**. Thus the extra totals represent overlap within the resource, not a larger output or a metadata-only request count. The analyzer records multiplicity and individual requests so nested splits are counted correctly rather than summed as unique coverage. [Accounting](validation/range-accounting-9.88/range-accounting.json).

UDM reuses the initial 200 response for its first range and then issues helper ranges whose declared ends can exceed their later shortened ownership. This is visible in both the retained request ranges and `Transfer.cpp`: live segment End bounds retained bytes; reads are limited to current ownership, a split can shorten that ownership during a pending read, and excess read bytes are charged separately. Dynamic shortening plus buffered response delivery is a concrete path for overlap. The records do **not** isolate how much each client buffered, discarded or retained, or prove that a particular split policy caused every duplicate byte.

Origin counts are bytes accepted by the server socket. Snapshots can precede handler teardown and are not a claim about every subsequent server operation or bytes consumed by the client. Request outcomes and handler-closed flags are retained without filling in missing outcomes. The analyzer rejects out-of-resource intervals and incomplete coverage. Future terminal accounting should record both process-exit and quiescent-origin snapshots while retaining the original performance endpoints.

## Availability ownership and next implementation gate

Current source inspection confirms that `Duplicates.cpp::publishFile` releases ordinary source/name pins **after** `PublicationCatalogProof::resolve` accepts the completed catalog. `PublicationPreparation::releaseAcceptedFile` already releases those pins before journal cleanup. The writer-excluding source and deletion-excluding published-name handles therefore enforce a real acceptance barrier; removing them before durable acceptance would change correctness. The historical post-acceptance pin hold is not being reintroduced as an explanation.

Next capture a common-clock current-binary diagnostic for receiver retirement, body writeback, intent write, rename, catalog snapshot/write/resolve, actual file-pin close and exclusive-open observation. Preserve a normal uninstrumented control and record handle lifetime separately from disk/flush time. Existing 9.60 traces localize historical serial durability costs but cannot attribute the current 9.88 intervals. Change one measured owner only after that attribution, then repeat alternating total/availability/exit/explicit-flush comparisons and publication recovery tests. In parallel, replay the recorded ranges against cancellation/ownership decisions before changing split assignment; request-count reduction alone is insufficient.

## Provenance and cleanup

The launch harness verifies the candidate executable SHA-256 before execution. Script hashes in the audit are recorded after execution and are not claimed as a pre-launch source snapshot. The six analyzer tests pass. Eleven protected installed/catalog/shared-tool files remain unchanged. All six verified payloads totaling **402,653,184 logical bytes**, plus **33 private record/catalog files totaling 163,917 bytes**, are removed after sanitized export. Shared media tools, current native products and source files remain. No physical-space or wear reduction is measured. [Audit](validation/range-accounting-9.88/audit.json) · [Cleanup](validation/range-accounting-9.88/cleanup-receipt.json).
