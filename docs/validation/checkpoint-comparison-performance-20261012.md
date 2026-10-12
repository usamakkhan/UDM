# Checkpoint comparison performance, 2026-10-12

The native startup performance fixture loaded 5,000 synthetic completed
downloads and measured six consecutive `Manager::tick()` calls. Ticks three
and six include the periodic checkpoint check. All times are milliseconds.

| Implementation | Startup samples | Tick samples |
| --- | --- | --- |
| Before direct job comparison | 98, 98, 109 | 20, 11, 123, 11, 8, 27 |
| Direct comparison, run 1 | 69, 83, 83 | 7, 6, 110, 8, 7, 17 |
| Direct comparison, run 2 | 70, 83, 86 | 7, 7, 128, 7, 8, 16 |

The unchanged checkpoint tick (sixth sample) fell from 27 ms to 16–17 ms in
these runs. The first checkpoint tick and startup measurements vary; this does
not establish a startup speedup. The fixture is isolated and synthetic, not a
comparison against IDM or the installed UDM build.

The candidate rebuilt successfully. `IdleCheckpointTests` passed 7 checks,
including persistence of nested video and audio changes. The real-window
`MenuStateTests` passed 370 checks.
