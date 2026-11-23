# Transfer-created stage cleanup: UDM 9.68

UDM 9.68 protects a replacement file or folder that appears at a created stage's old name before the ordinary transfer hands off publication. The source extension manifests advance to 0.73.32. Five native products build, and 1,351 assertions across seventeen affected suites pass. An additional real C:-to-D: fault run passes 36 checks, and a successful copied download passes four checks, including exact body hash and saved Complete status.

This is a failure-cleanup ownership fix. It does not claim a new IDM timing comparison, install an update, or adopt the rejected metadata write-through prototype. Installed app 8.68 and extension files 0.72.32 remain unchanged.

## Reproduced failure

The same real HTTP fixture links against frozen 9.67 core objects and current 9.68 objects. It calls the actual ordinary transfer directly, exercising the publication path without a prepared-worker handoff. At preflight begin or preflight ready it either leaves the created stage unchanged, moves it and writes an equal-byte replacement file, or moves it and creates an empty directory at its old name. It then throws a controlled publication error.

The cross-volume form keeps resumable data on C: and output on D:. The fixture verifies actual filesystem equivalence between stage and received data, rather than assuming the intended path selected a hard link or a copy. Frozen 9.67 passes 28 of 36 assertions but deletes the unrelated replacement in all eight substitution cases. With 9.68 all 36 pass: unchanged owned stages are removed, foreign files/directories survive, moved original bytes remain, received data is retained and no final file is published after the error.

The first probe tried to read the usual StageMode timing field, which is populated after direct publication returns. It is empty at an interrupted preflight, so that mode oracle was invalid. Its results are retained separately; the accepted before/after controls use the same corrected fixture bytes and physical file-identity test. The successful-copy benchmark initially omitted its StreamHash include; that compile log and the corrected input are also retained. Neither correction changes production code.

## Implementation and ordering

`CreatedStagingFile` captures volume, full 128-bit file ID and creation time from the exclusive CREATE_NEW handle before copy/assembly writes begin. Its fixed-size snapshot makes no JSON allocation. Cleanup deliberately excludes mutable length and last-write time from this ownership comparison: a file created empty must remain identifiable after it grows. A reliable identity is required before cleanup can delete anything.

Copy and assembly creation requests attribute-read access on that existing writer handle. On failed cleanup the named item is opened with delete/attribute access, denying concurrent writers and deleters; it must still be a regular, non-reparse object with that captured identity. Deletion is requested on the same checked handle. Missing proof, identity mismatch, sharing conflict or a failed operation retains the item. Microsoft documents comparison of volume and file ID to identify a file, and exposes creation time and attributes through basic file information. [FILE_ID_INFO](https://learn.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-file_id_info), [FILE_BASIC_INFO](https://learn.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-file_basic_info).

Hard links use the existing verified-link helper against the current partial data, only during failure cleanup. They add no creation-time query on the healthy prepared path. If an error occurs before handoff while an asynchronous final flush exists, the transfer's local guard joins that work and releases its write pin before ownership cleanup. The Manager worker still owns the job throughout this unwind, preserving the 9.64 failure-retirement ordering. The prepared-owner protection from 9.65 remains unchanged.

Successful direct publication now marks the stage delegated immediately after publishFile returns, so later cleanup cannot reinterpret its old name. Final body flush, journal durability, source validation and exact catalog acceptance are unchanged. Copy/assembly capture adds metadata queries on those paths; no measured zero-cost or speed claim is made.

## Validation and evidence

- Five native products have version 9.68 and retain their build hashes.
- Seventeen affected suites pass 1,351 assertions; this includes eighteen hard-link interruption assertions in the new registered native fixture.
- The additional cross-volume invocation passes 36 assertions, including repeated link controls and eighteen actual copy-path assertions. It is kept separate from the registered-suite total.
- A real 128 KiB C:-to-D: download verifies final byte count, SHA-256, actual copied-stage mode, absence of a stale stage, and the saved Complete/digest record (four assertions).
- All 686 captured native/build inputs and the separate success-benchmark input are verified. Eleven installed/personal/helper files are checked against their pre-investigation hashes.

[Build receipt](validation/transfer-stage-cleanup-9.68/build/build-result.json), [all affected suites](validation/transfer-stage-cleanup-9.68/build/suite-results.json), [same-source controls](validation/transfer-stage-cleanup-9.68/controls.json), [before failure](validation/transfer-stage-cleanup-9.68/before-967-accepted/negative-control.json), [cross-volume results](validation/transfer-stage-cleanup-9.68/cross-volume/results.json), [successful copy](validation/transfer-stage-cleanup-9.68/copy-success/results.json), [final audit and cleanup](validation/transfer-stage-cleanup-9.68/final-audit.json).

## Remaining work

This covers ordinary transfer stages, not every temporary file in the product. Compressed-response decoding/stripping and media staging still need their own ownership review, as does recovery of an unjournaled copied stage after process loss. Unsupported identity queries conservatively retain temporary files; no unsupported-filesystem, power-loss or hostile-kernel claim follows.

There is no new GUI/browser validation or successful-download timing comparison with IDM. Sequential/split finishing and exclusive first-availability gates remain unresolved. Broad IDM parity remains unproven.

Generated EXE/OBJ and selected test payload files were removed individually after hash verification and evidence export, without recursive deletion. The five products, 42 referenced core objects, sanitized evidence and small private records/directories remain. The final audit records exact removed counts and logical byte totals; physical free space was not measured.
