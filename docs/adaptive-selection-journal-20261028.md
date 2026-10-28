# Adaptive recovery selection binding

The adaptive journal now stores a SHA-256 selection digest in its snapshot and per-part receipts. A digest mismatch raises a recovery error without overwriting the mismatched snapshot or receipt. This complements, rather than replaces, the existing part-file hash and HTTP resource-validator checks.

Current recorded adaptive downloads pass the normalized `adaptiveSelection` identity, which excludes signed URLs, resource validators and display-only DASH duration. Selected tracks, ordering, ranges and other structural metadata remain bound. Existing flat-track downloads explicitly permit migration of older unbound snapshots or receipt-only journals. Migration loads compatible receipts first and writes a new snapshot generation before transfer begins. Old-generation receipts cannot override that snapshot. Existing part bytes are still checked by the ordinary transfer path before reuse.

New `dashPeriodJournal` construction derives a strict identity from the complete ordered Period plans and clocks. It does not permit legacy flat-track adoption. This factory is ready for the forthcoming Period transfer path; that path remains disabled, so this is not a claim that multi-Period recovery has been executed.

The default two-argument journal API retains legacy behavior for callers that do not supply an identity. A bound snapshot cannot be consumed through that unbound API. Corrupt JSON continues to use the existing empty-generation recovery behavior; an explicit selection mismatch is distinct and is never treated as ordinary corruption.

## Build and scope

The production build and installer results are in `validation/adaptive-selection-journal-build-20261028.json`. Browser 0.63.59 is unchanged. Runtime, crash and recovery tests are deferred at the user's request. The candidate is unsigned, uninstalled and unpublished. Multi-Period transfer, assembly, joining and handoff remain unfinished.
