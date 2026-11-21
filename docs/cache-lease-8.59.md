# Cache budget ownership, UDM 8.59 / browser 0.72.23

The optional full-body cache now uses a thread-independent atomic budget token. It can safely release that token when a serialized download worker disables caching or when cache destruction is handed to another thread. The process still admits one body buffer of at most 64 MiB, and optional-cache refusal/failure retains the existing disk fallback.

## Defect and implementation

In 8.58, transferHttp acquired a std::unique_lock<std::mutex> in WrittenBodyCache::reset before creating the range workers. A successful range write calls WrittenBodyCache::wrote from a different worker, under Manager's model mutex. Invalid notification bounds, excessive coverage fragmentation or bookkeeping allocation failure can call disable from that worker. The old disable unlocked the process-budget mutex acquired by the parent thread. The [C++ mutex requirements](https://eel.is/c++draft/thread.mutex.requirements#21) require the calling thread to own a mutex it unlocks. The source establishes this ownership violation; no deterministic old crash or hang is asserted, and the old undefined-behavior path was not executed as a baseline benchmark.

The replacement acquires the single global atomic flag with compare_exchange and releases it after discarding the old bytes and coverage storage. A denied cache never owns a token and cannot release another cache's reservation. Destruction releases the budget. Copying and moving cache objects are explicitly disabled so an owning flag cannot be duplicated accidentally. The cache's data operations remain serialized by the owning transfer, and readers still wait for its writers to join. This does not make one cache object generally safe for simultaneous unsynchronized calls.

No flush cadence, file-allocation policy, final durability barrier or HTTP request plan changed. The slower sparse experiments remain retired. A bounded fresh first GET would also need careful response-ownership and one-use-link fallback changes; this correctness increment does not claim to eliminate origin overfetch.

## Validation

The cache suite passes 79 checks on each drive. New cases hand failure and destruction to a different thread, check that denied-cache destruction preserves the real owner, and run 24 rounds with eight simultaneous contenders. Each round admits exactly one owner until release and then admits a successor. Compile-time checks reject copying and moving ownership. Existing byte coverage, cap, binary data, overlap, invalidation and fallback checks remain. Allocation-exhaustion injection is not claimed.

All 1,263 selected checks pass in twelve final-binary-matched runs:

| Suite | C: | D: |
| --- | ---: | ---: |
| WrittenBodyCache | 79 | 79 |
| WrittenPrefix | 13 | 13 |
| PrefinishWriteback | 51 | 51 |
| CompletedCheckpoint | 145 | 145 |
| CatalogCheckpoint | 40 | — |
| RecoveryCatalog | 558 | — |
| PublicationRecovery | 73 | — |
| SyncReplacementRecovery | 16 | — |

The native build freezes 516 inputs, recompiles Transfer for the changed cache header and Bridge for the version header, and retains forty qualified core objects. All five production programs and eight fixtures were built and their resource versions checked as 8.59. Both extension source manifests advance to 0.72.23; their JavaScript is unchanged. Exact inverse source reconstruction matches the qualified 8.58 hashes for all five changed product inputs. [Build and validation receipt](validation/cache-lease-8.59.json), [source reconstruction](validation/cache-lease-8.59/source-reconstruction.json), [reviewable source delta](../benchmarks/speed/experiments/cache-lease-8.58-to-8.59.patch).

A real, uninstrumented 64 MiB split download used the new UDM.exe in an isolated D: catalog. Its final phase confirms write-buffer advisory hashing, exercising the full-body cache. Length, independent SHA-256, saved Complete record, normal application exit and a separate explicit post-exit output flush all pass. The payload was deleted. The measured exit/durability tails were 658.133/675.068 ms; this single smoke is not an IDM comparison or evidence of a finishing-speed improvement. [App lifecycle and integrity record](validation/cache-lease-8.59/app-64m-cache-smoke.json).

## Cleanup and delivery status

Individually checked owned fixtures, complete hard-link groups and redundant fixture/entry objects were removed: 407 files / 126,169,357 logical bytes, plus the separately deleted 67,108,864-byte smoke payload. Five products, the forty-two core inputs and resource file remain. Personal catalogs and the installed app retain their captured hashes. [Cleanup](validation/cache-lease-8.59/cleanup.json).

Installed UDM remains 8.32. No installer, installation, extension deployment/reload, push or release was performed. The original finishing-delay, kernel-cause attribution and installed IDM parity work remain open.
