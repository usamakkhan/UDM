# Native DASH capture windows and persistent index

The native DASH module now connects planned windows to resource storage through a single-writer capture function. It creates an encrypted recording index bound to the complete selected-track identities. Before new network requests, it records pending media identities/URLs and initialization URLs, allowing interrupted work to remain identifiable independently of in-memory planner state.

The index validates existing segment bindings and accepts refreshed URLs only for the same sequence/time/duration. Segments must form a contiguous sequence and media timeline within each selected track; gaps, overlaps and replacements fail explicitly. The first captured segment may start at an arbitrary live position. A recording is limited to 10000 segments across tracks and an 8 MiB plaintext index. It does not yet span Period transitions or represent intentional discontinuities.

Initialization resources are downloaded through the verified receipt store and must retain identical bytes within the selected Period. Refreshed signed URLs may produce another resource key, but changed initialization bytes fail rather than silently mixing decoder configurations. Media downloads use the presentation/track/segment binding. Completed resources add file names, sizes and hashes to the index; index checkpoints are per track/batch, while individual verified receipts remain available after interruption. A crash before the batch checkpoint can therefore leave valid resource receipts beside pending index entries. Full recovery must reconcile those receipts before assembly; a pending entry is not a completed segment.

The capture function uses the existing quota/rate byte callback and cancellation path. It must be called serially for one job-owned folder. It preserves failed and partial work for later recovery. It does not delete earlier candidates or recordings.

## Remaining integration

Live planning, resource storage and persistent capture metadata are now connected through callable native functions. The running job loop still needs to drive those functions, reconcile pending records, choose/persist capture windows, handle Period transitions and user stop requests, and assemble/publish complete output. Browser live admission remains disabled. A static-manifest flag is recorded but is not treated as proof of download completion.

Runtime and regression tests remain deferred at the user's request. No network transfer was run during this implementation. Production compilation and packaging are recorded in `validation/native-dash-capture-build-20261028.json`. Browser 0.63.61 is unchanged; the candidate is unsigned, uninstalled and unpublished. Complete IDM parity remains unproven.
