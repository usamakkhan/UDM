# UDM 9.78: faster opened-file volume lookup; completion-speed gate still failed

UDM 9.78 replaces a repeated full-path volume walk with a guarded query through the actual opened file when Windows supplies a valid volume-GUID name. Both extension source manifests advance to 0.73.42; their JavaScript behavior is unchanged. Five native products build with version 9.78. **This candidate is uninstalled and is not promoted as faster overall.**

## Cause and implementation

The [9.77 worker probe](finishing-setup-probe-9.77.md) measured about 2.5 ms in the final volume-path query, compared with about 0.013 ms for advisory task submission. It did not reproduce the earlier 158 ms broad metadata interval. The new helper opens only file attributes with sharing for existing readers, writers and renames, requests the opened object's volume-GUID path, validates the complete GUID root and asks for its drive type. It closes its handle on every route.

A recognized removable, remote, CD-ROM or RAM volume remains non-fixed. Missing/unopenable files, malformed/truncated names, unsupported queries and unknown drive types retain the original path-based policy. No drive-letter result is cached. No hash, copy, body flush, durable intent, final catalog barrier, publication identity check or shutdown work is removed. The helper classifies storage for existing transfer policy; it is not a new publication identity proof.

Windows documents the opened-name/GUID flags and the limitations of GUID results on network or vendor-managed volumes in [GetFinalPathNameByHandleW](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfinalpathnamebyhandlew); root-path and return-type semantics are documented in [GetDriveTypeW](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getdrivetypew). Unsupported routes retain the legacy lookup. Live SMB, removable-media and mount-transition qualification is still outstanding; the existing-file fast attempt can add work before that fallback and needs its own remote-performance check.

Implementation: [FixedVolume.hpp](../native/FixedVolume.hpp), [Transfer.cpp](../native/Transfer.cpp), [FixedVolumeTests.cpp](../native/FixedVolumeTests.cpp), [isolated change from qualified 9.77](validation/fixed-volume-9.78/build/fixed-volume.patch). The reconstructed old Transfer bytes match its qualified hash. No unrelated source was reverted.

## Correctness and isolated cost

All **1,551 assertions across 23 affected backend suites** pass on D:. The nineteen new volume checks also pass on C:. Tests cover handle closure, fixed and known non-fixed results, malformed GUIDs, query truncation/failure, unknown types, missing files, remote fallback and real file/directory classification. Synthetic route tests do not establish live network/removable-device behavior.

A compiled read-only comparison calls the actual helper on existing C: and D: files ten times each, alternating measurement order. All twenty comparisons agree with the legacy classification, use one closed attribute handle and take no fallback. Median API-operation times are:

| Existing-file control | Legacy path walk | New complete helper call |
| --- | ---: | ---: |
| C: | 0.386 ms | 0.079 ms |
| D: | 0.876 ms | 0.114 ms |

This microcheck uses shallow existing paths and warm repeated queries. It measures the helper, not the several-millisecond deeper transfer-path lookup or whole-app completion. Its target files are read-only inputs; no payload is generated.

## Full normal-product pilot: retain every loss

One preplanned pair per case compares uninstrumented normal 9.77 and 9.78 products. Order is baseline/candidate for 4 MiB, candidate/baseline for sequential 64 MiB, and baseline/candidate for split 64 MiB. The same owned origin policy serves each file at 8,000,000 bytes/s per connection with 20 ms response latency. Both programs use their normal CLI/worker/window/shutdown path. All six outputs match the independent SHA-256 and final Complete catalog; all processes exit normally. An explicit output flush is measured after process exit.

| Case | Total to exit, 9.77 / 9.78 | Last body to exit, 9.77 / 9.78 | Last body to post-exit flush completion, 9.77 / 9.78 |
| --- | ---: | ---: | ---: |
| 4 MiB sequential | 1,045.042 / 1,231.250 ms | 159.284 / 148.304 ms | 183.779 / 178.400 ms |
| 64 MiB sequential | 8,893.242 / 8,916.129 ms | 176.786 / 194.027 ms | 201.391 / 215.811 ms |
| 64 MiB split | 2,447.484 / 2,427.951 ms | 335.313 / 364.763 ms | 358.160 / 390.695 ms |

The broad metadata interval decreases in all three samples: approximately 5.98→2.26, 4.97→1.91 and 4.34→1.92 ms. Nevertheless, 4 MiB total time worsens, sequential 64 MiB total and finishing worsen, and split 64 MiB finishing/durability tails worsen despite a lower total. The candidate body flush calls in the two 64 MiB cases take about 98.88 and 240.01 ms versus 83.12 and 207.85 ms. One pair cannot establish causation or a stable speed difference; it also cannot satisfy the no-slower gate. Do not repeat until a favorable sample appears or discard these losses.

There is **no new IDM comparison**. Its current-session background process remains present; supported Windows control again failed with “failed to write kernel assets” (OS error 3). Neither IDM session was terminated or its settings changed. No installer, installation, commit, push or release occurred.

Next investigate whether an already owned file handle can supply the volume query without another late attribute open, and correlate the remaining writeback/intent/catalog waits with filesystem/filter/storage activity. Preserve full-lifetime endpoints, remote fallback behavior and durability. The outside/inside parity goal remains open.

## Build evidence, registration and cleanup

[Build and captured inputs](validation/fixed-volume-9.78/build/build-result.json), [suite results](validation/fixed-volume-9.78/build/suite-results.json), [C: checks](validation/fixed-volume-9.78/suites/FixedVolumeTests-C.json), [read-only costs](validation/fixed-volume-9.78/cost/results.json), [all six pilot records](validation/fixed-volume-9.78/pilot/results.json), [planned matrix and order](validation/fixed-volume-9.78/drivers/fixed-volume-pilot-plan-9.78.json), [final audit](validation/fixed-volume-9.78/final-audit.json), [export hashes](validation/fixed-volume-9.78/export-manifest.json).

Transfer and Bridge are recompiled; forty core objects are retained. After compilation and the selected checks, the new fixture is registered in `native/backend-tests.json` for future regular runs. That JSON is not consumed by native compilation. The original pre-registration source inventories are retained alongside the explicit metadata-only amendment; the final inventory verifies 696 inputs, including the updated registry. Do not describe the registry's updated bytes as captured before compilation.

All six pilot payloads (264 MiB) are hash-checked and deleted. After sanitized export, 278 disposable fixture/build files are individually path/type/hash checked and removed, totaling 259,313,181 logical bytes. Current five products, the resource and all 42 referenced core objects remain. The separate 9.77 setup probes have also removed their 68 MiB payloads and eleven disposable artifacts, as recorded in their report. Physical space reclaimed was not measured. Protected installed binaries, extension manifests, personal catalogs and shared media helpers retain their recorded hashes. Compact diagnostic catalogs/evidence remain.
