# Native I/O observer scope and lifecycle, 8.58 research

This is a diagnostic-tool correction. Native UDM remains 8.58, both extension manifests remain 0.72.22 and installed UDM remains 8.32. No product speed improvement, installer, installation or IDM parity result is claimed.

## Corrected handle and sample scope

The observer previously called GetFinalPathNameByHandleW for every native write/flush handle. It now checks GetFileType first and performs path resolution only for FILE_TYPE_DISK. Pipe/character handles and unknown types have separate skip/failure counters. This avoids issuing file-path queries for unrelated IPC. It does not remove the underlying native API hooks or guarantee that every possible file-path query is nonblocking.

The previous nativeResolvedSamples list retained resolved paths before checking the owned-workload token. The corrected code checks ownership first; unrelated paths are never appended to that list. Aggregate resolution counts remain. The exact before/after source delta is retained as a reversible [patch](../benchmarks/speed/experiments/observer-scope-8.58.patch).

## Actual controls and failure

The [owned fixture](../benchmarks/speed/finish_observer_fixture.cpp) requires an existing fixture marker and same-parent target paths, refuses an administrator token, and waits for the controller before writing. It writes 32 bytes through an anonymous pipe, a separate 32-byte unrelated file and an 8 KiB owned body. It then flushes, maps/changes one byte, flushes again, closes and renames the owned body. Its second controller gate allows a snapshot before normal process exit.

One full-observer attempt completed its file work and returned from stop/unload/detach, but the released target stayed alive with one CPU-consuming thread. Three bounded samples observed the thread, including execution near NtDelayExecution and in private executable memory; raw stack words were not treated as an unwound stack or a root-cause attribution. After verifying the exact owned executable and its hash, the fixture was terminated with exit code 238. That attempt is failed, not a successful normal-exit run, and was never used for a performance claim.

The same binary exited normally without Frida and with an empty Frida script. Keeping the full observer script loaded until normal target exit also succeeded. These controls narrow this fixture failure to the hook/teardown interaction, but do not prove the cause of IDM's earlier hang. For short owned processes, save observations first, stop hooks, release the target, and retain the script until its observed normal exit. Do not early-unload the full observer in a real application until that separate lifecycle path is qualified. An observation timeout is not a reason to launch another copy or claim exit.

## Before/after results

The unchanged baseline agent under the revised lifecycle retained the unrelated test path, confirming the scope defect. The corrected agent on both C: and D: retained only owned paths. Each of the three runs captured 11 expected events, including one completed 8 KiB native write and two native flushes. Every event reported success and an ordered interval; each run had zero dropped events and zero remaining tracked files/mappings. Both target file hashes matched expected values and the process exited normally. Each corrected run skipped two nondisk pathname queries, had zero path-resolution failures and retained three matching native operations.

The runtime printed an idund startup warning in two attempts, while target attachment and the explicitly verified work succeeded. Those observed warnings are recorded in the sanitized results; no claim about idund health is made.

These are instrumented fixture observations. They do not establish complete production writer coverage, nonoverlapping byte-offset coverage, all kernel/background work or uninstrumented speed parity. In particular, the kernel FileIo opcode-84 coverage failure remains unresolved. The next application observation must preserve its normal-exit evidence and a terminal coverage record; an early snapshot or matching byte sum alone cannot establish complete IO coverage.

[Sanitized runs and retained failure](validation/finish-realtime-8.58/observer-scope-and-lifecycle.json). After terminal outcomes and individual boundary/link/reparse/hash checks, 12 payloads and three retired diagnostic build outputs were removed: 707,014 logical bytes. The single small reusable fixture binary, compact evidence and the existing shared Frida runtime remain. [Cleanup](validation/finish-realtime-8.58/observer-scope-cleanup.json).


## Subsequent backend use

The counter scratch call now runs exclusively under Frida's JavaScript lock and the observer has been used through a controlled Manager-destruction boundary. A terminal exit-hook experiment failed and was retired; that failed path is not the application measurement method. The real 64 MiB observation and two rejected sparse candidates are recorded in [finishing attribution](finishing-flush-attribution-8.58.md). Positive byte-interval coverage in that run is narrower than proof that every possible writer/kernel operation was observed.
