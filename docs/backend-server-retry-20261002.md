# Server-requested queue delays — 2 October 2026

The transfer engine already interpreted `Retry-After`, but its rejected response
lost that timing information when it reached the queue's file-retry policy. A
one-second queue delay could therefore override a three-second server delay.
Starting a new queue cycle after reopening the catalog could retry immediately.
Synchronization checks had the same problem. An unlimited queue also bypassed the
existing refusal to wait automatically when a server asked for more than five minutes.

The local HTTP baseline reproduced **30 failed assertions out of 64**. Requests
following a three-second response were retried after about 1.04 seconds, and after
74–81 ms when the catalog was reopened and the queue restarted. The test uses real
elapsed time; it does not advance the clock.

## Changes

- Persist the server's accepted retry deadline with the failed job.
- Schedule automatic retries after both the queue delay and server deadline.
- Retain the server deadline across catalog reload and new queue cycles.
- Preserve the existing five-minute automatic-wait limit: longer server waits
  require an explicit retry and cannot trigger a successful completion action.
- Keep cancellation effective after the deadline and clear stale retry state on
  a fresh attempt or a user-refreshed download address.

The policy applies to ordinary queued transfers, synchronization probes, and
synchronization replacement failures. Explicit individual Resume remains a user
retry. Explicit queue Start permits retrying a wait that required review, while
short, still-pending server deadlines remain in effect. Automatic scheduled startup
does not acknowledge a wait requiring review.

## Validation

`native/ServerRetryTests.cpp` exercises ordinary downloads and synchronization with
real local HTTP failures, retry timing, restart persistence, long-wait review, Stop,
malformed headers, saved-file integrity, and completion events. It is registered in
the normal backend test runner. All **64 focused checks pass** after the correction.
The three-second cases retried no earlier than 3.03 seconds, including after restart.
The long-wait cases resumed only after the test explicitly started the queue again.
The complete regression and desktop build results are recorded after validation.

Retained evidence: [failing baseline](evidence-server-retry-20261002/baseline-results.json),
[focused correction](evidence-server-retry-20261002/focused-results.json), and
[changed source hashes](evidence-server-retry-20261002/source-hashes.json).

```powershell
.\native\test-backend.ps1 -FocusedOnly -Case ServerRetryTests
```

This is a queue reliability correction, not a claim of complete IDM parity or
qualification of clock changes, physical sleep, or public servers.
