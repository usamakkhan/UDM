# Manager-owned Exit acquisition — UDM 9.47

Replace timed Exit model-lock retries with direct acquisition on a Manager-owned worker. Cancel a destroyed window without waiting for that acquisition; join all retained workers before Manager's final checkpoint. Build five native 9.47 products and pass 867 backend plus 1,360 native UI assertions. In three rotating rounds, pre-unlock-marker-to-acquisition median changes from 15.82 ms in 9.46 to 0.03 ms in 9.47. Controlled total-cleanup medians are 839.9 / 864.4 / 843.1 ms for 9.45 / 9.46 / 9.47. The matched median total gate remains open; universal and installed IDM parity remain unproven. Both source extension manifests advance to 0.73.11; installed 8.68 remains unchanged.

## Blocking evidence

The 9.46 event-driven observer showed a 21.1 ms median total-cleanup regression in its first matched comparison. That failed gate remains in the 9.46 report. Additional release-boundary controls retained all runs: three 9.46 observations spent 16.02–16.13 ms between the writer's recorded release and acquisition, despite different writer sleep and later teardown times. The earlier single observation with a 0.18 ms gap also remains preserved. This supports removing the retry wait; it does not identify every source of shutdown variance.

[Initial failure and all preflight controls](validation/managed-exit-9.47/preflight/runs.json), [prior failed total gate](exit-layout-9.46.md).

## Ownership and final-save behavior

MainExitLayoutTask now registers its dedicated worker with Manager before launching it. The worker waits directly on the existing recursive model mutex, checks cancellation after acquisition and before merging, and publishes its result after unlocking. It merges only the captured layout. The available-lock fast path remains immediate. Ordinary download locks and their unlock paths are unchanged.

A closing window cancels its operation and releases its own references. A blocked acquisition has only owned values and a weak notification, with Manager retaining the thread. Manager cancels and joins registered work outside its registry mutex before its existing final checkpoint or destruction. No worker is detached. Registration is rejected after shutdown begins; a failed shutdown reopens registration for retry. A worker cannot stop its own Manager or join itself. Stop before launch retires the registered owner so a later launch cannot escape shutdown.

The accepted File Info decision flag remains a separate deferral with a cancellable wait. This increment removes timed acquisition retries for normal mutex contention, and does not claim every deferred operation is free of timers. The surrounding integration, transfer and file-publication shutdown steps are retained.

## Functional qualification

867 backend assertions cover new owner/launch/join rules, held-model cancellation, independent registry access, destruction, durable state preservation and real final-save failure/retry, together with existing checkpoint, publication, writeback and body-cache behavior. 1,360 current native UI assertions cover actual Exit, close-to-tray policy, repeated commands, failure/retry, direct window destruction, menus, focus and modal windows. Window checks run on separate owned desktops without switching the interactive desktop or injecting user input.

[All backend assertions](validation/managed-exit-9.47/backend-checks.json), [UI totals](validation/managed-exit-9.47/ui-summary.json), [frozen build inputs](validation/managed-exit-9.47/build.json), [five product versions](validation/managed-exit-9.47/product-versions.json).

## Matched shutdown observation

All three statically linked versions use identical observer-source bytes and a deliberate 750 ms held model lock. Three rounds rotate each version through every position. Measurements retain actual writer sleep, post-release acquisition, window closure, Manager/background cleanup and whole-process wall time. Compiler activity finishes before observation.

| Version | Median pre-unlock marker to acquisition | Median through Manager cleanup |
| --- | ---: | ---: |
| 9.45 synchronous baseline | 0.07 ms | 839.9 ms |
| 9.46 timed retry control | 15.82 ms | 864.4 ms |
| 9.47 direct acquisition | 0.03 ms | 843.1 ms |

Acquisition may precede the writer's post-unlock marker slightly. The table uses the marker before unlock; raw signed offsets from the post-unlock marker and both sides of the release bracket are retained. Every sample remains in the evidence, including individual losses and scheduler variation. The matched total median gate does not pass in this batch. Actual writer sleep also differs: 9.47 has a 764.7 ms median hold versus 755.9 ms for 9.45. The 9.46 second round includes a 208.6 ms Manager-stop phase. These observations limit causal conclusions about total time; the raw total gate is kept open rather than replaced by a normalized score. These are controlled model-contention diagnostics, not installed IDM transfer benchmarks or proof of universal speed parity.

[All nine observations](validation/managed-exit-9.47/matched-comparison/runs.json), [release boundaries and totals](validation/managed-exit-9.47/matched-comparison/boundaries.json), [explicit gates](validation/managed-exit-9.47/performance-gates.json).

## Delivery and remaining work

Source and development binaries are 9.47 / extension 0.73.11. Installed 8.68 / extension files 0.72.32, both personal catalogs, shared media helpers and previous qualified products are preserved. Owned fixture/compiler artifacts are removed after exporting sanitized evidence. No setup package, installation, commit, push or release is created by this increment.

Hardware/browser comparisons and remaining shutdown, presentation and compatibility gaps stay open. A controlled median improvement does not establish full inside/out IDM parity.
