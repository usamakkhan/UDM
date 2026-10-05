# Synchronization probe validation — 2 October 2026

Synchronization must finish validating a range probe before declaring a saved file
current or creating its replacement. Previously, matching size and ETag headers
could produce an “Up to date” result even when the requested byte never arrived.
Malformed ranges could also reserve a replacement before the regular downloader
rejected the same response. A valid `416` response describing an empty resource was
always treated as a failed check.

`native/Queue.cpp` now validates the exact requested range, positive total size,
identity encoding, and one-byte response body. It accepts both length-delimited and
chunked probes, recognizes `Content-Range: bytes */0`, and preserves the existing
fallback for servers returning a full `200` response. Bad probes become failed
synchronization checks and use the existing queue retry policy. They do not create
a replacement record or alter the saved file.

## Regression coverage

`native/SyncProbeTests.cpp` uses a local HTTP server and real downloads. It covers
truncated bodies, wrong/missing ranges, unexpected encoding, oversized and empty
chunked bodies, empty resources, nonempty unsatisfied ranges, overflowing and
impossible sizes, servers ignoring Range, explicit identity encoding, valid chunked
responses, and cancellation while waiting for the response body. Each normal case
checks the synchronization outcome, retry state, request/record count, saved bytes,
completion-event failure count, and restart persistence. Completion events are
inspected without executing a system action.

The unchanged engine failed **21 of the initial 66 assertions**. The first fixed
run passed all 66. The expanded regression passes **83 checks**. A fresh complete
backend run passes **2,874 checks**: 2,545 native checks and 329 focused checks,
including the new regression. Existing queue retry, synchronization replacement,
completion, backup, and restore cases all pass. Builds succeeded with existing
compiler warnings.

Retained evidence: [failing baseline](evidence-sync-probe-20261002/baseline-results.json),
[expanded regression](evidence-sync-probe-20261002/SyncProbeTests-results.json),
[complete run](evidence-sync-probe-20261002/backend-summary.json),
[native log](evidence-sync-probe-20261002/native-suite.log), and
[tested source hashes](evidence-sync-probe-20261002/source-hashes.json).

Run the focused regression from the repository root in PowerShell 7:

```powershell
.\native\test-backend.ps1 -FocusedOnly -Case SyncProbeTests
```

Omit `-FocusedOnly -Case SyncProbeTests` to run the complete backend suite. Outputs
and test catalogs use the configured temporary output directory. This change does
not establish exact IDM behavior, public-site coverage, or full backend parity.
