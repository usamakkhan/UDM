# Reusing completed HTTP responses in UDM 5.55

The 5.54 cap trace showed two refused opens just before the server finished an accepted response. Its worker had finished the shortened assignment and was eligible for the reuse shortcut even though part of the HTTP body remained unread. [5.54 control and diagnostic](range-admission-5.54.md)

UDM 5.55 records whether the successful worker read the entire response. An idle worker with a shortened response joins the paced admission path. The immediate stale keep-alive retry also requires a previously drained response. Reconnects after a reset with bytes received continue through the existing progress-aware retry path.

All 69 native transfer checks passed against the optimized production objects. The strict two-slot fixture completed with two refused opens, compared with ten on 5.53 and six on 5.54. The download matched the expected SHA-256. The forced-reset case completed within its six-second bound, and the first connection after the 1.5-second blackout was accepted 31 ms after the blackout ended. The staged build contains all five native executables and both versioned browser bundles. [Build receipt](range-reuse-5.55-build.json)

## Paired application runs

Two trials per build used the same loopback origin and eight requested connections. Trial order alternated. The healthy 24 and 192 MiB origins served 5 and 10 MiB/s per connection with 20 and 25 ms response latency. The capped origin served 24 MiB at 4 MiB/s with 20 ms response latency and allowed two connections. The reset origin served 24 MiB at 5 MiB/s with 20 ms latency and dropped connections after every 3 MiB. All 16 files matched their expected length and SHA-256.

| Case | 5.53 HTTP requests | 5.55 HTTP requests | 5.53 refused opens | 5.55 refused opens | 5.53 seconds | 5.55 seconds |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Healthy 24 MiB | 10 / 11 | 9 / 10 | 0 / 0 | 0 / 0 | 2.627 / 3.349 | 3.305 / 2.549 |
| Healthy 192 MiB | 17 / 16 | 11 / 10 | 0 / 0 | 0 / 0 | 8.044 / 11.041 | 10.167 / 9.026 |
| Two slots | 6 / 6 | 5 / 5 | 6 / 6 | 2 / 2 | 5.057 / 5.199 | 5.820 / 5.948 |
| Forced resets | 17 / 18 | 14 / 13 | 0 / 0 | 0 / 0 | 3.072 / 2.942 | 2.953 / 3.049 |

Both builds reached eight concurrent connections in each healthy 192 MiB run. The capped case consistently used fewer requests and opens but took about three quarters of a second longer. In the healthy and reset cases, the two timing pairs point in different directions. These are controlled UDM-to-UDM measurements.
