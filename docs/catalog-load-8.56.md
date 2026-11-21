# UDM 8.56: adopt parsed catalog records

Native source is **8.56**, and both extension source manifests are **0.72.20**. Browser JavaScript is unchanged. Five native products built; installed desktop remains **8.32**. Installation remains deferred, without installer creation, extension deployment, push or release.

## Redundant startup ownership

The Manager constructor parses `state.json`, validates its outer structure, preserves the first-load backup and initializes defaults before loading Jobs. The old loop passed each parsed record as a const lvalue to the by-value `Job` constructor. That allocated a deep JSON copy even though the constructor moved its parameter into `Job::data` and Manager immediately cleared the entire source Downloads array after loading.

The loop now transfers each mutable parsed record into the existing Job constructor. This removes the additional deep record copy. The array retains its structure while each element is consumed; no external caller sees a partly initialized Manager. All records are validated and normalized through the existing constructor, scanner recovery and interrupted-status rules before recovery proceeds. The public Job constructor signature, nested audio/video creation, catalog schema and snapshot format remain unchanged.

The source array is still cleared before file-operation, replacement, restart, synchronization, link-conversion and response recovery. Snapshots still serialize Jobs, including their live nested tracks, into Downloads. No change was made to publication flushes or durability barriers.

## Failure behavior and qualification

The original saved catalog is not rewritten during record adoption. If a late record or nested track throws, member destruction releases the partially constructed in-memory state; Manager's destructor does not run on failed construction and cannot save an incomplete history. The existing first-load backup is created after outer schema validation, before the record loop. A preexisting backup remains untouched.

The new **112-check fixture passes on both C: and D:**. It exercises both immediate and desktop recovery modes; original record order and identifiers; 2 KiB descriptions, unknown nested metadata and segment arrays; completed queue membership; reserved filename normalization; interrupted Downloading, Pausing, Verifying, Merging, Resolving and Awaiting-confirmation states; stale adaptive size removal; independent audio/video identities; exact save/reload behavior and first-load backup bytes. Actual malformed late IDs, duplicate segments, malformed nested audio IDs and unsupported schemas are refused. Both original catalog and applicable backup bytes remain exact, without temporary or replaced-catalog files.

All **1,863 checks in ten final-binary-matched runs** pass. Retained regressions cover catalog checkpoints, completed checkpoints, recovery catalog transactions, publication recovery, synchronization replacement and queue startup rollback. Recovery catalog and publication recovery also run on D:. The initial focused 112-check probe passed and is excluded from the final aggregate. [Suite records](validation/catalog-load-8.56/suite-results.json), [validation notes](validation/catalog-load-8.56/validation-notes.json).

Five products, seven backend fixtures and one candidate startup diagnostic were built from **513 frozen inputs and 42 core objects**. Core and Bridge were rebuilt; twenty-two qualified native units and eighteen external C units were retained after owned include-closure verification. Every candidate output's file version is 8.56. An accurate 8.55 diagnostic was built against its qualified objects and resource before changing headers. The six-input source delta from 8.55 reconstructs both sides, including raw line endings. [Build evidence](validation/catalog-load-8.56.json), [source reconstruction](../benchmarks/speed/experiments/catalog-load-8.55-to-8.56.json).

## Real startup observations

Both engines use the exact same diagnostic App entry source. Its only addition to production App is a guarded normal close after the existing initial-refresh-returned timing marker. There are no timing injections inside Manager. Each comparison uses identically reseeded owned catalogs, isolated instance tags, three balanced fresh-process pairs (A/B, B/A, A/B) per history size and the same 2 KiB description on each completed nonmember. All eighteen processes exited normally and preserved every identifier, status and description.

Median milliseconds from app initialization were:

| Completed records | Manager ready 8.55 | Manager ready 8.56 | Post-pipe marker 8.55 | Post-pipe marker 8.56 |
| --- | ---: | ---: | ---: | ---: |
| 0 | 9.3438 | 9.9139 | 172.9787 | 180.3082 |
| 2500 | 156.4749 | 141.8676 | 333.6385 | 320.9829 |
| 7500 | 447.2012 | 406.3427 | 626.3996 | 584.0564 |

At 7,500 rows, manager-ready improved by **40.8585 ms**; initial-refresh-returned improved from **651.7352 to 609.4440 ms**. At 2,500 rows one candidate run was slower (191.0652 ms to manager-ready) than either neighboring candidate trial; it is retained. With zero rows both median stages were slightly slower in the candidate. These observations support reduced startup work for the measured large histories, not a universal speed-win or statistical-significance claim.

Manager-ready includes catalog reading, JSON parsing, defaults, adoption and recovery. The pipe marker records constructor return, not an external request handshake. OS caches may be warm; only three samples per engine per count were taken. Hidden startup does not measure visible paint, File Info appearance, reboot-cold startup, installed behavior, IDM comparison or the duration of all shutdown/background work. [All results and decoded traces](validation/catalog-load-8.56/startup-comparison.json), [diagnostic patch](validation/catalog-load-8.56/startup-diagnostic.patch).

An ordinary source-app 4 MiB transfer separately passed independent length/hash, the persisted Complete digest, one origin request, normal process exit after owned workers and an explicit completed-output flush. Its payload was removed immediately. Correctness work may have been running concurrently, so its timing is an integrity/lifecycle smoke rather than a performance comparison. [App record](validation/catalog-load-8.56/app-4m-smoke.json).

## Cleanup and remaining work

Removed **793 owned generated files / 333,714,036 logical bytes**, plus the separate 4 MiB download payload, and 754 empty owned directories. This includes synthetic catalogs, fixture payloads, backend/diagnostic executables and entry objects. Canonical roots were cached, resolved paths stayed within explicit owned roots, reparse points were refused, and only complete hard-link groups were removed. Five products, Core/Bridge objects, forty shared core references, resources, compact evidence, qualified prior references, shared tools, personal catalogs and installed app remain. Logical size does not prove physical reclaimed allocation. [Cleanup receipt](validation/catalog-load-8.56/cleanup.json).

Next isolate the remaining constructor work rather than extrapolating these medians. Filename validation currently constructs its reserved-name regex per record; that is an inspected candidate for measurement, not a proven dominant cause. JSON parsing, nested track copies and recovery also remain synchronous. Further work must retain invalid-name rules, Unicode behavior, failed-load preservation and recovery acceptance.

The original final-file flush delays, kernel cause attribution, installed/browser acceptance, external namespace/caller audits and IDM finishing-speed parity remain open. No additional elevated trace was run for this startup change, and no claim is made that the original finishing stall is fixed.
