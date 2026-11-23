# UDM 9.88: optional data-only flush control rejected

This follows the [common-clock publication attribution](publication-clock-9.88.md). The production App, five native products and 42 current core objects remain unchanged at **9.88**, with source extensions **0.73.52**. A private engine harness changes only the optional data-only operation through a captured environment switch. It is not the normal MFC App and cannot establish installed/IDM or GUI speed parity.

## Control and invariants

One hash-pinned diagnostic binary runs **normal / suppressed / suppressed / normal**. Each run downloads the same generated 64 MiB body at an owned origin with eight requested connections. Suppressed callbacks retain the worker bookkeeping but return without calling `NtFlushBuffersFileEx`. They do not represent physical flushes or durability acknowledgements. Actual data-only calls, suppressed callbacks and file writes have separate trace markers. The required final full body flush, durable intent, rename, catalog acceptance, checksum and owner shutdown remain unchanged in both modes.

All four output and saved-catalog hashes match, Complete is saved, the required final flush reports no error, and each process exits normally with all of its threads. Raw trace hashes, zero drops, event endpoints and span durations independently verify. [Validated control](validation/data-drain-control-9.88/analysis.json).

| Order | Mode | Total engine process time, s | Last send to availability, ms | Last send to exit, ms | Required final body flush, ms | Physical optional flush calls | Longest body write, ms |
| --- | --- | --- | --- | --- | --- | --- | --- |
| normal-a | normal | 2.915 | 325.869 | 369.607 | 184.130 | 7 | 276.558 |
| suppressed-b | suppressed | 2.427 | 707.401 | 748.479 | 645.739 | 0 | 251.682 |
| suppressed-c | suppressed | 2.511 | 750.395 | 795.912 | 511.037 | 0 | 294.666 |
| normal-d | normal | 2.778 | 278.221 | 308.889 | 189.562 | 7 | 358.224 |

Mean total time improves from **2.846 to 2.469 seconds**, but mean last-send-to-exit worsens from **339.248 to 772.196 ms**. Availability also worsens in both suppressed samples. Suppression moves much more work into the final flush instead of meeting the requirement to improve total time without losing finishing. **Do not adopt it.** These four engine observations are too few and too narrow for broad performance claims. [All individual observations](validation/data-drain-control-9.88/runs.json).

## What changes the diagnosis

Normal samples have 151 and 131 writes of at least 20 ms; suppressed samples have 19 and 49. Many normal slow writes overlap real optional flush operations. However, writes still take **251.682 and 294.666 ms with zero physical optional data-only calls**. The optional operation can contribute to the observed workload tradeoff, but cannot be a necessary explanation for every slow write. OS cache writeback and other storage/filter/scheduling effects remain possible; this user-mode trace does not distinguish them.

The suppressed callback scopes are still timed. Any overlap count referring to those scopes must not be reported as overlap with physical I/O. Their legacy callback statistics show seven callbacks each, while the explicit physical-call markers correctly show zero. Concurrent write and flush durations must not be added as serial wall time.

## Build correction and provenance

Only a private Transfer translation unit and a small engine entry are compiled, reusing 41 normal core objects and the normal resource. The first harness compile attempted private `Manager::start` and failed before any runtime. The corrected harness uses public Resume and Tick. Its old source, failed compile log, before/after hashes and pre-runtime amendment are retained; the already completed Transfer object is hash-verified and reused. No access rule or production class is weakened.

The original qualified inventory detects only the later benchmark origin-recording amendment. Native and browser source bytes remain unchanged. Launch inputs record the binary, driver, origin and helper hashes before the four runs. The new analyzer is read-only and can replay the sanitized lossless records after private artifacts are deleted.

## Next implementation gate

Keep the optional operation enabled in production. Next inspect allocation/open modes and the actual buffered-write/queue/filter path that still stalls without it. A new change needs an identified owner, a same-binary control, complete-byte/recovery qualification, and normal App comparisons for total time, first availability, process exit and explicit post-exit flush. Do not replace this failed control with another unmeasured pacing rule or remove durable acceptance barriers. Sequential body/intent/catalog waits, split byte overlap, installed/browser acceptance and the wider IDM UI/feature inventory remain open.

## Cleanup

Four verified payloads totaling **268,435,456 logical bytes** are deleted after hash verification. After sanitized export, **65 individually path/type/hash-checked files totaling 52,086,442 logical bytes** are removed, including private catalogs, trace mappings, source copies, diagnostic executable links and objects. Zero owned diagnostic processes remain at cleanup. Installed programs/manifests, both personal catalogs, shared FFmpeg/FFprobe, normal products and all 42 current core objects are unchanged. Physical reclaimed space and wear are not measured. [Audit](validation/data-drain-control-9.88/audit.json) · [Cleanup receipt](validation/data-drain-control-9.88/cleanup-receipt.json).

No production fix, version increment, installer, installation, UAC, IDM launch, commit, push or release occurs in this research pass.
