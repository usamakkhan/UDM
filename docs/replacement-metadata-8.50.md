# UDM 8.50: completion metadata bound to verified replacement files

UDM 8.50 fixes reproduced replacement-recovery defects that could persist a Complete size different from the actual file and consume the recovery journal. Native version advances to 8.50 and both extension source manifests to 0.72.14. Browser JavaScript is unchanged. Installation remains deferred at 8.32.

## Five-case 8.49 control

The [exact baseline probe](../benchmarks/speed/replacement_size_probe.cpp) links the qualified 8.49 core and resources. It creates real interrupted replacement publications in isolated catalogs, then changes only selected journal metadata before recovery. Four cases acknowledge Complete, consume the journal and persist Size 100 for an actual 14-byte new file or 9-byte old file:

| Case | Actual file bytes | Saved completed size | Journal consumed |
| --- | ---: | ---: | --- |
| Completed new replacement | 14 | 100 | Yes |
| Completed previous version | 9 | 100 | Yes |
| Retained original with changed Before accounting | 9 | 100 | Yes |
| Restored original with changed Before accounting | 9 | 100 | Yes |

A fifth fractional-size control is already rejected by the current-format publication binding. It is retained to distinguish an existing validation from the newly fixed false completions. [Results, exact source/binary/core hashes](validation/replacement-metadata-8.50/size-baseline-proof.json). No personal catalog or installed executable is used by these probes.

## Completed pairs require measured agreement

Recovery already owns the exact files and hashes their contents. Before applying a completed pair, it now requires both After records to describe the corresponding held file: Status Complete, exact bounded integer Size/Received, equality of those counts to the measured length, the measured SHA-256, and the actual absolute destination. Unsafe file names and mismatched destinations are refused. This also protects legacy journals that do not pass through the newer commit-identity binding.

The current replacement target and original location must match the operation's actual target/previous destinations. Still-present pending ReplacementOf, ReplacementHash and PreviousPath fields must agree too. The checks bind history to the verified file rather than trusting consistent-looking journal numbers or paths. A mismatch preserves both byte versions, current records and the immutable intent for review/retry. Correcting the owned fixture journal to its healthy saved operation allows explicit retry.

This performs no extra hash pass and does not change the existing file-sharing, rename or durability protocol. The 8.49 single-pass intact-original classification remains covered by its read-loop regression suite.

## Retained and restored originals use their verified bytes

An OriginalBefore record can contain stale accounting, including a genuinely stale catalog record from before the replacement began. Replaying that accounting made the prior implementation resurrect the wrong completed size. The repaired original now takes Size, Received, SHA-256 and destination from the actual held verified old file. The new replacement remains Paused with its unpublished bytes. Descriptions and the original finish history remain unchanged.

The restoration rename also preserves the measured length when replacing its temporary share-delete source handle with the final-name pin. A zero-byte original remains a valid completed zero-byte file; it does not become an unknown-size record when that handle changes. Failed catalog persistence preserves the journal and rolls back uncommitted model fields; retry can reconcile the already restored physical name.

## Shared completed-counter schema

`PublicationCatalogProof::completeByteCounts` accepts only nonnegative integers within the signed 64-bit file-size domain, with Size equal to Received. Normal publication binding and recovery-proof preparation use the same check. Fractions, booleans, strings, missing counters, negative counts, unsigned overflow and unequal counters cannot be acknowledged as completed recovery. Paused rows retain their separate partial/unknown-size schema.

The proof schema validates the counters' representation; the replacement IO check additionally proves agreement with the measured file. Neither an ID, a valid-looking count nor a higher checkpoint sequence substitutes for byte verification and a containing durable snapshot.

## Qualification

All **10,686 focused checks pass across 24 runs**. The [validation receipt](validation/replacement-metadata-8.50.json) records final checks, sources, core inputs, executable versions, app lifecycle and cleanup. The new 2,490-check matrix covers 228 recovery cases plus normal/recovery proof-schema checks, and runs on C: and D:. It tests current and legacy visible replacements, current private replacements, empty/nonempty old and new files, completed pairs, retained originals and restored originals. Legacy private journals without commit identities are not a supported format and are not represented as a new capability.

Completed-pair faults change new/old counts, Received, numeric type/range, missing fields, digest and destination. Original-repair cases change Before accounting, digest or path and include an actually stale pre-publication catalog. Rejected operations preserve records, bytes and journal; successful repair or explicit retry verifies exact file paths, lengths, hashes, row visibility and metadata. Private completion receipts are read from the actual persisted catalog before another save. Real catalog-flush failures test rollback and retry for every branch and supported format. Normal/proof tests also exercise zero and the signed-range upper bound without creating huge payloads.

All existing byte-IO, read-count, ownership, journal, catalog/proof, replacement/publication, synchronization, File Info, header-draft, pending cancellation and headless UI-command regressions run against the same final core. Native fixtures are separate from installed/browser or rendered UI acceptance. There is no claim of completing the full recovery transaction or IDM parity.

Core, Duplicates and the version-reporting Bridge are rebuilt. A recursive owned-include audit selects those three changed native units and retains twenty-one unchanged native units; eighteen external C inputs remain pinned. Five production programs and nineteen fixture programs are built from the verified 42-object selection. Canonical objects and prior qualified references remain unchanged. Media helpers are shared through a junction; there is no extra FFmpeg copy.

A complete ordinary 4 MiB app download used one origin request, matched independent SHA-256/length, retired its intent, exited normally and passed an explicit post-exit output flush. Exit and durability tails were 219.002 ms and 241.179 ms. This is an unpaired lifecycle smoke, not an IDM comparison or speedup gate; prior timing rows and losses remain. Its payload was removed immediately. [App smoke](validation/replacement-metadata-8.50/app-4m-smoke.json).

## Limits and next work

Journal discovery/open/read, post-acceptance and exceptional handle-release cleanup, zone/retained-output work and receipt-only pruning still hold the model lease. Continue the discovery/ownership-handoff and cleanup stages in the [full transaction design](publication-recovery-transaction-design.md), preserving corrupt/changed evidence, exact retirement and receipt ownership. Further caller and external namespace/alias audits remain necessary; this change does not establish a globally atomic filesystem transaction.

Normal mutation saves, completed Properties, installed visual/browser checks, original sequential/split finishing losses and full background-work IDM comparisons remain open. This integrity fix is not a speedup or universal timing claim. The installed app remains 8.32; no installation, extension deployment/reload, installer, push or release is performed. Personal data and installed hashes are preserved.

## Source and cleanup

The [8.49-to-8.50 source delta](../benchmarks/speed/experiments/replacement-metadata-8.49-to-8.50.patch) reconstructs seven changed/added native/manifest inputs in both directions with exact hashes and recorded line endings. The baseline source is independently retained as a repository benchmark artifact with a tested-source hash. These are source proofs, not promises of timestamped binary reproduction.

Cleanup is limited to individually checked owned payloads, fixture/control artifacts, redundant target objects and temporary source copies after evidence is saved. Paths, parents, file identity and complete hard-link groups are checked; no recursive or directory deletion is used. Five current production programs, three rebuilt private core inputs, thirty-nine retained inputs, prior qualified references, shared tools and personal data remain. Removed **4,478 owned files totaling 241,807,467 logical bytes**, plus the separately deleted 4 MiB smoke payload. There were no unresolved link groups, skipped paths or directory deletions. These are logical totals, not physical allocation claims.
