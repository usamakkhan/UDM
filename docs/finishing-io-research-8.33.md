# Finishing IO research after 8.33

Research date: 2026-11-15. Installation is deliberately deferred at the user's request. Installed UDM remains 8.32; qualified 8.33 and browser manifests 0.71.97 are unchanged. No new performance run, test payload, elevation request or product change was made for this research.

## What the saved runs actually establish

The [nine-row wait audit](validation/finishing-8.33/finish-wait-audit.json) identifies final output flushing as a substantial measured wait: median 123.053 ms, maximum 800.791 ms. Output opening never exceeded 0.160 ms in these samples. This argues against another reopen optimization as the next intervention. It does not identify the kernel component responsible for the flush wait.

A second audit reads the original retained catalogs, not just the public timing prefixes. Each catalog hash and relevant source hash is retained in the [join audit](validation/finishing-8.33/prefinish-join-audit.json).

The losing 4 MiB trial has these ordered milestones:

| Interval | Saved delta | Interpretation |
| --- | ---: | --- |
| Last credited byte to all download workers joined | 47 ms | Worker/network completion and scheduling; not an isolated flush measurement |
| Workers joined to HTTP boundary verification ended | 109 ms | Includes early-flush worker join, its handle close and bookkeeping; zero boundary checks in this sequential sample |
| Boundary verification ended to prefix hasher stopped | 0 ms | Below the timer's resolution |
| Later final file flush | 53.538 ms | Separately measured synchronous call |
| Later publication-intent helper flush/close boundary | 112.232 ms | Separate publication transaction; not the same file or an isolated device-service measurement |

`Transfer.cpp` joins the network workers, calls `PrefinishWriteback::finish()`, closes its handle, verifies boundaries, then joins the prefix hasher. `finish()` stops admitting requests but joins an already running flush. Therefore the source and milestones expose an additional pre-finalization wait site. They do **not** prove all 109 ms was inside the flush syscall. The other eight samples have a zero worker-join-to-boundary-end delta at the saved tick resolution.

The cumulative 2.385-second early flush in the slow split sample had already completed when the writers joined. Adding it to the 800.791-ms final flush as though both were post-network tail would be wrong. Likewise, the 2.872-second cumulative early writeback in sequential trial 2 did not become a measurable post-worker join wait.

These milestones use `GetTickCount64`, whose typical resolution is 10–16 ms. Zero means unresolved below that granularity. [Microsoft timer documentation](https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-gettickcount64)

## Storage topology changes the scope of the claims

All nine current portable outputs and their isolated parts/catalog folders are on **C:**. The [read-only topology receipt](validation/finishing-8.33/storage-topology-research.json) resolves C: to disk 0, Kingston SKC600/256G, and D: to disk 1, WDC WD2500AAKX-753CA1. D: is an NTFS dynamic volume; the ordinary `Get-Partition -DriveLetter D` lookup did not enumerate it, while the Win32 logical-disk association did.

C: has 4 KiB allocation units; D: has 128 KiB allocation units. These are filesystem allocation units, **not** physical-sector alignment values for unbuffered IO. The point-in-time free-space observation is also retained. It does not prove SSD garbage collection, disk failure or the cause of a historical stall.

Consequently the current portable results cannot establish the outcome on D:. Separate disks also do not rule out shared controllers, filters or unrelated background activity. A future D: comparison must explicitly record where both the output and parts reside and whether publication copies across volumes.

## Windows contracts constrain the possible fixes

`FlushFileBuffers` forces the specified file's buffered information to its device. Microsoft warns that frequent explicit flushes can be inefficient because of cache interactions. This supports examining redundant barriers, but is not evidence that removing the final barrier preserves UDM's recovery contract. [API documentation](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-flushfilebuffers)

Replacing that call with `FlushViewOfFile` is not equivalent: flushing a mapped range does not flush file metadata or guarantee completion through the underlying hardware cache. Microsoft's documented durable sequence also uses `FlushFileBuffers`. IDM's mapping-related imports alone do not establish its actual download writer or durability sequence. [Mapped-view flushing](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-flushviewoffile)

