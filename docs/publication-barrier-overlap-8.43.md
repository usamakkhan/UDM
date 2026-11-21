# UDM 8.43: overlap independent publication durability barriers

Status update: 8.44 removes this experiment from the normal path. Its fifteen binaries, four core objects and resource were retired after the [source reconstruction and replacement qualification](serial-publication-8.44.md). The records below describe the earlier candidate's observed state; the archive preserves its source, not the compiled artifacts.

The 8.42 D: sequential 64 MiB observations identified the body, intent and catalog persistence paths as the dominant measured finish costs. Version 8.43 overlaps body writeback with publication-intent preparation for the existing prepared ordinary-transfer path. Its eligibility remains the established non-prefetch, fixed-volume, non-sparse, same-volume hard-link path up to 64 MiB, without POST/FTP, prefix stripping or replacement. This is a protocol change, not another early-writeback pacing adjustment.

The source body is pinned with read/write access and denies other writers. One joined task forces its body flush while the publication worker writes and flushes the intent. The worker must obtain both successful outcomes, recheck cancellation and model destination inputs, validate the staged file and pinned directories, and only then rename. Catalog completion still requires its snapshot-bound durable proof. A separate read handle pins the published name through catalog acceptance; together, the two handles deny external writes and renames without dropping ownership to reacquire a read-only handle.

Every early intent explicitly records `FlushBeforePublish: true`. Recovery opens the preserved file for pinned writeback, validates its measured length/identity/digest, flushes its bytes, then follows the existing rename and catalog-acceptance ordering. A refused write handle, changed/truncated bytes, unrecognized staging path or uncertain rename must retain the intent rather than fall back to old completed-byte accounting. For a renamed staging file, recovery keeps both its write-owning handle and its destination-name pin through acceptance. Legacy intents retain their prior behavior.

Automatic checkpoint coalescing remains active for the exact full-body finalization owner until publication resolves. Explicit saves, pending admission acknowledgments and publication proof captures keep their real persistence operations. Shutdown/cancellation waits for the joined body task; no worker, future or file handle detaches.

The barrier fixture independently holds body IO, intent completion and catalog acceptance; checks model availability, forced/automatic saves and writer/name exclusion; injects body/intent failures, cancellation and changed destinations/source names; verifies changed/truncated/foreign recovery sources; and terminates an owned subprocess after a durable intent while its body flush is still held before the syscall. Startup then verifies and flushes the preserved source before completing, with a second catalog reopen. This is process-interruption coverage, not a physical power-loss test.

All 1,473 focused checks pass across ten suites, and five native products plus ten focused fixtures are built. The candidate is **experimental and withheld from installation**: the two alternating D: 64 MiB trials did not demonstrate a speedup. Both extension source manifests advance to 0.72.7 alongside native 8.43; browser JavaScript is unchanged. No installer, installation or publishing is included. The installed desktop remains 8.32. IDM comparison still requires its existing instance to close normally. A green fixture does not establish speed parity or identify kernel/disk/filter root causes. Retained losing timing rows remain part of the evidence.


## Qualification and timing decision

[The native receipt](validation/publication-barrier-overlap-8.43.json) records exact source/core/product hashes, the 1,473 checks and the failed prototype observations that drove the refused-write and corrupt-source fixes. The barrier fixture contributes 138 checks, including explicit gate-release verification; no deadline expiry counts as success. Two initial prototype failures are retained separately from final passing qualification.

The four uninstrumented sequential 64 MiB transfers alternate version order: 8.42 then 8.43, followed by 8.43 then 8.42. Each used an isolated D: catalog/output folder, one connection, the same deterministic loopback payload and 8,000,000-byte/s cap. All four used exactly one origin request, matched the independent SHA-256 and normal process exit, and removed their final payload after verification. Both 8.43 records confirm that the overlap path actually ran. A symmetric explicit output-file flush after process exit was measured separately from the original application-exit metric.

| Version | Trial 1 exit tail | Trial 2 exit tail | Median exit tail | Median post-exit durability tail |
| --- | ---: | ---: | ---: | ---: |
| 8.42 | 244.230 ms | 288.961 ms | 266.595 ms | 293.097 ms |
| 8.43 | 263.306 ms | 276.327 ms | 269.816 ms | 297.584 ms |

The candidate loses the first paired trial and wins the second. Its medians are about 3.2 ms and 4.5 ms higher respectively. Two trials on an uncontrolled desktop do not establish a statistically reliable regression or benefit; they fail to demonstrate the intended speedup. These values include application-owned worker/shutdown work, not a kernel trace of external services or device internals. They are a UDM-version comparison, **not a new IDM comparison**. No slower result was removed. [All four rows and timing scope](validation/publication-barrier-overlap-8.43/sequential-64m-comparison.json).

Do not promote this candidate as a speed upgrade on this evidence. The next step is a fresh actual IDM persistence/process-lifetime comparison and investigation of the remaining barriers, retaining durability and restart proof. The existing IDM and older owned UDM observation app remain open; normal close is pending. Sky input failed twice with `GetCursorPos: Access is denied`; the alternate desktop provider also failed with `failed to write kernel assets`. No process was force-terminated to clear that blocker. Installation remains deferred and the installed desktop is still 8.32.

## Cleanup

After qualification, individually removed 2,212 generated fixture files, redundant target objects and obsolete prototype fixture executables (141,899,973 logical bytes). Three multi-link file groups were removed only after verifying that every link was an owned path in the deletion plan and the complete known group equaled the link count. Four final 64 MiB comparison payloads were separately verified/deleted (268,435,456 logical bytes). Canonical and prior qualified core/products, all fifteen candidate binaries, four new core objects, resources, compact results and personal catalogs remain. Logical bytes are not physical reclaimed allocation. [Cleanup evidence](validation/publication-barrier-overlap-8.43/cleanup.json).
