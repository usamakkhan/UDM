# Synchronization checks the saved file length

Synchronization previously compared the remote size and validator only with catalog metadata. A local file shortened or extended after completion could therefore be reported as up to date when the server had not changed.

The unchanged decision now also requires the destination length to equal its saved local Size. A mismatch enters the existing replacement workflow: download a new version, preserve the current file as a previous version, and publish through the existing recovery journal. Missing files retain their existing explicit error. Converted offline pages still compare remote size against GrabberSourceBytes and local size against the converted file Size, so differing source and converted lengths do not force a replacement.

This is a length check, not a complete integrity scan: same-length local edits are not detected. Existing replacement hashing and publication guards remain responsible for protecting a file modified during replacement. No claim of full IDM parity or measured performance is made.

The existing queue fixture now exercises mode switching on a custom synchronization queue because built-in queue roles are fixed. Its scenarios have not been executed.

All five production targets compiled and installer packaging passed. See [build evidence](validation/sync-local-size-build-20261029.json); the bundle manifest records all 147 payload hashes. Runtime and regression testing are deferred at the user's request. Extension 0.63.82 is unchanged. No installation or publication is requested in this batch.
