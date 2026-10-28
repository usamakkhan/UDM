# Live DASH manifest revision checkpoints

Build status: all five production targets and the installer compiled successfully. The staged candidate is unsigned, uninstalled and unpublished. Compilation is not runtime recovery validation.

The live recording worker now checkpoints the latest accepted manifest in a separate DPAPI-protected `manifest-revision.json`. The original `bootstrap.json` remains the initial live-presentation identity. Changed XML, final URL or cache validators trigger a checkpoint before the corresponding media window is downloaded. Unchanged polling windows do not rewrite it.

Resume restores the original track selection, then validates and rematches the saved revision before contacting the server. The revision binds the full selected plan, track identities and known Period durations. Presentation identity, publication ordering and timeline origin must agree with the original bootstrap. Network refresh is compared against this latest revision, so a server rollback older than an already accepted publication is rejected even after restart. Original bootstrap-only recordings remain readable. An orphan revision without its bootstrap is rejected without rebinding retained media.

Known Period ends cannot change during rematching and remain available if a later manifest omits the duration. The revision also stores this learned boundary separately from the XML. This is needed when a sliding presentation removes earlier timing context. It does not implement automatic transition to another Period or join multiple recorded Periods.

Writes use the existing flushed temporary-file and atomic replacement path, with reparse checks for the directory, revision and temporary file. Plaintext serialization is limited to 8 MiB and the protected envelope to 16 MiB. XML continues to use the parser's existing limits. Failed checkpoint writes prevent the worker from adopting the new state and starting its media requests.

Persisted snapshots never restore monotonic-clock timestamps. Their receipt anchor is deliberately stale, and clock samples are cleared. Revalidation or disk reads cannot make a direct UTC timing sample fresh. Network clock synchronization remains subject to the existing freshness rules.

Browser 0.63.62 is unchanged. Runtime and regression tests are deferred at the user's request. Build and packaging results are recorded in `validation/native-dash-revision-checkpoint-build-20261028.json`; the candidate remains uninstalled and unpublished.
