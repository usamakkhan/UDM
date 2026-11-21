# Accepted catalog text proof, 8.65 / browser 0.72.29

Completion, clean shutdown and destruction now compare the pinned catalog's contents with the exact serialization accepted by the latest durable save. This removes the second JSON parse/equality/destruction step from those disk proofs. The live-state comparison, real disk read, restrictive pin, sequence and pending-operation checks remain. The destructor's remaining pathname-based read is also replaced.

The measured catalog and Manager-stop stages improve substantially. **This does not establish complete download-time or IDM parity, or resolve the earlier multi-second disk stall.** Ordered body writes remain private after their previous no-regression failure.

## Cache ownership and proof contract

`saveCatalog` already owns both a detached snapshot and its serialized string. Only after actual coordinator publication and the existing receipt/publication notifications, and only while its sequence is still current, it moves both into the accepted checkpoint under the model mutex. `checkpointText` is private runtime state; it is not a new catalog field, receipt or permission to skip disk inspection. No extra serialization or string copy is needed to install it.

Failed saves retain the previous accepted bytes. A skipped stale save cannot replace a newer cache; a published but superseded save cannot install its older snapshot/text over the current sequence. Existing storage errors, catalog transactions, admission/publication overlays, disk-credit bindings and worker/lifecycle ownership still prevent inappropriate shortcut use. Missing accepted bytes force a real save.

Each eligible proof still opens and pins the actual file, reads it through the 8.64 bounded regular-file reader and compares all normalized text bytes. It does not trust cached file identity, length, timestamps or a hash alone. Read/type/size errors, changed/missing contents or missing ownership take the existing forced-save path. UTF-8 BOM normalization remains accepted. Alternate whitespace/key formatting now causes a conservative canonical save even when the JSON parses to the same value; it is not silently accepted as the original serialization.

The memory tradeoff is one retained serialization buffer per Manager. Its logical text length has the existing 32 MiB catalog cap; allocated string capacity may be larger. The buffer is moved from the already necessary save serialization and replaced on later accepted saves, then released with the Manager. This avoids repeated parsed temporary trees but does not make the retained buffer free or change the separate 64 MiB body-cache budget.

## Matched build and persistence checks

Native versions advance to 8.65 and both browser manifests to 0.72.29. Browser JavaScript and catalog schema are unchanged. Because Manager's private layout changes, the [dependency audit](validation/accepted-catalog-text-8.65/native-closure-audit.json) requires rebuilding all 24 C++ core units; eighteen qualified C codec objects remain. A separate normal Transfer object keeps ordered writes disabled. [Native](validation/accepted-catalog-text-8.65/build-result.json) and [normal](validation/accepted-catalog-text-8.65/default-build-result.json) receipts cover 526 inputs, ten products, 43 distinct core objects and five product entry objects. Twenty [current executable resources](validation/accepted-catalog-text-8.65/resource-versions.json) report 8.65; the separately rebuilt baseline comparison fixture reports 8.64.

All **1,992 selected assertions in sixteen C:/D: runs** pass: 53 catalog-checkpoint, 145 completion-checkpoint, 17 pinned-reader, 49 disk-credit, 55 admission-registry, 46 commit-coordinator, 558 recovery-catalog and 73 publication-recovery checks per drive. [Suite record](validation/accepted-catalog-text-8.65/suite-results.json).

Thirteen new catalog assertions exercise accepted-byte installation, failed-save retention, successful retry, missing-cache stop/destructor fallback, BOM versus formatting behavior, and a real held older save superseded by a newer accepted one. Existing held/failed/superseding completion, receipt, corruption, recovery and explicit-save checks continue to pass. This aggregate is affected qualification, not every historical suite.

Two actual normal App 4 MiB downloads on C:/D: pass independent file size/SHA-256, catalog, normal process exit and separate post-exit output flush checks. [App records](validation/accepted-catalog-text-8.65/app-smoke.json). They validate integration without repeating the earlier large-body matrix and do not establish an application speedup.

## Disk-proof component measurements

The [controller](../benchmarks/speed/accepted_catalog_read_control.cpp) compares 8.64's pinned read plus JSON parse/equality with a pinned read plus accepted-text equality. The canonical text is already bound to the expected snapshot before timing, as it is at real save acceptance. The common live-state check and handle acquisition are outside this component measurement. Both variants read the same immutable generated catalog, warmed once, with ten alternating pairs at each record count and drive. All **120 measured proofs** return exact equality.

Median wall milliseconds, ten samples per reader:

| Drive / records | Parsed proof | Accepted text |
| --- | ---: | ---: |
| C: / 128 | 0.724 | 0.020 |
| C: / 7,500 | 49.838 | 0.937 |
| C: / 30,000 | 218.740 | 5.117 |
| D: / 128 | 0.818 | 0.029 |
| D: / 7,500 | 50.728 | 1.009 |
| D: / 30,000 | 214.631 | 5.210 |

