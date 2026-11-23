# Current finishing pilot: 9.71

Three normal-product UDM downloads completed on D: with exact size/SHA-256, saved Complete records, process-exit observation and a symmetric post-exit output flush. Every payload was removed after verification: 138,412,032 logical bytes (132 MiB). Installed files, personal catalogs and shared helpers match the eleven protected hashes.

This is one sample per case, with no baseline or IDM run. It cannot establish a regression, improvement, equivalence or parity. Current-session IDM was already running, so the full comparison's clean-start requirement was not satisfied. No process was terminated. The Windows-control retry failed with `failed to write kernel assets: The system cannot find the path specified. (os error 3)`.

| Case | Exclusive file availability after last send | All observed app processes exited after last send | Full elapsed through exit | Post-exit flush completed after last send |
| --- | ---: | ---: | ---: | ---: |
| 4 MiB sequential | 110.752 ms | 159.227 ms | 1,258.611 ms | 184.435 ms |
| 64 MiB sequential | 216.110 ms | 265.295 ms | 9,140.833 ms | 289.528 ms |
| 64 MiB split | 214.798 ms | 265.084 ms | 2,375.455 ms | 277.094 ms |

These endpoints have different meanings. Exclusive opening waits for UDM's publication pins to release after durable catalog acceptance. It is stronger than simply observing the filename or opening with shared access. Process exit includes the observed app's remaining work and `/q` shutdown. The extra flush measures an explicit output-file barrier after exit, not every system-service action or directory metadata durability. No GUI timing was measured.

## What blocks the current paths

The sequential 64 MiB body flush took 171.393 ms. The publication intent write took 97.737 ms and its following body join still waited 73.780 ms. The flush operation remains a measured dependency; this does not distinguish storage queueing, filesystem filters or scheduling within that call.

The split run's optional hash took 82.981 ms, with 81.470 ms recorded in the late-hash wait. Source inspection shows the future is consumed in `transfer()` before `PreparedTransferCommit::prepare()` returns the work to the publication owner. Intent persistence therefore cannot overlap that wait. Its body flush ran concurrently with the hash, so it would be incorrect to add all these durations as serial work. The later intent write took 30.012 ms, followed by a negligible 0.026 ms body join.

The next targeted design should overlap advisory hashing with the durable intent transaction while keeping the hash inside the measured worker lifetime and before the final accepted catalog snapshot. The existing future captures the local transfer hasher by reference; moving that future into the publication owner as-is would leave a dangling reference. A correct implementation needs value-owned hashing state and body lifetime, exact source binding, cancellation/error joining, and a consistent recovery intent/catalog proof when an advisory digest arrives after intent construction. Required user/server/replacement hashes must remain prepublication gates. No wait should be moved outside the measurement or dropped to produce an apparent win.

The full comparison driver is prepared for normal 9.71, frozen 9.63 and IDM, with three rotating trials of each case. 9.63 is the last measured control, not a previously passing parity baseline. All samples, losing pairs, full elapsed, first availability, process exit and post-exit flush must be retained. No compiler should run concurrently. A clean current-session IDM start is pending.

## Reproduction and evidence

The private pilot is derived from the unchanged normal comparison harness. Its recorded transformations select only UDM, remove all IDM setting reads/writes and remove the IDM clean-start guard for this UDM-only run. Timing, payload verification, catalog verification, process observation and owned-file deletion retain their existing logic. Provenance includes the original/derived driver hashes and exact transformation list. This does not change production source or versions.

[All raw runs](validation/current-finishing-pilot-9.71/results.json), [analysis and cleanup](validation/current-finishing-pilot-9.71/analysis.json), [terminal record](validation/current-finishing-pilot-9.71/terminal.json), [pilot provenance](validation/current-finishing-pilot-9.71/driver/current-pilot-9.71-request.json), [prepared paired driver](validation/current-finishing-pilot-9.71/driver/run-relocation-comparison-9.71.py), [export hashes](validation/current-finishing-pilot-9.71/export-manifest.json).
