# UDM 8.45: early Internet-zone metadata experiment

Native 8.45 and extension source 0.72.9 were an **uninstalled experimental candidate, retired by 8.46**. Correctness checks pass, but the complete-lifetime performance gate is **not accepted**. This is not a fix for all finishing delays and must not be promoted as faster than 8.44 or IDM.

## What changed

For a fresh ordinary HTTP download with a known positive size no greater than 64 MiB, uncompressed content, a regular local file on a fixed volume and no known cross-drive inline-copy requirement, prepare the Internet-zone stream after response headers arrive and before body writers begin. This is worker work; no new UI wait or task is introduced.

An attempt is tracked only in the transfer's lifetime. At the existing final metadata step, the candidate verifies the exact complete marker and preserves it if already correct. A missing, truncated, different or inaccessible marker takes the existing best-effort write path. A copy or prefix-transformed result therefore still receives its final marker. Server-date handling, body flush, publication intent, final rename, catalog acceptance, recovery pins and flush pacing remain unchanged.

`Core.cpp` remains byte-identical to qualified 8.44. Other transfer paths retain the original single metadata-write call, avoiding a new generic cold read on remote destinations. Both extension manifests advance to 0.72.9; browser JavaScript is unchanged and no extension files were installed.

## Correctness and build ownership

All **489 focused checks** pass: Internet-zone handling 28, completed checkpoint 145, publication preparation 87, early-intent recovery 156 and publication recovery 73. These cover exact markers, malformed/truncated/extended/differently encoded markers, refused writes, a removed marker, hard-link staging/rename and body-only copy fallback, alongside the existing durability and recovery checks.

Two actual 4 MiB downloads each matched the independent length and SHA-256, used one origin request, exited normally, passed a separate post-exit output flush and left no publication journal. The marker was observed while both received origin bytes and local body length were zero. One case deliberately changed it to ZoneId 1 before EOF; final verification repaired the marker to ZoneId 3. Both payloads were removed. [Live records](validation/early-zone-metadata-8.45/live-downloads.json).

The first private link accidentally retained the baseline Transfer object while tagging the executable 8.45. The live check correctly failed to observe early metadata; a bounded user-mode IO observation corroborated only the old final mark. That candidate was rejected, and the final link explicitly checks both the selected Transfer object's path and its compiled-source/object hashes. All five focused suites and both live cases were repeated against the corrected build. The earlier failures remain evidence and are not counted as successful candidate tests. No candidate was installed. [Rejected-link record](validation/early-zone-metadata-8.45/stale-object-rejection.json).

The corrected build contains 42 core objects: 38 retained canonical objects, the qualified 8.44 Core object with unchanged source closure, and private candidate Transfer/Duplicates/Bridge objects. Five production programs and five fixtures were linked and all ten file versions were checked as 8.45. Fixture executables and redundant target objects were retired after verification; five production programs and current core inputs remain. [Qualification, exact source/object/product hashes](validation/early-zone-metadata-8.45.json).

## Full-lifetime timing gate

Two alternating sequential 64 MiB D: trials per version used the same owned 8,000,000-byte/s loopback origin. There was no instrumentation, native build or REA analysis during these comparisons. Each result includes actual application process exit, then a separate symmetric output flush. All four outputs matched the independent hash and were removed immediately. These are source-build comparisons, not installed IDM comparisons.

| Version | Exit tail, trial 1 / 2 | Median exit tail | Median tail through separate output flush |
| --- | --- | --- | --- |
| 8.44 | 262.112 / 236.906 ms | 249.509 ms | 267.779 ms |
| 8.45 candidate | 295.202 / 203.225 ms | 249.213 ms | 299.547 ms |

The candidate has one exit-tail win and one loss. Its exit median differs by only 0.296 ms, while its durability-tail median is worse by 31.768 ms. The measured metadata region is 5.409/6.682 ms in the baseline and 5.341/3.910 ms in the candidate; this region includes more than the zone operation. Candidate body-flush waits are 114.364/84.988 ms versus baseline 99.833/61.469 ms. The sample is small and cannot establish causation or a universal regression, but it does not establish the requested speedup. Keep every result; do not treat moving a metadata write as proven overall progress in speed. [All timing rows and phases](validation/early-zone-metadata-8.45/sequential-64m-comparison.json).

The experiment is preserved as a reverse-checkable [seven-file source delta](../benchmarks/speed/experiments/early-zone-8.44-to-8.45.patch); reconstructed baseline Transfer bytes match the qualified 8.44 source hash. Before promoting this candidate, either obtain convincing full-lifetime evidence or retire its active path while retaining the compact experiment and losses. Do not add more pacing rules based on this result.

## Remaining work and installation

Installed desktop remains **8.32**. Research-first installation deferral remains in force. No installer, extension install/reload, push or release was performed. Both personal catalogs and the installed executable retain their original hashes.

The prior slow IDM observation positively establishes one matched mid-transfer body flush, not complete coverage or total durability. The remaining high-rate case and a fresh installed comparison await normal closure of its completion dialog; desktop input still fails with `GetCursorPos: Access is denied`. The original sequential/split finishing delays and the recovery transaction's shared-model IO remain open. Neither the experiment nor the green focused suites closes those gaps.

Removed all 276 MiB of generated download payloads across the successful and rejected private observations/comparisons, plus 520 generated fixture/object/executable files (102,548,722 logical bytes) and the initial unqualified Core object. Complete hard-link groups were verified before individual removal. No recursive deletion, directory/junction deletion, personal catalog change or process termination was used. Canonical tools/objects, qualified 8.42/8.44 references, compact evidence and current production candidate binaries remain. Logical totals do not measure physical reclaimed allocation. [Cleanup](validation/early-zone-metadata-8.45/cleanup.json).

## Retirement in 8.46

The active metadata path, helper and fixture are removed. Five old production executables, three private core objects and the old resource were retired after exact ten-file source reconstruction; their hashes and measured losses remain. The ordinary Transfer source/object are restored to qualified 8.44. [Current recovery-journal fix, source retirement and cleanup](recovery-journal-ownership-8.46.md). Earlier retention statements describe the qualification-time state, before this retirement.