The [summary](validation/accepted-catalog-text-8.65/proof-comparison.json), [C: samples](validation/accepted-catalog-text-8.65/proof-C.json) and [D: samples](validation/accepted-catalog-text-8.65/proof-D.json) retain wall and thread-cycle observations. No slower sample is discarded. No accepted-text wall sample is slower in these sixty pairs; this bounded result is not a guarantee about future runs.

## Actual Manager stop and destructor

The [second controller](../benchmarks/speed/accepted_catalog_manager_control.cpp) runs the real `Manager::stop` method with 7,500 paused records, no active transfers and an accepted catalog. It includes the live snapshot and existing ownership/proof checks that the component benchmark excludes. Each version is warmed once, then measured ten times. Version-group order reverses across drives; these are grouped process runs, not alternating per-call cross-version trials. Setup and its initial real save are outside the measured stop calls. Both versions leave the accepted catalog hash unchanged and issue **zero additional catalog writes** during stop or destruction.

| Drive | 8.64 median stop (ms) | 8.65 median stop (ms) | 8.64 destructor sample (ms) | 8.65 destructor sample (ms) |
| --- | ---: | ---: | ---: | ---: |
| C: | 123.025 | 42.768 | 230.925 | 75.033 |
| D: | 119.567 | 44.643 | 159.609 | 72.553 |

There are forty measured stop calls and four destructor samples, all retained in the [summary](validation/accepted-catalog-text-8.65/manager-comparison.json) and individual manager records. The two versions have equal raw catalog lengths within each drive: 2,820,555 bytes on C: and 2,355,555 on D:, reflecting different fixture parent-path lengths. Do not treat the cross-drive absolute values as identical-input disk comparisons. Destructor values are one sample per version/drive, not medians.

The [build provenance](validation/accepted-catalog-text-8.65/manager-build-result.json) links the frozen 8.64 core with its exact Core.hpp reconstructed from the qualified before snapshot, and the 8.65 core with its current matching header. Three tiny forwarding headers reuse canonical shared headers instead of copying a source tree. SpeedMeter/PeriodicCheckpoint match the prior receipt; the shared JSON header is verified against current tracked Git bytes because that vendor header was not separately listed in the old receipt. That provenance limit is recorded rather than claiming an independently archived complete vendor snapshot. Include traces verify the selected Core.hpp. Both fixtures run normally and their generated catalogs are independently rehashed before cleanup.

These measurements establish a faster warm Manager-stop proof under this workload. They are not full App exit, active-download completion, cold-start, browser or IDM timings. The retained live snapshot work still costs time; the new proof is not O(1).

## Cleanup and older artifact retirement

[Fixture cleanup](validation/accepted-catalog-text-8.65/cleanup.json) removes **794 files / 107,034,611 logical bytes** in 26 owned fixture/replay roots, including comparison catalogs, temporary header/patch replays, fixture executables and entry objects. Two independently verified App payloads, another 8,388,608 logical bytes, were separately removed during their samples.

[Older artifact retirement](validation/accepted-catalog-text-8.65/retired-8.63-artifacts.json) removes seventeen unused 8.63 files / **117,364,895 logical bytes**: ten product executables, five product entries, the old resource and the unused Bridge object. No matching process was observed before cleanup. Latest and 8.64 baseline dependency sets were checked first and rehashed afterward. In particular, the 8.63 Transfer/DefaultTransfer objects still needed by the 8.64 baseline remain. Sources, compressed snapshots, compact evidence, toolchains and FFmpeg are retained. Past 8.63 product hashes describe the historical build; they no longer imply those retired executables are available.

Deletion checks resolved boundaries, refuses reparse traversal, verifies every hash and complete owned hardlink group, and touches no installed/personal file. Counts are logical bytes, not physical allocation or wear measurements.

[Final verification](validation/accepted-catalog-text-8.65/final-verification.json) rechecks 526 inputs, both benchmark sources, ten current products, 43 current core objects, five entries, three protected installed/personal files and 59 baseline 8.64 artifacts. The [six-file source delta](../benchmarks/speed/experiments/accepted-catalog-text-8.64-to-8.65.patch) is actually reversed in a temporary six-file subset; all six reconstructed prior hashes match before that subset is removed. [Patch verification](validation/accepted-catalog-text-8.65/source-patch-verification.json).

No installer, installation, browser deployment/reload, administrator prompt, push or release occurred. Installed UDM is unchanged. The earlier body-writeback/publication stall, balanced complete-download comparisons, installed IDM/browser workflows and broader feature/design parity remain open.
