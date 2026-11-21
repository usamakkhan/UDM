# Failed publication keeps its owner: UDM 9.64

UDM 9.64 fixes a real worker-lifetime bug found while reviewing 9.63. A publication exception could reach the worker's error handler, show Failed and erase active ownership before the still-live prepared owner's destructor joined its asynchronous body-result task. Another action could then observe an idle download while the old task and cleanup were not yet retired.

The worker now constructs the prepared owner in an in-place optional scope. On a caught exception it resets that scope **before** taking the model lock, updating failure status, saving the failure or erasing active ownership. Reset joins the owned task and performs the existing prepared cleanup while the original worker still owns the job. The local completion flag is cleared so failed code cannot use the reset owner in the completed-checkpoint branch. On success, the owner remains alive through checkpoint proof, independent-finishing policy and notification, as before. The optional scope adds no heap allocation.

Five native products build at 9.64, paired source extension manifests advance to 0.73.28, and 1,289 affected checks pass. This is a failure-lifetime fix. It does not change the successful I/O policy and does not claim a new successful-download timing result. The unresolved [9.63 speed gates](publication-source-snapshot-9.63.md) remain admission blockers; 9.64 is not installed or promoted. Installed app 8.68 and extension files 0.72.32 remain unchanged.

## Reproduction and verification

The new fixture runs the actual Manager/Core worker against a private loopback HTTP server. At each selected publication point, its test-only friend wrapper retains the real body future inside an owned result-producer task and gates result delivery. The native writeback is allowed to finish; this is an ownership test, not a simulated physical-disk latency measurement. The callback then throws an owned publication error. The test polls active ownership and failed status while result delivery remains held, releases the task, waits for failure retirement and joins the real worker.

The same fixture linked against frozen 9.63 fails after two assertions at `publication-preflight-begin`: active ownership has retired and Failed is visible while the result task is held. Linked against 9.64 it passes twelve assertions covering four points:

- Before source preparation.
- After source preparation.
- After the intent temporary file is flushed.
- After the durable publication intent is written.

In all four corrected cases, active ownership remains present and Failed is withheld while the task is held. After release, the owned task retires, failure becomes visible and no final output is published. Existing publication, recovery, HTTP completion, independent finishing, availability and launch checks also pass, for 1,289 assertions across twelve suites. The early twelve-check precheck is retained without double-counting it.

Two earlier probes are preserved. The component-only exception probe did not establish the actual Manager retirement behavior. The first actual-worker probe incorrectly awaited a final completion event on ordinary failure; that event is not emitted by the existing code. Its raw ownership observations are retained, but it is not counted as the valid negative control. The corrected oracle polls the actual model state. The accepted frozen control and current build use the same fixture source hash.

[Before/after real-worker evidence](validation/publication-failure-owner-9.64/before-after.json), [all suites](validation/publication-failure-owner-9.64/suites.json), [captured build](validation/publication-failure-owner-9.64/build.json), [control corrections](validation/publication-failure-owner-9.64/control-corrections.json).

## Remaining work

The [captured-source fix in 9.63](publication-source-snapshot-9.63.md) is retained. Its three clock proofs show real body/intent overlap, but the normal comparison still loses all nine IDM exclusive first-availability pairs, and sequential/split finishing regressions remain. 9.64 has no new successful-path speed comparison. Neither broad parity nor the user's no-slower-case requirement is established by these lifetime tests.

Continue by identifying the remaining storage waits and evaluating the actual persistence operations, with documented durability and recovery behavior. Keep full elapsed, exclusive availability, process-exit and post-exit durability endpoints separate. Changes to a durable completion receipt or catalog acknowledgment require explicit crash/failure qualification. Installed/browser/dialog comparisons and full inside/out IDM parity remain open.

## Build and cleanup

Only Core and the version-dependent Bridge core units are recompiled; five frontends/resources are built at 9.64. Source and dependency inventories, compiler output, same-fixture negative control and corrections are retained after sanitization. Temporary failed-transfer catalogs, test binaries, compiler objects and control inputs are removed after export. Current and frozen-baseline products, required core objects, installed files, both personal catalogs and shared media helpers are preserved. No installer, installation, commit, push or release is created.
