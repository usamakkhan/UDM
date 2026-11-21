# Prepared-stage cleanup ownership: UDM 9.65

UDM 9.65 protects a different file or directory that appears at a prepared download's old staging name after handoff. Before this fix, an ordinary publication failure could join body writeback correctly but then remove that pathname without proving it still referred to the received file. The same component fixture and real HTTP-worker fixture reproduce loss on frozen 9.64.

Both source extension manifests advance to 0.73.29 under the requested paired-version policy. This is a native failure-cleanup fix. It does not adopt the rejected aligned metadata writer, change successful publication barriers, install an update or claim IDM speed parity.

## Reproduction

The component fixture creates a prepared owner, with and without actual asynchronous body writeback. It leaves the stage unchanged, renames it away, replaces its old name with an equal-byte different file, or places an empty directory there. The old destructor removes the equal-byte replacement and the unrelated empty directory. Equality of the payload is deliberately insufficient: file ownership, rather than a digest match alone, determines what may be deleted.

The real HTTP fixture downloads 128 KiB through Manager's ordinary worker and substitutes a file or empty directory at `publication-preflight-begin` and `publication-preflight-ready`, then throws an owned error before durable intent publication. Frozen 9.64 retains the moved source bytes and reports failure, but deletes the foreign staging name in all four cases. The accepted negative control uses nine hash-verified frozen 9.64 headers and the corresponding frozen core objects, so the client's class definitions match its worker implementation. The after-control uses the same fixture source bytes against 9.65. An earlier mixed-header diagnostic is retained separately and is superseded by this fully frozen control.

## Implementation

The prepared owner captures the staging identity at handoff. When an asynchronous pin exists, it copies the identity already captured before that file object's flush starts. This adds no source metadata query during the active flush. The serial fallback captures identity before taking Manager's model lock; lack of a reliable identity prevents deletion rather than preventing publication.

On unsuccessful destruction, the owner first joins its body task and retains the existing recovery-journal check. Only when no recovery needs the stage does it release its owned write pin, open the staging name with delete/attribute access while excluding other writers and deleters, and compare the full stable identity. Deletion is requested on that same checked handle. No subsequent pathname deletion can select a replacement name between the check and deletion. A directory, reparse point, changed identity, sharing conflict or failed deletion is retained. The Windows API requires DELETE access for this disposition operation and applies deletion when the handle closes. [SetFileInformationByHandle](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-setfileinformationbyhandle), [FILE_DISPOSITION_INFO](https://learn.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-file_disposition_info).

The successful asynchronous publication path reuses its existing identity snapshot; it does not gain a new handle open, hash pass, disk flush or metadata query for this cleanup proof. The fallback and failure paths do gain identity work. No new timing benchmark has established a speed improvement or no-regression claim.

## Scope and remaining gaps

This fixes cleanup after a successful prepared handoff. Transfer-level cleanup before handoff, media staging, and reuse of an already occupied staging name are separate paths and still need an ownership audit. It does not establish that every temporary-file cleanup path is protected. Existing final body flush, intent durability, exact catalog acceptance and failed-worker retirement requirements remain in force.

The prior metadata write-through prototype remains unadmitted because it lost two finishing pairs. The normal 9.63 sequential/split and IDM first-availability gaps remain unresolved; 9.64 and 9.65 do not have a new successful-download IDM comparison. These correctness regressions cannot prove broad UI, browser or performance parity.

Five native products built at 9.65. All 1,315 assertions in fourteen affected suites passed; the earlier 26 precheck assertions are retained but not double-counted. All 682 captured inputs, 42 core objects and eleven protected installed/personal/helper files were verified. Installed app 8.68 and extension files 0.72.32 remain unchanged.

[Build receipt](validation/prepared-cleanup-owner-9.65/build/build-result.json), [affected suites](validation/prepared-cleanup-owner-9.65/build/suite-results.json), [same-fixture controls](validation/prepared-cleanup-owner-9.65/controls.json), [frozen 9.64 HTTP failure](validation/prepared-cleanup-owner-9.65/http-before-frozen/negative-control.json), [corrected HTTP worker](validation/prepared-cleanup-owner-9.65/suites/PreparedCleanupHttpTests.json), [final audit](validation/prepared-cleanup-owner-9.65/final-audit.json).

The broad generated-test-file cleanup was blocked by automatic approval review ("blocked by policy"). A narrower cleanup removed 33 individually verified compiler objects and test executables from the new build, totalling 187,872,111 logical bytes (about 179 MiB); product executables and referenced core objects remain. The earlier metadata experiment also cleared sixteen generated binaries/objects totalling about 166 MiB. Small test files, records, frozen negative-control material and directories remain; the final audit records their exact remaining size. No recursive removal or personal-file deletion succeeded or was attempted outside the recorded owned roots.
