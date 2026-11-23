# Exclusive staging and verified link reuse: UDM 9.67

UDM 9.67 prevents an ordinary transfer from silently deleting or overwriting a pre-existing staging item, while preserving recovery of a stage hard-linked to its current partial file. Both source extension manifests are 0.73.31. The first exclusive-creation increment was 9.66 / 0.73.30; it was superseded before a full desktop build or installation because it needed verified owned-link reuse.

## Confirmed failures

A real loopback HTTP worker control on frozen 9.65 seeds a file or empty directory at the expected staging name. A valid body causes the old code to remove that item and publish over its name. A checksum-rejected body can also remove the pre-existing item even though this transfer has not created a stage. The fixture verifies status, preserved foreign content and retained received bytes. All four content-preservation cases fail before the change.

Exclusive creation in 9.66 passes the twelve foreign-item assertions, but a separate corrected resume fixture shows why blanket refusal is incomplete: a valid saved partial file with an ETag, range plan and matching staging hard link cannot finish. The corrected same-source fixture fails its three owned-link expectations on 9.66 and passes all six owned/foreign assertions on 9.67. An equal-byte different file remains foreign and is preserved.

The original reuse fixture seeded bytes without valid resume metadata. HTTP correctly reset that partial-file identity, so it did not prove owned-link recovery. Its failures on both candidates are retained as superseded diagnostics. The corrected fixture implements range responses and a strong ETag, with a half-complete saved segment. No production code was changed to accommodate the invalid fixture. The initial full 9.67 build completed its five products, then stopped at that old test; only the corrected test was recompiled before final qualification. The receipt preserves the initial sources, failed test results and explicit fixture-only source correction. All production source inputs stayed unchanged through that correction.

## Implementation

The ordinary transfer's staging guard starts unclaimed. It may remove a temporary stage only after this attempt actually created one; an early hash failure or failed creation no longer grants ownership of an arbitrary pathname. New assembled/copy outputs use `CREATE_NEW`. The unconditional pre-link pathname deletion is gone.

For the usual hard-link path, UDM first attempts creation. Only if creation fails does the recovery helper inspect the existing stage and current partial file. Both must be regular, non-reparse files with identical stable volume/file identity, size and timestamps. While handles exclude other writers/deleters, it requests deletion through the checked stage handle. The handles close before creation is retried. If proof or deletion fails, exclusive creation cannot overwrite the occupied name. A legitimate leftover link can be recreated and published while an unrelated equal-byte file stays intact.

The healthy hard-link path adds no identity query or recovery handle: the helper runs only after failed creation. It also avoids the previous unconditional deletion attempt. This is a statement about the code path, not a measured timing result. Body writeback, durable publication intent, source validation and exact catalog acceptance stay unchanged.

Windows documents `CREATE_NEW` as failing if the file exists, and requires DELETE access for handle-based disposition. Those APIs support the ownership protocol; they do not establish power-loss or performance parity by themselves. [CreateFileW](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew), [SetFileInformationByHandle](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-setfileinformationbyhandle).

## Scope and remaining work

This covers ordinary staging admission, unclaimed cleanup and reuse of a hard link to current partial data. It does not establish safe ownership of every temporary file: cleanup after creating a stage but before handoff, compressed-response staging, media staging, and recovery of an unjournaled cross-volume copied stage still need their own ownership audit. The prepared-owner cleanup protection in 9.65 remains in place. No broad safety or IDM parity completion follows from these tests.

There is no new successful-download timing comparison with IDM. Sequential/split finishing and exclusive first-availability gates remain open. The aligned metadata write-through prototype remains unadmitted. Installed app 8.68, extension files 0.72.32, personal catalogs and shared helpers remain outside these changes.

Five native 9.67 products built successfully. All 1,333 assertions in sixteen affected suites passed on D:, with eighteen additional staging assertions on C:. The earlier prechecks and invalid-fixture attempts are retained separately and are not added to those totals. All 684 current inputs, five retained products, 42 referenced core objects and eleven protected installed/personal/helper files are verified.

[Build receipt](validation/staging-admission-9.67/build/build-result.json), [D-volume suites](validation/staging-admission-9.67/build/suite-results.json), [C-volume checks](validation/staging-admission-9.67/build/c-drive-results.json), [same-source controls](validation/staging-admission-9.67/controls.json), [explicit fixture correction](validation/staging-admission-9.67/build/source-correction.json), [final audit and cleanup](validation/staging-admission-9.67/final-audit.json).

Cleanup used individually recorded, hash-verified files, with no recursive deletion: 63 disposable binaries/objects and 235 selected test payloads were removed. A separate narrow follow-up also removed 222 older generated binary/payload files that had remained after the previous broad-cleanup rejection. Combined logical length removed is 369,470,799 bytes (about 352 MiB); physical free space was not measured. Products, referenced core objects, sanitized evidence and small private records/directories remain.
