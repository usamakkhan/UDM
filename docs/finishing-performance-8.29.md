# Separating published-file open from flush: 8.29

Desktop 8.29; Chromium/Firefox manifests 0.71.93. The previous [8.28 split outlier](finishing-performance-8.28.md) combined published-file opening and flushing into 1.831 seconds of a 2.715-second tail. That historical interval cannot be split retroactively.

## Writeback path

Final writeback now requests GENERIC_WRITE without unnecessary GENERIC_READ. FlushFileBuffers requires write access; sparse conversion accepts write-data or write-attributes access. [Microsoft FlushFileBuffers](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-flushfilebuffers) · [Microsoft sparse control access requirements](https://learn.microsoft.com/en-us/openspecs/windows_protocols/MS-FSA/b5b3dd4c-b585-49d7-960e-fbfbf7a85af0)

The worker records PublishedFileOpen, PublishedSparseNormalize, PublishedFileFlush and PublishedFileClose separately, along with operation-specific Windows errors. PublishedWriteAccess records write-only. The prior FlushPublished aggregate stays; it now includes handle close explicitly. The original full-process timing never excludes that close.

Regular same-file, copied-destination and sparse-normalization paths share the helper. Open failure never attempts flush/close on an invalid handle. Valid handles are flushed and closed even if normalization fails. Failed confirmation retains downloaded parts and records a warning. This also removes the older sparse branch's ignored flush result. Required final flush, hash join, cleanup and durable final checkpoint remain before retirement. No success is inferred from merely opening the output.

Sixteen additional checks cover a real write-only flush preserving bytes, missing output, deliberately held open versus flush operations, requested access and operation ordering, flush/open/normalization failures, and five actual HTTP-worker modes recording all counters before the final checkpoint. Completed-checkpoint checks increase from 92 to 108.

## Symmetric durability endpoint

The tracked runner adds an optional `--flush-after-exit`. After observing all application processes exit, it verifies expected output length/exclusive availability, opens the completed file exclusively for writing, calls FlushFileBuffers, closes it, then independently hashes it. Record open/flush/close timings and durable tail/full elapsed endpoints separately. Restore IDM settings as before. The original process-exit tail and wall metrics remain unchanged and are not replaced by a more favorable score.

The installed comparison enables this option for both applications in every paired trial. This covers application-owned work through exit plus a symmetric completed-output durability barrier; it does not measure unrelated registry writes, all system services or prove hardware power-loss reliability. No GUI dialog timing is inferred.

## Gates

Build and qualify the updated native code, install verified files with rollback/catalog protection, retain every paired observation and compare both endpoint families. Only after process exit, explicit output flush and size/hash verification remove owned temporary payloads and generated target objects. Preserve current binaries, canonical core, source, catalogs, evidence, rollback records and required historical fixtures.

## Installed qualification and precise attribution

All **1,848 maintained native checks passed**, including 108 completed-checkpoint checks, 274 completion checks through observed exit 0 and the live gzip/prefetch/resume workflow. All 25 binaries have verified 8.29 resources/hashes; 491 frozen inputs and 42 canonical core objects match. Transfer and Bridge rebuilt; 40 core hashes retained. Installed seven changed files and version registration with unchanged personal UDM catalogs. [Qualification](validation/finishing-8.29.json) · [Installed receipt](validation/finishing-8.29/installed.json)

Across all nine UDM transfers, published-file opening took **0.138–0.263 ms**, closing **0.033–0.073 ms**, and flushing **2.388–266.728 ms**. In split trial 2 specifically, opening took **0.255 ms** and FlushFileBuffers **266.728 ms**, with close 0.071 ms. The measured current bottleneck is the flush interval, not handle opening. These wall intervals include error capture/scheduling and are not a kernel device-service trace. The historical 2.7-second combined interval cannot be separated retroactively, and that stall did not recur in this run. Removing read access corrects an unnecessary request; it is not evidence that the underlying flush latency has been fixed.

An additional real NTFS sparse-file probe passed four checks using the production helper: create a sparse file with a known prefix and unwritten hole, normalize/flush through the write-only handle, verify the sparse flag clears and exact length remains, then independently hash the whole 1 MiB result. Its bytes matched the expected prefix plus zero-filled hole. This supplements the maintained 1,848 checks and is counted separately. Its temporary source/object/executable/payload were removed after exit and verification. [Actual sparse probe](validation/finishing-8.29/sparse-writeback-probe.json)

## Full comparison: target remains unmet

All eighteen downloads matched their expected independent sizes/hashes and UDM's final saved hashes. All UDM open/normalization/flush/close error codes were zero. IDM connection settings were restored. [All observations and paired scores](validation/finishing-8.29/comparison.json)

| Case | UDM median process-exit tail | IDM median process-exit tail | UDM median full elapsed | IDM median full elapsed |
| --- | ---: | ---: | ---: | ---: |
| 4 MiB sequential | 405.884 ms | 339.741 ms | 1.406 s | 2.171 s |
| 64 MiB sequential | 362.188 ms | 372.225 ms | 9.090 s | 9.923 s |
| 64 MiB split | 519.835 ms | 508.974 ms | 2.398 s | 4.052 s |

The original process-exit metric won **five of nine paired tails** and **all nine full elapsed times**. It did not pass the no-slower target. UDM split tails were 254.171, 647.131 and 519.835 ms; IDM split tails were 363.692, 508.974 and 2,552.917 ms. Neither app's outliers are excluded.

| Case | UDM median tail through post-exit flush | IDM median tail through post-exit flush | UDM median full durable elapsed | IDM median full durable elapsed |
| --- | ---: | ---: | ---: | ---: |
| 4 MiB sequential | 470.958 ms | 651.337 ms | 1.419 s | 2.261 s |
| 64 MiB sequential | 432.726 ms | 671.327 ms | 9.161 s | 10.222 s |
| 64 MiB split | 585.608 ms | 1,545.457 ms | 2.540 s | 5.089 s |

The symmetric output-durability endpoint won **eight of nine paired tails** and **all nine full elapsed times**. The remaining durable-tail loss was sequential 64 MiB trial 3: **720.012 versus 671.327 ms**, approximately 48.685 ms slower. Thus this endpoint also failed the all-cases target. It is not substituted for the original failed score.

Post-exit flushes change inter-trial cache conditions relative to older runs without that barrier, so these comparisons do not isolate a code-only causal speedup. Flush timing can include other storage contention, and post-exit flush costs do not prove that every millisecond represents dirty bytes of that particular file or reveal IDM's internal API sequence. The existing IDM profile, request scheduling, hardware and other storage activity remain confounds. The new counters identify current user-visible wait locations, not a universal hardware diagnosis.

## Evidence labels and cleanup

The copied private launcher retained an unquoted 8.25 text label in versions 8.26–8.29. Qualification resources, installed receipts and every UDM row's executable SHA-256 verify which binary actually ran. Public labels are corrected with the original recorded label retained; raw private files are unchanged. Canonical fingerprints verify every non-label observation is unchanged. The launcher now derives its label from the qualified version. [Correction ledger](validation/finishing-8.29/evidence-label-correction.json)

Individually removed **270 generated files / 1,099,818,205 logical bytes** after process exit and verification: eighteen benchmark outputs, fixture payloads, unique target objects, twenty superseded 8.28 test executables and the supplemental sparse probe artifacts. Linked input objects were verified before cleanup. Preserve all current binaries, 42 canonical core objects, source, catalogs, reports/journals/screenshots, rollback copies, shared tools and required historical fixtures. Logical size is not measured physical allocation. [Cleanup records](validation/finishing-8.29/cleanup-total.json)

The remaining optimization is writeback scheduling, not a faster file open. Any bounded background-writeback experiment must pace outside the model lock, preserve range-credit/resume correctness, wake cancelled writers, handle flush failures, join before teardown and retain the required final durability barrier. Measure full elapsed time so a shorter finish cannot hide a slower transfer. The earlier unbounded parallel prefinish experiment did not improve its median; do not reinstate it without new evidence. No unchanged-build reruns were used to seek a passing score. No setup package, commit, push or release; live GUI timing remains unverified.
