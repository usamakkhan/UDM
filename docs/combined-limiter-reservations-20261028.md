# Combining total and per-file bandwidth limits

The regular transfer path previously waited for the total limiter, then separately waited for the per-file limiter. When both caps were active, this added their delays for the same bytes. For an isolated worker with two equal limits, the scheduling formula could impose approximately twice the intended delay. This is a source-derived finding, not a fresh measured throughput result.

The implementation now reserves both shared time slots before waiting and uses their later deadline. Total allowances remain serialized across downloads and local allowances remain serialized across a download's workers. A disabled limiter clears its obsolete debt. The short portions and live preference refresh from the previous change remain in place. Cancellation is checked before reservations and during the wait; quota accounting remains after pacing and charges the original bytes once.

This brings the regular transfer path into line with the existing captured-original-response scheduler, which already combines global and local readiness with a maximum. It also agrees with `effectiveLimitKbps`, which reports the tighter active cap rather than a harmonic combination of both caps.

No profile settings, browser code or installed files are changed. `Rate::wait` remains available using the same reservation helper. Concurrent reservations can still add queueing latency, and scheduling granularity can affect observed throughput; this is not a claim of exact timing parity with IDM.

## Delivery and deferred acceptance

All production build and installer results are recorded in `validation/combined-limiter-build-20261028.json`. Runtime tests are deferred at the user's request. Pending acceptance includes equal and unequal combined caps, concurrent downloads and segments, changes while waiting, cancellation and shared pacing with captured responses. The staged candidate includes the preceding live-setting and proxy changes; browser version remains 0.63.55.
