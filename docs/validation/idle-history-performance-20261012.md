# Idle history performance, 2026-10-12

An isolated native harness loaded 5,000 synthetic completed downloads from a
saved catalog and measured `Manager` construction and six consecutive ticks in
milliseconds. The third and sixth ticks include the periodic checkpoint check.
The harness is registered as `StartupPerformanceTests` in
`native/backend-tests.json`.

| Variant | Startup samples (ms) | Tick samples (ms) |
| --- | --- | --- |
| Before backend changes | 97, 97, 99 | 39, 38, 218, 39, 37, 96 |
| Final backend changes | 77, 84, 171 | 9, 9, 131, 10, 8, 31 |
| Final repeat | 98, 98, 109 | 20, 11, 123, 11, 8, 27 |

The idle queue shortcut reduces ordinary tick time for this completed-history
fixture. Checkpoint ticks remain more expensive because they compare saved job
state. Startup measurements are too variable to establish a speedup. These are
isolated synthetic measurements, not a comparison against IDM or an installed
UDM build.

Validation of the final candidate: `QueueCompletionCycleTests` passed 15
checks, `QueueRetryPolicyTests` passed 16 checks, `IdleCheckpointTests` passed
5 checks, and the real-window `MenuStateTests` passed 370 checks. The queue
completion fixture required loopback networking, so it was run outside the
network-restricted sandbox.
