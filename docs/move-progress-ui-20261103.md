# Background Move/Rename — 1.11.0

The existing Move/Rename command and completed-file Properties now open a worker-backed progress dialog. It displays phases and known byte counts, and enables Cancel only while cancellation is supported. Enter cannot dismiss a running worker. Dialog teardown joins the worker, and failures are reported after it finishes.

Relocation releases the manager lock during source verification, copying, destination verification and rollback. An active cancellation token protects the selected download; a reserved destination prevents new downloads, catalog imports, property edits, automatic extension corrections and replacement-history paths from claiming it. The reservation also blocks descendants that would need the file path as a directory. Shutdown requests cancellation and waits for relocation to leave its reserved state. Only one relocation runs at a time.

History commit and recovery update location fields without replacing unrelated metadata changes made while the lock was released. New journals mark this path-only recovery behavior; older journal semantics remain supported.

All five native production targets built successfully at 1.11.0; source hashes remained unchanged. Runtime tests, visual inspection, installer creation, installation and publication remain deferred. This is source/build evidence, not measured responsiveness or proven IDM parity. Preflight recovery and final catalog writes still take the manager lock; Windows filesystem calls may block. Junction/alias conflicts, concurrent admission, shutdown, cancellation and interrupted-save cases require runtime acceptance. Installed UDM remains 1.6.0 / browser 0.63.82. Extension source remains 0.63.84.
