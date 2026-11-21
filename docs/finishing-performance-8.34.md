# UDM 8.34: inline hashing for fresh sequential HTTP

Desktop version: **8.34**. Chromium and Firefox source manifests: **0.71.98**. Installation remains deferred at the user's request; installed desktop remains **8.32**. No installer is created.

## Evidence and change

The preceding [high-resolution diagnostic](finishing-io-research-8.33.md#high-resolution-join-attribution) separated a 61.931-ms prefix-hasher join from file-handle close, intent and catalog waits in a sequential 64 MiB sample. The source showed that large sequential responses reread successfully written bytes on a background prefix thread, although small fresh responses already hash their write buffers inline.

`Transfer.cpp` now extends that existing inline path to fresh known-length responses with proven non-range ownership: range support is absent, reconnect verification is not required, and the initial range probe is not pending. Those responses have one download writer. SHA-256 consumes bytes only after their successful write, retaining the existing contiguous-offset validation and disk fallback if coverage becomes invalid. Resume, pending-confirmation and unproven range ownership retain their existing paths.

Extending hashing does not extend full-body capture. The previous small-response capture-size limit remains; larger inline responses retain the bounded sniffing prefix rather than reserving the entire body. Existing optional body-cache limits remain separate. Required user/server digest verification still gates publication. Removed an unsupported source comment claiming IDM never hashes files; the implementation does not depend on that assumption.

Flush pacing, final file writeback, publication intents, catalog acknowledgement, crash recovery and GUI behavior are unchanged. This change removes redundant read work on an eligible path; it does not solve every measured durability wait.

## Validation and native build

The new live loopback regression exercises 8 MiB responses with irregular 65,521-byte server chunks, configured eight connections, and actual sequential ownership. It checks final length, independent payload digest, saved digest, inline path selection, required-digest acceptance and rejection, and range-path exclusion. The range case actually reached two concurrent connections. All **17 new checks passed**.

All **1,930 checks in 21 selected suites passed**, including the new 17, existing 274 completion checks and 43 live gzip/prefetch/resume checks. This is the selected transfer, recovery, admission, publication and completion qualification, not the entire repository test catalog. An initial admission fixture stopped after fourteen passing checks because its new output directory lacked a media-helper reference. A junction to the canonical installed tools corrected the fixture environment without copying any helpers; the [initial result](validation/finishing-8.34/initial-media-helper-failure.json) is retained and a fresh retry passed all 73 checks. This is an environment correction, not a hidden product assertion failure.

The completed native build recompiles Transfer and version-bearing Bridge, retains forty canonical core objects and compiles all **26 targets** against new 8.34 resources. All **494 frozen source inputs**, 42 core objects and 26 binaries match their qualification hashes; all binary version resources were checked. Five previous 8.33 product executables were moved to their existing qualification area before replacement, preserving the earlier payload without another copy. [Qualification](validation/finishing-8.34.json) · [Final verification](validation/finishing-8.34/final-verification.json)

## Qualified portable comparison

With compilation, correctness tests and fixture cleanup stopped, ran the qualified production executable from its build folder through all eighteen original alternating transfers. This is a C: build-folder comparison, not an installed or D: result. All eighteen lengths and independent digests matched; every UDM saved Complete digest matched. IDM settings were restored. All three 64 MiB sequential runs recorded `HashMode=inline`, confirming selection of the extended path.

| Case | UDM median exit tail | IDM median exit tail | UDM median full elapsed | IDM median full elapsed |
| --- | ---: | ---: | ---: | ---: |
| 4 MiB sequential | 239.920 ms | 412.078 ms | 1.435 s | 2.485 s |
| 64 MiB sequential | 568.490 ms | 341.011 ms | 9.443 s | 10.257 s |
| 64 MiB split | 388.579 ms | 461.506 ms | 2.429 s | 3.871 s |

**The all-pairs performance target remains false.** UDM exit tails won only **4/9 pairs**, while full elapsed times won **8/9**. Sequential exit tails lost all three pairs. Split trial 2 also lost full elapsed time: **6.510 versus 3.871 seconds**. The unchanged 4 MiB control trial 2 lost its tail, **803.053 versus 516.843 ms**. No losing observation is removed or replaced with a rerun.

The separate symmetric post-exit output-flush endpoint won **7/9 tails** and **8/9 full elapsed runs**. It does not override the failed process-exit target. Durable tail medians UDM/IDM were 273.326/635.002 ms, 653.345/708.399 ms and 420.616/2029.795 ms for the three cases. Durable full elapsed medians were 1.446/2.719 s, 9.507/10.559 s and 2.461/5.573 s. These are different endpoints; neither measures every unrelated system service or GUI animation.

IDM returned launcher code 1 in every sample, matching the earlier retained runs. A new postprocessor initially assumed zero for both programs and stopped before any payload deletion. The correction retains all IDM codes without assigning an unverified exit-code meaning; UDM still requires zero, and actual exit, complete availability, size and independent digest are checked separately for both. No download or product test was rerun to change the outcome. [Correction record](validation/finishing-8.34/postprocess-correction.json)

The source confirms removal of the eligible trailing reader, but a causal speedup is not established by these observations. File/journal/catalog waits, request scheduling, cache/device state, IDM's existing profile and background load remain confounds. The previous 8.33 portable median was affected by large IDM outliers, so cross-run median subtraction is not an isolated code comparison. [Every pair, phase counter and limitation](validation/finishing-8.34/portable-comparison.json)

## Cleanup and preserved state

After verification, individually removed **289 generated fixture, comparison, compiler and tracing files / 2,567,783,263 logical bytes**, including the later D:, trace and compact-reader workloads described below. A hard-link group was removed only when all its links were enumerated owned payload paths; no outside link group remained. No recursive delete was used. Catalogs, journals, logs, screenshots and qualification evidence remain. All 26 ready binaries, 42 canonical core objects, previous 8.33 product payloads, source and shared media helpers remain. Raw ETL/XML/report exports were removed after decoding and sanitized rejection records were retained. [Cleanup](validation/finishing-8.34/cleanup-total.json)

Both personal catalog hashes matched the pre-comparison snapshot. All seven previously installed file hashes, including extension files, remain unchanged. Canonical media helpers match their earlier hashes and are referenced by a junction; no copies were made. No installer, deployment, Git commit, push or release was created.

## Performance and installation limits

The installed desktop remains 8.32; installing a newer build is deliberately deferred by the user. The original finishing target is incomplete. Next work must explain the remaining file/journal/catalog waits and retained slow samples, with kernel-quality IO attribution where needed, rather than another pacing adjustment. The controlled D: comparison below does not verify the default installed C:-parts/D:-output workflow. GUI completion timing and unrelated system-service work are separate requirements.

## Separate D: comparison

Ran the same unchanged qualified executable and original eighteen-transfer protocol with output, isolated UDM catalog and UDM parts on D:. This is a separate data-placement case; it does not replace the C: losses. IDM used its existing profile and D: output, but its temporary storage was not independently traced in this comparison. No storage-causal or default-profile equivalence claim follows. [Every D: pair and phase counter](validation/finishing-8.34/portable-d-comparison.json)

| Case | UDM median exit tail | IDM median exit tail | UDM median full elapsed | IDM median full elapsed |
| --- | ---: | ---: | ---: | ---: |
| 4 MiB sequential | 333.226 ms | 1032.524 ms | 1.536 s | 5.117 s |
| 64 MiB sequential | 465.850 ms | 553.831 ms | 9.405 s | 12.219 s |
| 64 MiB split | 777.216 ms | 600.311 ms | 4.177 s | 5.446 s |

All eighteen files matched their sizes and independent digests; every UDM saved Complete digest matched, settings were restored, and both personal catalogs matched their pre-comparison hashes. Exit tails won **5/9** pairs and full elapsed times **9/9**. All three split tails lost; sequential trial 3 also lost its tail, 559.056/485.084 ms. The separate post-exit output-flush endpoint won **9/9** tails and full elapsed runs, which does not erase the primary losses or establish universal parity.

The three D: split final `FlushFileBuffers` calls took **512.489, 522.962 and 365.880 ms**. File opens took 0.082–0.153 ms, sparse normalization was not needed, and late-hash waits were below 0.1 ms. Thus these samples prioritize actual final flush waits over opening, sparse normalization or late hashing. Their underlying kernel/storage/filter/scheduling cause remains unverified. The eighteen D: payloads were individually removed after checks.

## Privileged tracing attempts and coverage gates

The user approved tracing while keeping installation deferred. The first owned collection stopped within 19.236 seconds but its fixed five-second stop-confirmation timeout interrupted WPR export; only a header decoded. The next controller expired before approval and its stale identity did not start a collector or download. After correcting the stop deadline and pre-start approval window, a collection exported successfully with a **23.620-second** observed collector lifetime, inside its unchanged 45-second cap. All owned instances were subsequently confirmed off; unrelated system tracing sessions were left alone.

Export success was **not** accepted as IO attribution. The decoded trace contained 348,756 events and zero ordinary lost events, but no identifiable UDM PID events, circular last-dropped metadata, and schema warnings. Its owned-process coverage is inadequate. No disk, antivirus, filesystem filter or scheduler is named as the cause from this data. [Sanitized attempt records](validation/finishing-8.34/io-trace-attempts.json)

Memory logging overwrites old events when its ring fills; a zero ordinary lost-event counter does not establish that the target interval remains. [Microsoft WPR logging-mode documentation](https://learn.microsoft.com/en-us/windows-hardware/test/wpt/logging-mode)

Prepared a 64 MiB memory profile, preserving the exact original 16 MiB profile separately, and an attribution-only private helper that runs UDM last. The qualified performance helper is unchanged; AST checks verify an identical per-transfer body and identical main function except ordering. A fresh Windows elevation request for that capture was canceled before helper start; its waiting controller was stopped using a PID/creation-verified handle, with no collector or workload launched. The staged helper was removed. The larger profile is parser-validated, **not live-validated**. [Order amendment](validation/finishing-8.34/trace-order-amendment.json) · [Canceled request](validation/finishing-8.34/io-trace-canceled-64m.json)

All **21 supplemental recorder checks** passed, covering remaining-budget stop/export behavior, cleanup ownership, controller loss, roles, identity and profile constraints. A read-only D: plan created no directories, and three unsafe/existing workload paths were refused. Native version, all 494 qualified source inputs, 42 core objects and 26 binaries remain unchanged by these tooling amendments. [Tooling checks](validation/finishing-8.34/profiler-validation.json)

## Compact consumer prepared without another approval request

Added a standalone normal-token ETL reader using Windows `OpenTrace`, `ProcessTrace` and TDH. It emits bounded private JSON instead of writing an intermediate system-wide XML export. No logger-control or application-execution API is part of the reader. The scenario-specific structural gate requires a complete matching PID/image-basename lifetime, circular-history metadata before that lifetime, known owned file identities and exactly two successful paired `.data` flush operations. It rejects incomplete/ambiguous pairs, schema failures and resource-cap exhaustion. A passed structural gate would still not prove a disk/filter/scheduler cause.

All **20 normalized-event fault checks** passed, including missing lifetimes, repeated PIDs, image mismatch, ordinary loss, circular overwrite, incomplete metadata, missing/extra flushes, unresolved paths and capacity exhaustion. Five actual CLI checks refused missing/malformed input, PID zero, an existing output and output outside the private root; existing bytes were preserved.

A separate process-local fixture ran unelevated, enabled zero kernel providers, emitted three TraceLogging events and stopped its own session successfully. The native consumer read five total records, including metadata, exactly matching Windows `tracerpt`; TDH decoded all five and the consumer correctly rejected their missing kernel coverage. This is actual file/API integration evidence, **not live kernel FileIo/DiskIo validation**. The target kernel adapter remains unverified and the app's finishing target remains unmet. [Consumer source](../benchmarks/speed/finish_etl_reader.cpp) · [Validation](validation/finishing-8.34/etl-reader-validation.json)

Removed the tiny fixture ETL/reference CSV, malformed/sentinel inputs, producer executable and both compiler objects after checks: seven files / 4,414,461 logical bytes. Kept the ready compact consumer for the next valid owned kernel trace. No app source, version, installation, media helper, security setting or user catalog changed; no new UAC request was opened. [Reader cleanup](validation/finishing-8.34/etl-reader-cleanup.json)
