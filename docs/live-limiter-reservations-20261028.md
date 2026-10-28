# Live speed-setting changes during download pacing

Source inspection found that `Manager::charge` read the global and per-file limits once, then `Rate::wait` reserved the entire received block at those values. Its cancellation polling did not re-read speed settings. A 64 KiB reservation at 1 KiB/s represents 64 seconds of pacing for one worker even if the user subsequently disables the limit. The transfer reader retains a 64 KiB minimum; SABR also charges 64 KiB portions. These are source-derived durations, not new runtime measurements.

`Manager::charge` now divides pacing into portions worth at most 50 ms at each currently active limit, re-reading settings between portions. The last portion is shortened to the remaining byte count. Unlimited transfers still take a single pass. The same shared global and per-download Rate objects serialize workers, preserving aggregate versus per-file behavior and tighter file-specific limits. Quota accounting remains after pacing and charges the original byte count once.

This removes large stale reservations after raising, disabling or switching a limit. It does not promise a 50 ms end-to-end response: concurrent workers can already have short reservations ahead of this worker, both global and file limits may apply, and OS scheduling adds latency. The separate captured-original-response credit scheduler is unchanged. The displayed progress still advances when the existing transport commits its received block.

The previous GUI audit's statement that UDM only supports a total limit is stale: the existing `GlobalLimitMode` already supports both Total across downloads and Each download. This change preserves the saved choice rather than silently changing users' bandwidth policy.

## Delivery and qualification

The production build and installer record is `validation/live-limiter-build-20261028.json`. Runtime, timing and regression tests remain deferred at the user's request. This is an implementation improvement, not evidence of equal limiter behavior or timing with IDM. The candidate includes the preceding proxy Negotiate change; the browser package remains unchanged at 0.63.55.

The subsequent [combined limiter correction](combined-limiter-reservations-20261028.md) makes global and per-file reservations concurrently and waits for the later deadline, removing the earlier serial-delay penalty.
