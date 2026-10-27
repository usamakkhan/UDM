# Adaptive interruption recovery

Recorded HLS/DASH transfers now report Paused after cancellation and Failed after
a transfer/assembly failure, clearing any obsolete prefetched exact-size claim.
On catalog reload, interrupted adaptive downloads recover with a Paused phase;
existing Paused/Failed entries receive matching phases and lose obsolete preview
size metadata. Completed entries retain completion and receive the Complete
phase, correcting old records that still said Waiting for confirmation.

Recovery does not delete retained segments or change download destinations.
It does not claim an unconfirmed assembled output survived a restart.

## Validation

The native `--adaptive-prefetch-checks` suite passed **229 checks, 0 failures**.
New coverage exercises cancellation, a real media-quality validation failure,
catalog reload from Downloading/Merging/Paused/Failed/Complete states, removal of
stale confirmation/size claims, and preservation of retained bytes. Existing
exact-size, folder-change, early confirmation, timing and media tests also pass.
Test and desktop builds succeeded; `git diff --check` passed.

Candidate SHA-256:
`E06692D4E331611D4C7C10097BC0E35B63DDAA1922F10504BDD91938C6B45FE2`.
Installation succeeded and the installed executable's SHA-256 matches the tested
candidate. The earlier browser quality matrix applies to its recorded build,
not a new live test of this recovery-only update.

[Installation and validation record](validation/adaptive-interruption-recovery-20261026.json).
