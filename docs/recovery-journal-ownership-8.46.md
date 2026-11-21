# UDM 8.46: owned recovery journal snapshots and retirement

UDM 8.46 closes a reproduced recovery race: a journal could change after being read, and recovery could then delete the newer file at the same pathname. The fix covers both ordinary publication and replacement journals. Extension source versions advance to 0.72.10. The unhelpful 8.45 early-zone metadata experiment is retired; the ordinary Transfer source and its qualified core object are restored to 8.44.

## Reproduced defect

The baseline probe uses actual interrupted ordinary and paired replacement publications. At the real recovery catalog `temporary-flushed` boundary, it either renames the original journal and inserts a new one, overwrites the journal, or writes through a second hard link. All six baseline variants allowed the mutation, then deleted the changed journal path without reporting failure. Existing model locking did not protect filesystem objects from those operations. [Baseline cases and exact probe hashes](validation/recovery-journal-ownership-8.46/baseline-reproduction.json).

## Ownership and retirement

`RecoveryJournal` opens the journal with read and delete access and read sharing, rejects unsupported/directory/reparse inputs, bounds its held-file read, and preserves existing UTF-8 BOM handling. JSON parsing consumes that immutable held snapshot. The same handle stays alive through byte verification, recovery moves and the existing durable catalog save. A failed parse, failed catalog save or abandoned lease closes its handle without deleting the journal: it is not opened with delete-on-close.

After acceptance, retirement uses file disposition on that held handle instead of resolving the journal pathname again. Extended POSIX disposition is requested first, with classic disposition used only for unsupported-operation errors. No read-only-attribute override is requested. Microsoft documents the required DELETE access for [SetFileInformationByHandle](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-setfileinformationbyhandle) and the [disposition/link semantics](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/ns-ntddk-_file_disposition_information_ex).

A reader inspecting a held journal must share deletion. An existing reader that refuses deletion sharing now stops recovery before it changes the selected row or moves the saved bytes; after that reader closes, explicit retry succeeds. Scoped replacement discovery does not claim unrelated readable journals: it checks relevance, then reparses the relevant operation from its owned snapshot. A live known publication owner is deferred before acquiring its journal handle.

## Verification

All **1,661 checks passed across 12 runs**: the expanded 85-check journal suite on both C: and D:, plus publication recovery 73, ownership 186, publication commit 347, replacement commit 395, synchronization replacement recovery 16, early-intent recovery 156, completed checkpoint 145, preparation 87, catalog checkpoint 40 and catalog coordinator 46.

The journal suite exercises real ordinary/replacement catalog boundaries, rename/write/hard-link mutation exclusion, shared inspection, preserved journals and rows after a catalog failure or refused ownership, explicit retry, unrelated locked scoped journals, UTF-8 BOMs, size limits, abandonment, single-link retirement with a retained alias, directory refusal, and actual read-only retirement failure followed by retry. The six formerly successful mutation variants are excluded on each tested drive. [C: cases](validation/recovery-journal-ownership-8.46/RecoveryJournalTests.json) · [D: cases](validation/recovery-journal-ownership-8.46/RecoveryJournalTests-D.json).

Five production executables and eleven fixture executables were built and all sixteen file versions checked as 8.46. Their 42 core inputs use two rebuilt private objects (Duplicates and Bridge), the qualified 8.44 Core and Transfer objects with unchanged source closure, and 38 retained canonical inputs. The selected Duplicates object is explicitly verified as the newly compiled object. The later test-only expansion rebuilt only its fixture; its final source/binary hashes are included in the final receipt. [Complete qualification and exact hashes](validation/recovery-journal-ownership-8.46.json).

An actual 4 MiB ordinary download on D: matched its independent length and SHA-256, used one request, completed through the restored serial publication path, exited normally, and passed a separate post-exit output flush. Its 161.637 ms exit tail and 175.561 ms durability tail are a single lifecycle smoke, not an IDM comparison or speedup claim. The payload was immediately removed. [App smoke](validation/recovery-journal-ownership-8.46/app-4m-smoke.json).

## Limits and next work

Recovery still holds the shared model mutex across its disk work. The probes explicitly observe that lock unavailable at the catalog IO boundary. This fix is a journal-ownership prerequisite, not the caller/reservation migration and not a finishing-delay fix. Snapshot-bound recovery catalog proof, detached IO with complete job-tree/path reservations and revalidation, and migration of every outer-lock caller remain required by the [transaction design](publication-recovery-transaction-design.md).

The native C:/D: cases do not separately force the unsupported-filesystem disposition fallback. No installed/browser timing comparison was performed. The original slower sequential/split cases and complete IDM parity remain unresolved. No body flush, intent or catalog durability barrier was relaxed and no new pacing rule was added.

Installed desktop remains **8.32**. The research-first installation deferral remains; no installer, installation, extension deployment/reload, push or release was performed. Both personal catalogs and the installed executable retain their hashes.

## Retiring the 8.45 experiment and cleanup

The 8.45 metadata candidate failed its full-lifetime timing gate, so its active path, helper and fixture are removed. Its correctness results and measured losses remain. A ten-file [8.45-to-8.46 source delta](../benchmarks/speed/experiments/retire-zone-8.45-to-8.46.patch) reconstructs the former candidate and the current sources; all recorded source hashes were verified after restoring line endings. Temporary source copies were removed. This proves source reconstruction, not bit-identical timestamped executable rebuilding. [Reconstruction evidence](validation/recovery-journal-ownership-8.46/experiment-source-reconstruction.json).

Individually removed 892 generated/retired files totaling 215,457,268 logical bytes, including nine retired 8.45 executable/core/resource artifacts totaling 57,591,008 bytes. The 4 MiB smoke payload and 278,142 bytes of temporary source reconstruction were also removed. All eleven current fixture executables and redundant target objects are retired after verification. Five current production binaries, two private core objects, qualified 8.44 Core/Transfer inputs, prior qualified 8.42 references, canonical tools/objects and compact evidence remain. Complete hard-link groups were verified; there was no recursive deletion, junction/directory deletion or process termination. Logical byte totals are not physical allocation reclaimed. [Cleanup receipt](validation/recovery-journal-ownership-8.46/cleanup.json).
