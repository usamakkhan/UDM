# Responsive immediate Save To settings — UDM 9.19

UDM 9.19 / source extension manifests 0.72.83 moves category Browse, remember-path, server-date, temporary Browse and Use default writes off the Options window thread. These actions retain their existing immediate-save semantics; closing Options discards other unapplied edits, not a successfully acknowledged immediate change.

ImmediateOptionsDraft owns an action kind, category key, folder and boolean choice. The existing FileInfoDecisionTask serializes accepted work under the global decision guard. Its worker reads current settings under Manager, changes only the intended category entry or field, runs the existing settings validation/transaction and returns a limited acknowledgement. Category actions reject a category that was removed while waiting; relative category/temporary destinations and unknown commands are rejected before mutation. Temporary Use default accepts an empty override.

Options freezes accepted controls and keeps Close/duplicate OK from abandoning the pending write, while pulse and paint remain responsive. Errors retain ownership through acknowledgement, restore the actual control from the last accepted preferences and permit explicit retry. Success updates only CategoryPaths/CategoryRememberLast, UseServerDate or TemporaryFolder in both its draft and original baseline. Only the affected controls refresh; other unfinished preferences and even uncollected text in other editors remain intact. No startup/credential apply or browser-host restart is included in an immediate change.

## Qualification

Main/Options passes all 632 optimized assertions and pending File Info passes 257. Presentation reaches 315 assertions, then fails an older login expectation requiring IndividualStart after the payload has already completed. The saved job has Status=Complete, QueueOrigin=false and no IndividualStart flag; its 1,048,647-byte file matches the complete authentication fixture payload by SHA-256. Core.cpp clears the transient flag during finishing. Native entry compilation also records C1060 failures for App/Monitor while parallel UI compilation is active; the full native build is incomplete. Verified core objects and successful entries are recorded as a partial cache, not a qualified release.

## Superseded candidate and diagnostic boundary

An early owned backend run passes 24 assertions. An /Od UI diagnostic using the current freshly compiled cores stops before the new Save To scenarios at the older all-static-label File Info check (181 assertions, including that failure and its cleanup failure). Preserve this separately from optimized qualification; its final error is the cleanup assertion. It does not qualify the immediate UI workflow. The existing File Info label fixture and alternate optimization modes remain a separate concern.

9.18 native/backend and pending checks pass, but its optimized presentation driver assumes one nonblocking refresh can always present the next receipt. Two fresh receipts remain saved after its failure. The 9.19 fixture drives the existing bounded refresh-until contract and still requires exactly the expected dialog count and consumed saved receipts. The 9.18 Main driver reaches its immediate-save warning but sends the wrong command ID; actual child-button inspection records OK at ID 2 and the synthetic failure text. A test-only external click settles that failed run, which remains excluded. The 9.19 driver waits for the actual visible OK control and clicks it, without assuming its resource ID. No production warning behavior is changed by these fixture corrections.

[Main/Options checks](validation/immediate-options-9.20/superseded-9.19-modal.json), [presentation failure](validation/immediate-options-9.20/superseded-9.19-presentation.json), [exact terminal file](validation/immediate-options-9.20/superseded-9.19-terminal-file.json) and [incomplete native cache](validation/immediate-options-9.20/superseded-9.19-native-cache.json) retain its limits.

## Remaining work

The Windows shell folder-picker interaction itself is unchanged; owned UI tests inject its return value and exercise the actual Browse button and subsequent save. Other Options subdialogs, File Info's server-name action, scheduling/admission, other advisory reads and exceptional teardown retain waits or need separate audit. Full inside/out IDM parity remains unproven.

Installed UDM remains 8.68 / extension files 0.72.32. Browser runtime JavaScript is unchanged. No installer, installation, personal-desktop control, commit, push or release is included.
