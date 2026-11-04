# Download-list refresh snapshots

Development version 1.20.0; browser source remains 0.63.85.

The main list previously held Manager::mutex across stable sorting, encrypted-header decoding, shell file-type icon lookup and list control updates. A slow shell extension or large history sort could therefore delay transfer bookkeeping and browser admission waiting for the same lock.

The list now captures the scalar display inputs, transfer speed, queue icon state, quota-wait membership, settings and storage warning under the manager lock. It releases that lock before search text expansion, sort-key derivation, sorting, icon lookup and painting. Selection/focus IDs are captured under the lock, preserving identity even when catalog operations replace job objects and scroll anchoring retains its existing behavior. Path rendering uses the captured folder/filename, not mutable job data after lock release.

DownloadListSnapshot.hpp defines the list projection explicitly. It includes progress/ETA and exact prefetch-size inputs, dates, descriptions and optional encrypted headers needed by visible or sorted Referer/Web page columns or search. Transfer journals, adaptive plans and private child tracks are excluded. Unchanged rows reuse immutable cached projections; changes to their display inputs, speed or queue icon create a new projection. Each regular refresh replaces the retained cache with the current category/queue/project candidates, dropping older entries. Row invalidation compares projection identity plus quota/settings state instead of comparing/copying entire job records.

The change preserves current sorting rules, stable ties, category/queue/project filters, search semantics, progress calculations, quota indicators, icon fallback and selection/focus restoration. Sound notifications are dispatched after the snapshot lock is released. Existing tree refresh, scheduler tick and command-state paths retain their own locking; this change does not establish that every UI lock or startup bottleneck is resolved.

No timing improvement is claimed without measurement. Large-catalog, active-transfer, browser-handoff and UI interaction comparisons remain deferred by instruction.

Build status: passed for all five production targets. Full and final incremental builds completed; embedded versions and SHA-256 hashes are recorded. Feature source hashes matched the final-build snapshot. Installed version 1.6.0 was rechecked directly. Runtime tests, installer creation, installation and publication remain deferred. Installed UDM remains 1.6.0. See [build receipt](validation/download-list-snapshot-refresh-build-20261104.json).
