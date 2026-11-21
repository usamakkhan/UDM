# Responsive immediate Save To settings — UDM 9.18

UDM 9.18 / source extension manifests 0.72.82 moves category Browse, remember-path, server-date, temporary Browse and Use default writes off the Options window thread. These actions retain their existing immediate-save semantics; closing Options discards other unapplied edits, not a successfully acknowledged immediate change.

ImmediateOptionsDraft owns an action kind, category key, folder and boolean choice. The existing FileInfoDecisionTask serializes accepted work under the global decision guard. Its worker reads current settings under Manager, changes only the intended category entry or field, runs the existing settings validation/transaction and returns a limited acknowledgement. Category actions reject a category that was removed while waiting; relative category/temporary destinations and unknown commands are rejected before mutation. Temporary Use default accepts an empty override.

Options freezes accepted controls and keeps Close/duplicate OK from abandoning the pending write, while pulse and paint remain responsive. Errors retain ownership through acknowledgement, restore the actual control from the last accepted preferences and permit explicit retry. Success updates only CategoryPaths/CategoryRememberLast, UseServerDate or TemporaryFolder in both its draft and original baseline. Only the affected controls refresh; other unfinished preferences and even uncollected text in other editors remain intact. No startup/credential apply or browser-host restart is included in an immediate change.

## Qualification

Five native products built and all 308 targeted backend assertions passed across C:/D:. Pending File Info passed 257. Presentation failed after 79 assertions at its single-refresh expectation, with both fresh receipts still saved. Main reached all seven new Save To scenes but its warning driver sent hard-coded IDOK instead of clicking the actual OK button (observed control ID 2). It ended with 622 assertions and two failures after the recorded test-owned warning was externally acknowledged. That intervened run is not qualification. The current product is uninstalled and this candidate is not qualified as a whole.

## Diagnostic boundary

An early owned backend run passes 24 assertions. An /Od UI diagnostic using the current freshly compiled cores stops before the new Save To scenarios at the older all-static-label File Info check (181 assertions, including that failure and its cleanup failure). Preserve this separately from optimized qualification; its final error is the cleanup assertion. It does not qualify the immediate UI workflow. The existing File Info label fixture and alternate optimization modes remain a separate concern.

[Main failure](validation/immediate-options-9.20/superseded-9.18-modal.json), [presentation failure](validation/immediate-options-9.20/superseded-9.18-presentation.json) and [warning observation/intervention](validation/immediate-options-9.20/superseded-9.18-warning-observation.json) retain the failed candidate separately.

## Remaining work

The Windows shell folder-picker interaction itself is unchanged; owned UI tests inject its return value and exercise the actual Browse button and subsequent save. Other Options subdialogs, File Info's server-name action, scheduling/admission, other advisory reads and exceptional teardown retain waits or need separate audit. Full inside/out IDM parity remains unproven.

Installed UDM remains 8.68 / extension files 0.72.32. Browser runtime JavaScript is unchanged. No installer, installation, personal-desktop control, commit, push or release is included.
