# Returning a refused range to its existing response

In both 5.55 capped runs, the origin refused the first extra connection. Its assigned range stayed in the catalog. The accepted response stopped at the new boundary, and the remaining bytes required further requests. The first replacement for that tail arrived more than a second after the shortened response closed. [5.55 timings](range-reuse-5.55.md)

UDM 5.56 can restore the range boundary when a connection fails before writing any bytes, the split is the last slot, and the original worker still has an accepted response covering the full tail. The worker clears that eligibility before leaving a shortened response and whenever it discards bytes past the boundary. The manager persists the restored map before either worker can continue. If the split cannot be restored, its range returns to the existing reassignment path.

The frozen 5.55 control passed 69 checks, including the output digest and refusal bound. It failed the new check for keeping both original responses. The 32 MiB two-slot origin logged four accepted HTTP requests.

The optimized 5.56 production objects passed all 70 native transfer checks. The same capped fixture used two accepted HTTP requests and two refused opens, with the expected SHA-256. The forced-reset and pause/restart cases passed, and the first accepted connection after the 1.5-second blackout arrived 32 ms after it ended. [Build receipt](range-tail-5.56-build.json)

## Paired application runs

Two alternating runs per build used the same loopback matrix and eight requested connections. All 16 files matched their expected length and SHA-256. Both 5.56 capped runs used the original full-file response through its midpoint assignment and the original midpoint response through the end. Each capped run had one refused open and took 1.13–1.41 seconds less than its 5.55 counterpart.

| Case | 5.55 HTTP requests | 5.56 HTTP requests | 5.55 seconds | 5.56 seconds |
| --- | ---: | ---: | ---: | ---: |
| Healthy 24 MiB | 7 / 8 | 9 / 9 | 3.529 / 3.301 | 3.367 / 3.098 |
| Healthy 192 MiB | 8 / 8 | 9 / 8 | 7.579 / 7.693 | 8.243 / 7.876 |
| Two slots, 24 MiB | 5 / 5 | 2 / 2 | 5.917 / 5.750 | 4.506 / 4.621 |
| Forced resets | 15 / 14 | 14 / 18 | 3.083 / 3.246 | 3.051 / 4.006 |

The capped origin refused two opens in both 5.55 runs. The healthy 192 MiB runs both reached eight concurrent connections. The healthy 192 MiB runs and one reset run took longer on 5.56. This small loopback sample establishes the capped improvement and leaves broader timing and live IDM comparison open.
