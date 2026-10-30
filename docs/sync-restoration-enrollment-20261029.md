# Synchronization restoration enrollment and retries

The missing-file restoration path is now connected to queue enrollment and retry admission. A completed, eligible direct download whose destination is absent can be enrolled in a synchronization queue. Existing non-file destinations and unreadable filesystem statuses remain rejected. Batch enrollment continues to validate the whole selection before saving.

Synchronization metadata failures can now retry when the destination remains missing. The existing retry delay, retry-count limit, server backoff, authentication exclusions and queue-cycle checks are unchanged. Previously a regular-file-only condition prevented that retry before recovery could start. After restart creates a transfer generation, ordinary transfer retry rules continue to apply.

A shared destination predicate is used by membership validation and metadata-retry admission. It reads filesystem status with an error_code rather than throwing from the scheduler on a transient status error. This does not bypass the restart or publication checks that guard an occupied destination. Unsupported request/media types, recycled entries and prior replacement versions retain their existing enrollment restrictions.

The repository advanced independently to c0b4d91 and version 1.2.1 before this batch; those changes are preserved. Production compilation and installer packaging are in progress. Runtime, queue and recovery tests remain deferred at the user's request. Extension 0.63.82 is unchanged. Nothing is installed or published by this batch.
