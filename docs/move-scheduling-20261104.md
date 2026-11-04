# Relocation and transfer scheduling — 1.11.2

A background file move keeps an active token to protect its catalog entry and support shutdown cancellation. Queue admission previously counted that token as a network transfer, and automatic queue-window stops could cancel it. The scheduler now excludes relocation from global/per-queue/project transfer accounting and from automatic schedule-stop cancellation. Explicit queue Stop leaves the move running while still clearing pending queue work. File Info prefetch admission uses the same transfer count.

The active token still protects the file against edits/removal. Application shutdown and explicit per-file cancellation retain their existing behavior. Completion readiness continues to wait for all active operations, including relocation, so power/exit actions cannot run while a move is unfinished.

All five native production targets built successfully at 1.11.2. Final-build source hashes remained unchanged. Runtime tests, installers, installation and publication are deferred. Installed UDM remains 1.6.0; browser source remains 0.63.84. This is an integration correction, not proof of full IDM parity.
Synchronization enrollment can remain pending while a completed file moves; it starts only after relocation releases the download.
