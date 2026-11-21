# Ordered storage faults: whole-Manager qualification

Status: test-only follow-up to [the 8.62 candidate](ordered-body-8.62.md). Application definitions remain 8.62 and extension source manifests remain 0.72.26. No production source or object changed, so no product increment is added. Ordered writes remain disabled in normal builds. Installed UDM remains 8.32.

## Scope and injection boundary

[`ordered_storage_fault_control.cpp`](../benchmarks/speed/ordered_storage_fault_control.cpp) links the exact qualified 8.62 private-build core objects and runs the actual Manager HTTP/IO/checkpoint/recovery pipeline. It temporarily replaces WriteFile import slots **in its own fixture executable only**, applies faults only to the exact owned parts-file path, and restores the slots after Manager stop and joined IO. DLL images, other processes, installed applications and on-disk production objects are untouched. No administrator token, trace session, service or UAC change is required.

The mechanism uses the executable import-address table described in Microsoft's [PE format reference](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format). The write adapter verifies both API success and the exact completed byte count, following the [WriteFile contract](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-writefile). These instrumented runs qualify failure handling; they are not performance measurements.

## Final cases and independent oracles

The final binary passes **126 assertions across fourteen scenarios**: seven cases on each of C: and D:. Each case uses an 8 MiB deterministic loopback body; twelve subsequent recovery invocations run without installing the import shim. [All results](validation/ordered-storage-fault-8.62/results.json), [build identities](validation/ordered-storage-fault-8.62/build-result.json), [request recovery proof](validation/ordered-storage-fault-8.62/request-recovery-proof.json).

| Case on each drive | Fault and required result |
| --- | --- |
| control | Pass through every real write; complete via the actual pipeline and match final bytes/hash. |
| fail | The third body write returns failure without writing bytes. Earlier acknowledged bytes remain credited; the original write error survives HTTP cancellation; no later body operation or output publication occurs. |
| partial-fail | Write a deliberately corrupt half-prefix in the third operation, then return failure. The corrupt bytes exist in the part but receive no saved credit. Recovery must revisit and overwrite them. |
| short-success | Write the same corrupt half-prefix but return success with a short byte count. Exact-count validation rejects it, retains only earlier credit and prevents publication. |
| hold-pause | Hold the first body write while accepted/network progress runs ahead. A real save contains only already written prefixes. Pause/stop waits for IO while model snapshots remain usable; release preserves accepted data and saves exact reconciled progress. |
| hold-rollback | During the early held write, inject failure after flushing the pause checkpoint temporary file. The real published catalog stays unchanged, live state rolls back and the generation remains uncanceled. A subsequent successful pause and release preserve the exact bytes. |
| hold-eof | Hold the final body write after reception. Pause still prevents publication and completion acknowledgment, even when the writer had already selected its finish policy. Release permits joined IO and an exact saved body for later recovery. |

For every non-control case, the Python oracle reads the actual saved part before recovery, compares each credited segment prefix against the independently generated source pattern, and computes coverage from successful full WriteFile operations. Saved Received/Done must agree exactly. For partial/short cases, it also verifies the deliberately wrong physical prefix and proves that no saved segment credits any of those bytes.

All six ordinary write-failure cases retain exactly 49,152 earlier successful bytes. Their first recovery response is HTTP 206 starting at byte 49,088, revisiting the failed offset at 49,152 with the existing 64-byte overlap. That is verified from actual origin requests, not inferred from a final status. Every recovered final file is independently checked in full for 8 MiB length and the expected SHA-256, followed by an explicit post-exit flush. The failed operation cannot be silently skipped using its physical file length or a cached advisory hash.

Early hold cases retain partial bodies (7,421,952 bytes in the final C: runs; 7,421,952/7,913,472 in the final D: early pause/rollback runs). EOF holds retain all 8,388,608 bytes while the parent remains Paused and unpublished. Recovery results and origin probes are recorded rather than claiming that full saved-body recovery issues zero requests.

## Rejected assertion and EOF semantics

The first held-IO assertion incorrectly required the component's Paused flag. In that run, network reception had ended and the writer had selected finish before the parent pause arrived. The parent correctly ended Paused, with no output or acknowledged completion; all written bytes remained valid. A component reporting complete written coverage is distinct from publishing or completing the parent job.

The [rejected assertion result](validation/ordered-storage-fault-8.62/rejected-assertion-result.json), [failure/origin record](validation/ordered-storage-fault-8.62/rejected-assertion-failure-private.json) and [build identity](validation/ordered-storage-fault-8.62/rejected-assertion-build-result.json) remain. The final fixture deliberately holds early IO and EOF separately, checks the parent publication boundary, and permits the already-selected internal finish policy. Earlier pilot/intermediate runs are excluded from the 126-assertion aggregate. No production fix was needed for this distinction.

## Cleanup and preservation

[Cleanup](validation/ordered-storage-fault-8.62/cleanup.json) removed all twenty owned fixture roots, including intermediate/pilot data, the test executable and object: **132 files / 178,451,727 logical bytes**. Each deletion was scoped, hashed and checked for hardlinks/reparse points. Logical bytes are not a physical freed-space claim. Source and sanitized evidence remain; the small rejected-source snapshot is compressed privately.

[Preservation verification](validation/ordered-storage-fault-8.62/preservation.json) rechecks all 524 qualified inputs, ten qualified normal/private products, 43 distinct core objects, the fixture source and three protected installed/personal files. The existing 1,780-check 8.62 qualification remains separate from these new 126 instrumented assertions. No installer, installation, extension deployment, push or release was performed.

## Remaining gates

The scoped zero/partial/short-write, early/EOF hold and pause-checkpoint rollback gate now passes through the real Manager pipeline on both drives. This does not prove all filesystem failure modes, power-loss durability or a speed gain. The existing explicit final flush remains required.

Next exercise refresh/reset, folder changes/removal and resource-change repair with queued data and old checkpoints; add direct shutdown and cancellation races beyond the tested pause/stop sequence. Qualify one-use/no-range and denied/changed response cases, then balanced whole-lifetime sequential/split performance and installed IDM/browser workflows when deployment resumes. Default activation remains deferred until those relevant gates pass.
