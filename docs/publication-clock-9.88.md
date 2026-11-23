# UDM 9.88: current publication delay on a common clock

This diagnostic follows the [repeated split comparisons](range-accounting-9.88.md). It changes no production code or version: normal native **9.88** and both browser source manifests **0.73.52** remain unchanged and uninstalled. A private instrumented App completes three owned downloads with exact output and saved-catalog hashes, then exits normally. This is attribution, not a normal-product speed comparison or installed/IDM GUI acceptance.

## Measured barrier and handle lifetimes

A private copy adds timestamp spans around existing calls while preserving their ordering. All timestamps use QueryPerformanceCounter, mapped to the parent origin clock with a measured error bound below 10 microseconds in these samples. The lossless ring is hash-verified, has no dropped events, and its decoded events and spans reconcile. The analyzer checks that catalog resolution precedes file-pin close and that every span endpoint exists on its recorded thread. [Validated analysis](validation/publication-clock-9.88/analysis.json) · [Lossless events and runs](validation/publication-clock-9.88/runs.json).

| Case | Body flush, ms | Intent write, ms | Catalog write, ms | Resolve to pin-close start, ms | File-pin close, ms | Last send to exclusive availability, ms |
| --- | --- | --- | --- | --- | --- | --- |
| single-4m | 88.743 | 91.659 | 34.859 | 0.035 | 0.071 | 138.878 |
| single-64m | 183.162 | 80.281 | 31.880 | 0.035 | 0.055 | 227.914 |
| split-64m | 157.296 | 159.713 | 78.209 | 0.044 | 0.043 | 383.694 |

Body flush and intent write overlap and must **not** be summed. In the 64 MiB sequential sample the body flush outlasts the intent: the owner waits another 103.137 ms at the body join. The catalog then takes 31.880 ms. In the split sample, body flush and intent occupy roughly the same 144–304 ms interval after last server send, followed by the 78.209 ms catalog write.

Closing the pins is **0.043–0.071 ms**, and the preceding resolution-to-close dispatch is **0.035–0.044 ms**. Exclusive-open observation follows pin release by 0.163–2.679 ms, including polling/scheduling. This rules out a large post-acceptance handle-close hold in these three observed paths. It does not justify releasing writer/name protection before the completed catalog is durably accepted.

## Split work before publication

The sequential final body writes end about 1 ms after last origin send and body settlement follows around 3 ms. The split final body write ends **133.878 ms** after last origin send; body settlement follows at **139.471 ms**, and intent/body publication starts around 144 ms.

Four `writeAt` spans crossing or following the last-send boundary take **102.130, 109.695, 110.274 and 27.846 ms**. The function's only blocking operation is the actual `WriteFile` call with an explicit offset. Three slow writes overlap each other; summing them would exaggerate the wall-time loss. This localizes a substantial part of the split pre-publication tail to in-progress file writes, rather than pin closing or a fixed presentation timer. It does not distinguish disk queueing, filesystem/filter work, synchronous-file serialization or scheduling within those calls.

`HttpJoinWaits.ReceiverThreadsSeconds` covers the receiver's whole lifetime, including transfer, and cannot be presented as a post-last-byte stall. Likewise, last server send is not proof that the client has already received every byte. The trace records those boundaries separately. [Individual native operation spans](validation/publication-clock-9.88/owned-split-64m/result.json).

## Next change admission

The next diagnostic should time the optional data-only flush operations against these exact file writes and capture the file-handle/open modes, while retaining a same-binary control. If a causal experiment changes optional writeback scheduling or flags, retain the required final body flush, intent/catalog acceptance, ownership and recovery barriers; reject it if total time or any finishing case regresses. Earlier optional-flush suppression already lost a historical ABBA comparison and is not a default fix. No additional pacing experiment or blanket flush removal is admitted by this result.

For the sequential path, target the measured body/intent/catalog operations with a concrete ownership-safe proposal before coding. Catalog serialization, pin close and UI presentation cannot be blamed for the measured body-flush interval. Keep normal repeated UDM/IDM total, availability, exit and explicit post-exit flush gates separate from the instrumented attribution. Extra split-byte overlap remains an independent range-ownership item.

## Build provenance, correction and cleanup

The private build captures its source/patch hashes before compilation, recompiles Core, Queue, Transfer, Duplicates and the unedited App entry with diagnostic flags, reuses 38 current core objects and the normal resource, and links one diagnostic program. Normal five products and all 42 original core objects verify unchanged. The first preflight correctly detects the newer `compare-completion.py` origin-recording amendment in the qualified inventory and stops before compilation. The corrected preflight records that sole research-script amendment explicitly; no production-source mismatch is waived. The initial failure record and both driver revisions are retained.

The current production inventory, diagnostic patches, compiler logs, runtime driver and hash-verified trace records are exported after sanitizing local profile paths. Installed programs/manifests, both personal catalogs and shared media tools match the eleven protected hashes. Three verified payloads totaling **138,412,032 logical bytes** are removed after hash checking. After export, **75 individually verified files totaling 178,898,164 logical bytes** are removed, including private snapshots, catalogs, diagnostic executable links, trace mappings and objects. Zero owned diagnostic executables remain before cleanup. Physical space and disk wear are not measured. [Audit](validation/publication-clock-9.88/audit.json) · [Cleanup receipt](validation/publication-clock-9.88/cleanup-receipt.json).

No installer, installation, UAC, IDM process launch, commit, push or release occurs. Overall inside/outside parity, installed/browser acceptance, the first-availability gap, repeated speed gates and unsupported media cases remain open.
