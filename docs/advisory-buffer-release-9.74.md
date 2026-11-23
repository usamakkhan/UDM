# Completed hash-buffer release: UDM 9.74

UDM 9.74 / source extensions 0.73.38 removes a measured completed-buffer release from the publication owner's hash retrieval. Five native products build; all 1,522 affected assertions and 119 buffer/cache checks pass. All eighteen normal-product comparison files verify. **The overall no-regression gate still fails, so this remains an uninstalled candidate.**

## Cause and change

In 9.73 the value-owned async callable captures the existing 64 MiB body allocation. Under the tested MSVC runtime, completing the callable does not destroy that capture. Retrieving the result releases the async state and frees the allocation on the publication owner. This explains a specific approximately 3 ms owner cost; it does not explain every body/intent/catalog flush delay.

A standalone component check uses the actual `BodyBufferStorage`, CNG `StreamHash` and shipped compiler runtime, without linking Core objects. Sixteen 64 MiB in-memory samples alternate capture retention and explicit worker release. Every digest matches an independent Python SHA-256 oracle. Weak-reference and competing-budget reservations demonstrate that retained captures keep both storage and the single allocation budget occupied until retrieval, while worker release retires both before retrieval. No large payload is written to a download file by this component check; compiler/report outputs still exist.

Across eight samples of each lifetime, median retrieval takes 2.8785 ms with capture retention and 0.0028 ms with worker release. Worker release itself takes 2.83065 ms and is included in its total task duration. This is a component lifetime result, not a complete download or IDM comparison. The separate validator check measures about 2.25 microseconds per temporary-regex validation versus 0.062 microseconds for a scanner, with matching byte/length checks. Validation is not the millisecond bottleneck; production validation remains unchanged.

9.74 explicitly resets completed body storage inside the owned hash callable after its success/failure handler. It records `AdvisoryHashBufferRelease` and includes release in `AdvisoryHashSeconds`. It does not detach work, retain the large allocation after completion, copy a second body or omit deallocation from full process lifetime. The existing task joins, source binding, checksum policy, flush requirements, catalog acceptance and recovery remain in place. The budget token still retires only after the bytes are destroyed.

## Normal-product comparison: targeted wait reduced, full gate failed

Three alternating trials compare normal 9.74 with frozen 9.73 for 4 MiB sequential, 64 MiB sequential and 64 MiB split. Every size/hash and saved Complete record verifies. The harness measures exclusive availability, all observed app-process exit, full elapsed and a symmetric explicit post-exit file flush. No compiler runs during the matrix; all losing samples remain.

The three 9.74 split owner waits are 0.0292, 0.0222 and 0.0237 ms; the paired 9.73 waits are 2.9895, 3.1765 and 3.0809 ms. Thus the median measured owner wait changes from **3.0809 ms to 0.0237 ms**. Worker buffer-release durations of 2.7868, 2.9196 and 2.8282 ms remain recorded inside the task's total duration. Normal tasks can hash different remaining prefixes; their total hash durations are not a controlled full-body CPU comparison.

Medians in milliseconds:

| Case | Exclusive availability, 9.74 / 9.73 | Exit tail, 9.74 / 9.73 | Full elapsed through exit, 9.74 / 9.73 | Tail through post-exit output flush, 9.74 / 9.73 |
| --- | ---: | ---: | ---: | ---: |
| 4 MiB sequential | 126.463 / 113.300 | 173.469 / 165.605 | 1,023.167 / 1,033.108 | 196.385 / 198.042 |
| 64 MiB sequential | 200.464 / 183.630 | 248.431 / 232.538 | 8,963.482 / 8,943.606 | 269.377 / 259.919 |
| 64 MiB split | 240.574 / 350.297 | 289.947 / 400.925 | 2,360.663 / 2,452.300 | 317.709 / 424.132 |

9.74 wins four of nine exclusive-availability pairs, five of nine exit-tail and post-exit-flush-tail pairs, and six of nine full-elapsed pairs through either exit or the extra flush. The median split result improves, but one split trial still loses: 419.396 ms exit tail versus 382.541 ms. The 4 MiB and 64 MiB sequential exit-tail medians regress. Three samples per case on this shared host do not establish statistical equivalence or attribute the much larger split difference to a roughly 3 ms resource fix. **Do not claim that every case is faster.**

There is no fresh IDM run. Its existing current-session process remained running; the request to exit it normally was still pending. A fresh Windows-control attempt again failed with `failed to write kernel assets: The system cannot find the path specified. (os error 3)`. No process was forced to stop and no IDM settings were read/changed by the explicitly derived UDM-only harness. Installed app 8.68, extension files 0.72.32, personal catalogs and shared ffmpeg/ffprobe remain unchanged.

## Evidence and remaining work

The component passes 326 checks; the native build passes 1,522 assertions in twenty-one affected suites; the additional cache run passes 119 checks. These are scoped checks, not a complete project test-suite claim. Five exact pre-change source files plus unchanged current inputs reconstruct the 690 captured 9.73 inputs; the new build captures 691 inputs, including the component fixture.

[Component provenance](validation/advisory-intent-9.74/owner-cost/inputs.json), [component checks and all samples](validation/advisory-intent-9.74/owner-cost/results.json), [validator measurements](validation/advisory-intent-9.74/owner-cost/validation-cost.json), [native build](validation/advisory-intent-9.74/9.74/build-result.json), [native suites](validation/advisory-intent-9.74/9.74/suite-results.json), [cache checks](validation/advisory-intent-9.74/body-budget/results.json), [all eighteen normal downloads](validation/advisory-intent-9.74/comparison/results.json), [all failed/passing pairs](validation/advisory-intent-9.74/comparison/analysis.json), [final audit](validation/advisory-intent-9.74/final-audit.json).

Next work must investigate the actual sequential body/intent/catalog flush dependencies and retain the same full-lifetime endpoints. The known source-entry/name-rename ownership intervals and remaining media-assembly cleanup audit from [9.71](relocation-ownership-9.71.md) remain open. Installed validation, live GUI/browser comparison and broad IDM parity are unproven. This component fix does not justify another pacing change, skipped barrier or selected winning rerun.

All eighteen verified matrix outputs were removed immediately: 830,472,192 logical bytes. After sanitized export, 58 disposable binaries/objects and 274 selected payloads were individually removed with path/type/hash checks, totaling another 325,811,088 bytes. Combined logical length is 1,156,283,280 bytes (about 1.08 GiB); physical free space was not measured. Current five products, 42 referenced core objects, selected older comparison products and small evidence/metadata files remain. No installer, installation, commit or push was performed.
