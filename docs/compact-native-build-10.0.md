# Compact native builds — 10.0

Native source and all five compiled programs are **10.0**; both browser source manifests are **0.73.64**. This increment changes the development build workflow. The desktop, transfer and browser implementations remain those of 9.99. It does not establish another runtime speed improvement or IDM parity. No installer, installation or release is included.

## Why this work precedes further comparisons

The retained 9.99 build has approximately 1,092 MiB of object files, including separately retained UI fixture translation units. Current storage checks found about 0.67 GiB free on C: and 2.07 GiB on D: before this work. The lab remains unbooted because its expanding virtual disk cannot safely fit that reserve. Desktop window listing works, but launching IDM through the supported control connection fails with `GetCursorPos: Access is denied` after a refreshed retry. No alternate desktop input route or administrator request is used.

The earlier cleanup rejection remains in effect. This increment preserves those files and evidence; it reduces the intermediate retention needed by subsequent builds.

## Build behavior

Add `-CompactIntermediateObjects -OmitDebugSymbols` to the normal native build command. The option requires at least one linked target and rejects debug-symbol and core-only combinations before creating output directories. Ordinary builds keep their previous target-object layout and perform no compact bookkeeping.

A compact build holds an exclusive lock for its output root. Sequential target compilations reuse one scratch object, while finished programs, shared core objects, generated automation files and special diagnostic core variants remain separate. This is the lifetime of a new build's temporary object; it does not overwrite an earlier qualified build.

The target record starts incomplete. After each successful link, it captures the object size and SHA-256 before reuse, along with the executable digest. Only after all selected targets link does the record become successful. This state covers the target compilation/link phase, not tests, asset packaging, installation or release. The record describes consumed inputs; earlier scratch-object contents are not retained. Hashing uses .NET without requiring the `Get-FileHash` cmdlet. Success and exception paths release the lock.

The last of seven compact target objects occupies **2,688,052 bytes**, against **87,790,088 bytes** summed across their recorded consumed objects. The largest consumed object is 68,262,098 bytes. Those figures describe retention, not measured physical space reclaimed. The entire development area, including failed attempts and comparison controls, was approximately 153.4 MiB at verification. No FFmpeg or compiler distribution is copied by the validation driver, and no old test files are deleted.

## Verification and limits

The validation driver executes the actual selected statements from `native/build.ps1` with retained compiler configuration, resource generation and 42 shared core objects. It binds the source-root context explicitly. It does not execute the entire public build entrypoint's compiler discovery, MIDL setup or asset-packaging stages. The two invalid-option checks do enter the full script and confirm rejection before output creation.

All five native programs link and their PE product versions read 10.0. Source/hash verification covers 734 inputs, 42 core objects and 11 qualified outputs. Only the version-bearing Bridge core object changes from 9.99. Nine protected installed-file hashes remain unchanged.

Two backend suites contain **31 distinct assertions**. Each passes through the retained old normal loop, the current compact loop and the current normal loop. These six executions are not 93 different assertions. A live lock check verifies that a second writer is rejected while the current build holds the slot. A deliberate missing-source compilation after one successful target leaves the record incomplete, produces no executable for the failed target and releases its lock. The current normal branch retains two separate objects and produces no compact metadata.

Earlier failures remain distinct. The first driver did not bind its source-root context, so no test executable was produced. The second attempt linked a compact target but stopped when its PowerShell environment did not provide `Get-FileHash`; its incomplete record and logs remain. That attempt's failure control stopped at the same helper and is not counted as the intended negative test. The final attempt uses direct .NET hashing and fresh output roots.

[Sanitized records, source snapshots and qualification scope](validation/compact-native-build-10.0/verification.json).

## Remaining IDM work

The 9.99 folder-handoff qualification remains separate: [127 backend and 1,105 UI assertions, plus two frozen-source controls](validation/shell-folder-9.99/verification.json). Those results are not relabeled as a fresh 10.0 UI run. Real Explorer paint, installed browser/media workflows, remaining file/app-chooser/address/icon operations and storage-finishing comparisons remain open.

The retained feature inventory's F088 language gap is still visible in the current native source: the Language menu contains only English, and its command does nothing. This is a concrete remaining feature, not a completed localization system. The public parity review records its current source digest and locations. Broader controls, keyboard behavior, DPI, accessibility, metadata and media coverage still require their own evidence. No blanket completion percentage is assigned.