`FILE_FLAG_NO_BUFFERING` is a different writer design, not a flag to add to the existing arbitrary HTTP chunks. Windows requires sector-multiple offsets and lengths and suitably aligned memory. Range splits, retry overlaps, resumed extents, short final blocks and cancellation need ownership-aware assembly and recovery. Hardware caching remains a separate concern. [File-buffering requirements](https://learn.microsoft.com/en-us/windows/win32/fileio/file-buffering)

An early-flush timeout cannot safely detach its thread and destroy the handle or callback owner. Requesting IO cancellation is not proof of completion; driver support and races still matter. The current worker must remain owned until completion is observed. [Windows cancellation guidance](https://learn.microsoft.com/en-us/windows/win32/fileio/canceling-pending-i-o-operations)

## Next work, in evidence order

1. Keep the 8.33 automatic-catalog coalescing fix isolated. Do not mix another lead-byte threshold adjustment into its installed comparison. Installation remains deferred by the user.
2. For the next owned diagnostic build, bracket early-flush join, handle close, boundary verification and final catalog commit with high-resolution, thread-tagged events. Existing tick prefixes cannot separate those spans. Preserve macro-gated diagnostics and production no-op behavior; do not introduce a new shipping log.
3. Capture one bounded paired IO recording using the retained owned-instance recorder. Check event loss, process identity, file identity, disk number and clock alignment before interpreting it. Raw ETL stays private and is removed after verified decoding. The recorder's fault tests do not constitute a live trace.
4. Use the following decision gates to choose a product change. Do not infer a kernel cause from syscall wall time alone.

| Observation needed | What it would establish | Change justified only after that observation |
| --- | --- | --- |
| Early-flush thread still inside its call after all body writers finish | Optional in-flight flush extends the serial finishing path | Redesign ownership/barrier coalescing; preserve a barrier ordered after the last successful write and metadata changes |
| Own catalog/intent IO overlaps the critical output flush | Application-owned contention remains | Remove or coalesce only redundant checkpoints; retain admission, publication, forced-save and error-retry acknowledgements |
| Disk requests spend most of their elapsed time queued | Device contention rather than just slow reopen or hashing | Identify exact competing IO and its owner; change UDM's own unnecessary IO if present |
| Disk service itself accounts for the long flush | Storage service dominates that sample | Evaluate a different write architecture with identical final durability, recovery and full-lifetime endpoints |
| Flush thread is ready but not running for a material interval | Scheduling contributes to the measured call | Find the responsible CPU/thread activity; changing flush milestones would not address that evidence |
| Long file-operation span without a matching device-service span | Filesystem/filter or unobserved work remains possible | Obtain supporting stack/provider evidence; do not label antivirus or a filter as the cause without it |

WPA distinguishes IO elapsed time from disk service time; queueing can make the former larger. A file-flush interval and a disk-flush request are not automatically a one-to-one pair. [Microsoft disk-analysis walkthrough](https://learn.microsoft.com/en-us/windows-hardware/test/wpt/optimizing-performance-and-responsiveness-exercise-2)

5. Verify any resulting product change with failure/recovery invariants and actual paired installed runs. Retain every losing pair, final size/hash, saved Complete digest, process-exit endpoint and the separate symmetric post-exit output flush. Include a declared D: case before claiming the earlier D: delay is resolved. Completion-dialog timing remains a distinct GUI requirement.

## Current limits

REA's previous Ghidra session timed out before returning call sites. No proprietary IDM code was copied and no IDM runtime sequence was inferred from that attempt. No privileged kernel trace or new installed comparison has been completed. This document advances attribution and the implementation gates; it does not claim a new speedup or a completed parity goal.

## Static disassembly follow-up

The existing Microsoft DUMPBIN completed a separate read-only disassembly of the exact audited IDM executable. A PE32 import-slot parser tied API names to preferred import addresses; direct indirect-call encodings were then checked against the original executable bytes. This avoids treating a text-name search or an import alone as a call-site observation. [Verified API-reference facts](validation/finishing-8.33/idm-static-io-audit.json)

The listing contains four byte-verified direct calls through the `FlushFileBuffers` import slot, seven through `RegFlushKey`, 44 through `WriteFile`, and 16 each through `CreateFileMappingA` and `MapViewOfFile`. Additional register-load references are recorded separately. These counts describe static instructions, not execution frequency, file identities or download-path reachability. No normal `FlushViewOfFile` import was present; dynamic resolution and dependency behavior remain unexamined.

Two additional relative-call edges lead from a conditional caller through a small wrapper to one of the file-flush calls. This establishes a concrete candidate path for later runtime correlation. The controlling field has no verified meaning, so it is not described as a timer, byte threshold or download-completion condition. Other flush paths may belong to libraries or unrelated operations. Neither this chain nor the mapping references establish IDM's writer architecture or justify changing UDM's durability barriers.

DUMPBIN disassembles code sections; its listing does not establish runtime reachability. The byte cross-check confirms the reported instruction encoding, not an observed execution. [Microsoft DUMPBIN documentation](https://learn.microsoft.com/en-us/cpp/build/reference/disasm)

Removed the 32 MB broad listing and three other temporary inspection files after retaining compact selected evidence privately. The [cleanup receipt](validation/finishing-8.33/idm-disassembly-cleanup.json) records four files and 32,157,374 logical bytes removed. IDM's original hash is unchanged. No process was launched from the target, no product source or version changed, and no installation or speed comparison was performed.

## High-resolution join attribution

Completed a separate private diagnostic build from qualified 8.33. Two temporary instrumented translation units add QPC markers; Core, Transfer, Duplicates and App compile with the existing diagnostic macros, while 39 other core objects are reused. No production source, canonical core object, qualified binary, pacing rule or installed file changed. The temporary [marker patch](validation/finishing-8.33/join-diagnostic-markers.patch) and [build receipt](validation/finishing-8.33/join-diagnostic-build.json) identify the exact diagnostic variant.

After compilation stopped, ran one paired trial for each original C: case. All six final sizes and hashes matched, all UDM saved Complete digests matched, and IDM settings were restored. Three 2,048-capacity traces decoded with zero dropped or incomplete events. [Full attribution evidence](validation/finishing-8.33/join-attribution.json)

| Diagnostic case | Early-flush join | Early handle close | Prefix-hash join | Output-writeback bracket | Intent write | Catalog write |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 4 MiB sequential | 33.568 ms | 0.176 ms | 0.000 ms | 65.261 ms | 124.550 ms | 81.408 ms |
| 64 MiB sequential | 0.157 ms | 115.045 ms | 61.931 ms | 94.736 ms | 374.136 ms | 286.658 ms |
| 64 MiB split | 0.087 ms | 0.065 ms | 0.000 ms | 151.688 ms | 3.917 ms | 17.078 ms |

These are wall-time brackets, not isolated device service times. The output-writeback bracket includes open, optional normalization, flush and close. The intent and catalog brackets include their complete helper operations. The early-flush call can overlap network work and is not added as another serial tail. The exact sample intervals, threads and original result counters remain in the evidence.

The 4 MiB early-flush join overlaps its still-running optional flush, supporting the join-wait hypothesis for this new sample. In the sequential 64 MiB run the join itself is short; handle close and prefix-hash termination are separate material waits. Intent and catalog writes account for much more of that sample's finishing path than reopening the output. The split sample instead spends most of its measured IO bracket in output writeback. No automatic-checkpoint deferral marker occurred in these three critical paths, so the run does not prove that coalescing was exercised there.

Original process-exit tail outcomes were UDM/IDM 639.845/507.703 ms for 4 MiB, 1169.087/548.381 ms for sequential 64 MiB, and 324.464/462.722 ms for split 64 MiB. Both losing diagnostic pairs remain recorded. This is an instrumented one-trial attribution run, not a new shipping qualification or a replacement for any earlier losing result. Trace mapping, scheduling and storage conditions are confounds. Installed performance and D: remain unverified.

The source audit also identifies a concrete next implementation candidate: fresh large sequential responses currently use a background prefix reader, while small fresh responses hash their successful write buffers inline. The observed 61.931-ms prefix-join wait justifies evaluating inline write-buffer hashing for **provably single-writer fresh sequential** responses. That change must preserve resume/multiwriter exclusions, arbitrary chunk boundaries, server/user digest verification and disk fallback on invalid coverage. It could remove redundant read work; it cannot by itself eliminate the much larger journal/catalog barriers. This candidate is not implemented or claimed as a speedup here.

After verified decoding and hash checks, individually removed the six payloads, three trace binaries, four diagnostic objects, two temporary source files, executable and owned fixture marker: **17 files / 383,087,330 logical bytes**. Canonical 8.33 source/core/binary hashes were rechecked after cleanup. [Cleanup receipt](validation/finishing-8.33/join-diagnostic-cleanup.json)

## Buffered-open follow-up during 8.39 research

The 64 MiB recorder request was canceled by Windows before either recording or its workload started. No owned recorder was observed running on this follow-up. No additional elevation request, installation or transfer workload was started.

REA 6.5.0 passed its configured MCP doctor check with all 207 expected tools. This proves transport and catalog health, not deep-analysis success. The earlier Ghidra startup timeout remains unresolved; the expensive analysis was not repeated.

Bounded Microsoft DUMPBIN ranges and independent original-byte checks identify two `CreateFileW` call sites at preferred addresses `0x57033f` and `0x57048d`. Both pass read/write access, read sharing, `OPEN_ALWAYS`, zero flags and a null template. Both store the returned handle at object offset `0x14`; the previously observed flush wrapper reads that offset. These two opens therefore do not request unbuffered, write-through or overlapped IO. The object layout does **not** prove identical class identity, the same runtime object or use by a benchmark download. [Compact audit](validation/finishing-8.39/idm-buffered-open-audit.json)

Microsoft documents buffering as the default and treats unbuffered IO and write-through as separate choices. Changing UDM to unbuffered writes is not justified by these observed IDM call sites. Other IDM paths remain unexamined. [CreateFileW contract](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew), [FlushFileBuffers contract](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-flushfilebuffers)

The next decisive evidence remains runtime attribution: correlate the actual transfer file and handle with writer flags, flush entry/exit, disk requests and outstanding background IO. Static proximity cannot identify which Windows component accounts for the measured final-flush wait. No durability barrier, pacing, product version or installed file changed for this audit; no payload, broad disassembly export or additional binary copy was created.

## Completed REA analysis and user-mode observer follow-up

REA 6.5.0 completed the previously timed-out Ghidra analysis with a 900-second startup deadline. Five read-only evidence records independently decode the known flush instruction, three selected procedures and the flush wrapper's direct callers. The temporary provider project closed normally. Decompiled target text remains private; no proprietary implementation was copied into UDM.

A nonce-filtered user-mode observer now captures file opens, writes, mappings, flushes and closes with QPC timestamps. An owned 256 KiB x64 fixture verified all eight expected events, zero drops, closed handles, correct final bytes/hash and prompt normal detach. Two earlier teardown failures are retained: intercepting an unrelated pending ReadFile prevented prompt unload; removing that hook corrected the fixture. Actual IDM x86 and UDM download observation remain unverified. This is diagnostic preparation, not a performance improvement or parity result.

[Sanitized evidence and cleanup](validation/finishing-8.39/rea-and-user-io-research.json). The single private Frida runtime is retained for further observation; generated fixture payloads, executable and object were individually removed after boundary/link/hash checks. No functional app change, version increment, installation, release or new administrator prompt was included.

## Actual IDM x86 observation: writer coverage rejected

The nonce-filtered observer attached to the existing current-session 32-bit IDM. One isolated 4 MiB loopback download completed with the independently expected size and SHA-256; the observer detached, its owned payload was deleted, and both UDM personal catalogs plus the installed executable retained their hashes. IDM was not terminated or reconfigured.

Ten matching events covered five file opens and five closes, with zero dropped events and zero remaining tracked file handles. No body-write or flush call was captured. Therefore writer coverage fails: this observation cannot establish IDM's write path, flush absence or flush cadence. The next probe must observe native writes/flushes and resolve actual handles, covering temporary files and copy paths. These instrumented observations do not measure uninstrumented speed, process exit or all background work. [Result and explicit failed coverage gate](validation/finishing-8.39/idm-user-io-4m.json)

## Native-observer control and handoff gate

The next uninstrumented 4 MiB control also failed to start: IDM's launcher returned zero, but the owned loopback server received zero requests and sent zero bytes, and no output appeared. Therefore the prior native-observer timeout does not establish instrumentation as its cause. Further writer conclusions are gated on repairing or explaining the IDM handoff first. No larger transfer was attempted.

The private controller now saves its last observer snapshot, messages, server requests, launcher status and output existence/length on exceptions before detaching. Its updated source passes Python syntax compilation. Both supported desktop-control routes failed to initialize with a missing kernel-assets path, preventing inspection of a possible modal prompt; no modal prompt is asserted as observed. [Control and limitations](validation/finishing-8.39/idm-native-observer-control.json)

## Handoff restored after completion dialog closure

The user supplied a screenshot identifying the visible prompt as the earlier 4 MiB Download complete dialog and then confirmed closing it. The unchanged uninstrumented control subsequently made one request, received all 4,194,304 bytes and matched the independent SHA-256. Its completed payload was removed. The preceding zero-request result remains preserved in its original private record and the new ordered evidence, rather than being replaced by this successful retry. [Both controls](validation/finishing-8.39/idm-handoff-dialog-control.json)

Clearing the dialog restored CLI handoff in this observation. Native writer/flush coverage and timing parity remain unverified; the next instrumented run waits for closure of the new control's completion dialog. No IDM preference or UDM functional code changed.

## x86 coverage diagnosis

An independently built 32-bit fixture using the installed .NET compiler verified the observer on the same architecture as IDM: one full native 256 KiB write, two native flushes, correct bytes/hash and normal detach. Its payload and small executable were removed.

A fresh actual IDM 4 MiB download also matched its independent size/hash and was removed. The added non-content counters observed 423 native calls and 421 resolved handle paths, but zero full-token path matches. Thus the hooks do execute in IDM; the remaining coverage failure is at the path-filter/temporary-identity boundary. These counts do not identify which unrecorded paths carried the controlled body, establish flush absence, or verify timing parity. Both UDM catalogs and the installed executable retained their hashes. [Fixture and actual-process evidence](validation/finishing-8.39/x86-observer-attribution.json)

## Controlled IDM temporary-body writes observed

The bounded private path sample identified the filter mismatch: IDM truncated this owned 32-character hexadecimal filename to twenty characters in its DwnlData directory and temporary body filename. The revised filter requires the known cache root, exact token prefix, numbered directory and exact body/log leaf; generic cache paths do not qualify. Private resolved paths are not copied into tracked reports.

A fresh instrumented 4 MiB download captured 367 successful synchronous native body writes totaling exactly 4,194,304 bytes, alongside separate log writes. The final file matched its independent hash and size and was deleted; the observer detached normally. No explicit matched native or Win32 flush call was observed in this window. This does not establish absence of implicit, close/rename, later or device-cache writeback, nor explain every finishing loss. Null native offsets prevent asserting interval coverage from totals alone. [Sanitized body-write observation](validation/finishing-8.39/idm-temp-body-observation.json)

## Isolated UDM operation comparison

The qualified 8.39 executable completed an isolated 4 MiB transfer with the expected independent hash; its output payload was deleted and both personal catalogs retained their hashes. The observer captured seven native flush operations. Each maps uniquely to its enclosing Win32 call by handle, thread and QPC interval, so fourteen API events do not mean fourteen flushes. Roles were {'catalog': 3, 'body': 3, 'publication-intent': 1}. The separate IDM matched-file observation captured zero explicit flushes. These separate instrumented observations identify an operation-policy difference, not a causal speedup or full durability equivalence.

Normal shutdown of the owned build is pending: desktop input failed with GetCursorPos access denied, and the exact owned process remains alive. No installed process was terminated and no all-background-work endpoint is claimed. [Operation comparison and limitations](validation/finishing-8.39/file-io-operation-comparison.json)

## Flush ordering against the last body write

The seven observed UDM native flushes do not constitute seven finishing barriers. Four completed before the last successful native body write returned: two catalog flushes and two opportunistic body flushes. Three began afterward: final body flush at +15.5721 ms, publication-intent flush at +20.8320 ms, and catalog flush at +26.5882 ms. Their native durations totaled 3.9410 ms in this particular instrumented C: sample; the final catalog flush returned at +27.3407 ms. Pre-publication metadata accounted for 10.1528 ms in UDM's own phase counters.

The reference is a body-file write return, not the origin's last sent byte, visible completion or application exit. These figures do not establish hardware/kernel attribution or extrapolate to the retained D: losses. Changing early-flush milestones would not remove the three observed post-body durability operations. [Complete ordered evidence](validation/finishing-8.39/udm-flush-timeline.json)


## 8.42: targeted D: 64 MiB sequential observations

Three fresh owned loopback transfers were used, all with one connection, an 8,000,000-byte/s source cap and the deterministic 20261113 payload. Every final file had 67,108,864 bytes and SHA-256 `A5F55D2DA15C2BDD9999D79DFD6B952BC8FFFEE2508666650BF202699AFEDB12`. No new product behavior or version change was made during this research; native remains 8.42 and source extension manifests 0.72.6.

The first Frida observation completed its download and process exit, but its exit hook delivered no final snapshot. It is retained as an observation failure, not IO coverage. A second controller streamed each owned event during execution and captured 607 events without a native terminal snapshot. It observed twelve native flushes, each enclosed by one same-thread/same-handle Win32 wrapper. These are twelve operations, not twenty-four. Three native flush operations began after the final source body send: body 56.277 ms, intent 20.828 ms, catalog 17.779 ms. Their combined API durations were 94.8845 ms; process exit followed the final source send by 196.483 ms. All values are instrumentation observations, not uninstrumented comparative performance. The body-write completed-byte sum matched the output length, but does not prove non-overlapping writer coverage. The private controller reused its clock-anchor variables before saving a QPC error bound; that numeric bound was discarded from the public result. [Streamed operations and limitations](validation/finishing-8.42/udm-single-64m-user-io.json) · [Both observer attempts](validation/finishing-8.42/observer-attempts.json).

A separate diagnostic binary compiled the unchanged current Core, Transfer, Duplicates and App sources with the existing bounded native markers. It reused 39 qualified core objects, changed no pacing, and was never installed. Its mapped trace survived normal exit with no dropped events. The final source-body-to-process-exit interval was 219.491 ms:

| Observed interval | Time |
| --- | ---: |
| Output open/flush/close path | 69.112 ms |
| Durable intent write | 19.535 ms |
| Final catalog checkpoint | 62.069 ms |
| Publication cleanup to worker commit return | 0.598 ms |
| Final worker proof | 0.861 ms |
| Finished notification to close handler | 0.555 ms |
| Integration stop | 0.219 ms |
| Manager stop, including joins/proof | 1.150 ms |
| Final frame close call | 26.625 ms |
| ExitInstance | 3.013 ms |

These intervals do not form a complete additive partition. The intent's inner temporary-file flush was 17.326 ms. The catalog's inner temporary-file flush was 50.129 ms, followed by about 10.928 ms before its file-published marker. Catalog write-lock acquisition was effectively immediate, proof resolution was about 0.001 ms, and Manager shutdown reused its exact completed checkpoint without another forced save. There was no observed 500 ms timer wait after the Finished notification. The 10.473 ms between File Info worker stop and Manager stop spans status/layout, disabling the window and optional progress/network cleanup; this trace does not isolate those calls. [Decoded native events](validation/finishing-8.42/udm-single-64m-native-phases.json) · [Diagnostic build identity](validation/finishing-8.42/diagnostic-build.json).

The observations identify explicit persistence waits as the dominant measured tail in this case; they do **not** identify whether the underlying kernel wait comes from disk cache, filesystem/filter work or scheduling. They do not establish a new IDM comparison, recovery responsiveness improvement or universal speed parity. Desktop input failed twice with `GetCursorPos: Access is denied`; the existing IDM and older owned observation app still need a normal close before the process-lifetime comparison can run. No app was force-terminated.

The next performance change must preserve completion durability and restart proof. Evaluate an explicitly journaled protocol that can prepare recovery intent earlier or overlap independent persistence work, with fault coverage for an intent surviving before body durability. Do not merely remove the body/catalog flush, acknowledge Complete earlier, or add another late-flush pacing heuristic. Measure ordinary uninstrumented lifetimes and a symmetric post-exit output flush separately once IDM is closed. Preserve the current losing baseline rows.

All three 64 MiB final payloads were individually verified and deleted (201,326,592 logical bytes). The temporary diagnostic executable, four compiler objects and trace mapping were individually hash/link/reparse checked and removed (105,955,180 logical bytes). Qualified production products/core, decoded evidence, compact controllers, personal catalogs and the installed app are retained. [Cleanup receipt](validation/finishing-8.42/cleanup.json).


## 8.43 overlap experiment: correctness passes, speed gate remains unmet

The body/intent overlap candidate passes 1,473 focused checks, including explicit recovery writeback, changed/truncated/foreign source refusal, name/write ownership and an actual process interruption before the body flush syscall. Five native products and ten focused fixtures are built. Two alternating uninstrumented D: 64 MiB trials retained all four rows: 8.42 exit tails 244.230/288.961 ms; 8.43 tails 263.306/276.327 ms. Median exit tails were 266.595/269.816 ms, and symmetric post-exit durability medians 293.097/297.584 ms. There is no demonstrated speedup; installation is withheld. This is a UDM-version experiment, not a fresh IDM comparison. Both desktop controls failed, and normal close of existing IDM/older observation app remains pending. [Protocol, failed prototypes, full qualification, all timing rows and cleanup](publication-barrier-overlap-8.43.md).


## 8.44: retire overlap; resolve the known IDM flush condition

The ordinary overlap task was removed after the unproven 8.43 timing experiment. Saved-intent recovery and ordinary final-name pins remain; all 1,491 focused checks pass. A 64 MiB serial-path smoke completed with an independent matching hash and no early-intent/overlap marker. Its 348.894 ms app-exit tail is retained as a smoke observation under concurrent REA analysis, not a controlled speed comparison. Twenty obsolete prototype artifacts and current generated files were individually removed after exact source reconstruction.

REA plus bounded image disassembly resolves the known explicit IDM flush chain to a progress-driven guard beyond 40 seconds and below 500 KiB/s in an effective byte-rate field that includes a tenth increment and optional cap. Socket-closing and destructor-like callers use a control argument that bypasses this branch. This is a static call-chain finding, not proof of all runtime IO or crash durability. No new UDM pacing rule was inferred from it. [Full findings, primary CRT/tool references, qualification, smoke and cleanup](serial-publication-8.44.md).

## 8.45: early-zone metadata gate did not establish a full-lifetime speedup

The corrected candidate proves its marker exists before any origin body byte, repairs an intentional pre-EOF ZoneId change, and passes all 489 focused checks. A stale private link selecting the baseline Transfer object was detected by failed live observations, corrected, and all candidate checks repeated. This was not installed.

Two alternating D: 64 MiB trials retain one candidate win and one loss. Median exit tails are 249.509 ms (8.44) / 249.213 ms (8.45); including a separate post-exit output flush, medians are 267.779 / 299.547 ms. The full-lifetime performance gate is not accepted. No new pacing, durability relaxation, IDM parity or kernel-cause claim follows. [All source/build ownership evidence, rows, failures and cleanup](early-zone-metadata-8.45.md).

## 8.46: recovery journal identity before detached IO

The rejected early-zone metadata experiment is retired. A separate native probe reproduced ordinary and replacement recovery deleting changed journal bytes/names at actual catalog flush. The fix holds the parsed journal through catalog acceptance and retires the captured file by handle; all 1,661 focused checks pass, including C:/D: journal cases. This is a correctness prerequisite for recovery detachment, not a finishing-speed or model-lock fix. A real 4 MiB native-app smoke passed; no new installed IDM comparison was made. [Detailed evidence and cleanup](recovery-journal-ownership-8.46.md).
