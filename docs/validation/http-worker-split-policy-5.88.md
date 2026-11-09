# HTTP startup splits by worker count

Version 5.87 made the first six ranges more even and eliminated the routine seventh request in the 24 MiB loopback case. Its eight-worker case had wider initial ranges. Version 5.88 keeps midpoint splitting when the initial worker count is a power of two and keeps share-based splitting for other counts. Slow-connection help, the routine tail limit, and saved-range recovery follow their existing paths. [5.87 comparison](http-initial-range-balance-5.87.md)

All 81 native checks passed. The loopback benchmark alternated 5.87 and 5.88 three times per size. All 24 files matched the expected SHA-256.

| File and initial workers | Median initial range spread, 5.87 / 5.88 | Requests per trial, 5.87 / 5.88 | Median wall time, 5.87 / 5.88 |
|---|---:|---|---:|
| 12 MiB, three | 3.00 / 3.00 MiB | 4, 4, 4 / 4, 4, 4 | 2.235 / 2.031 s |
| 16 MiB, four | 0.75 / 1.00 MiB | 4, 4, 4 / 5, 4, 4 | 2.559 / 2.438 s |
| 24 MiB, six | 1.25 / 1.25 MiB | 6, 6, 7 / 6, 6, 7 | 2.475 / 2.040 s |
| 192 MiB, eight | 4.75 / 3.00 MiB | 8, 8, 8 / 8, 8, 8 | 4.104 / 4.066 s |

Initial range spread is the difference between the largest and smallest initial assignments reconstructed from the saved starts. The eight-worker spread narrowed. The four-worker spread widened slightly and one trial used an extra request. The three- and six-worker cases use the same split policy in both builds, so their elapsed differences show run-to-run variability. Direct paired timing against IDM remains open.

The fixture and origin logs are in the Windows local temporary folder under `udm-http-worker-split-policy-5.88`.

[Production build receipt](http-worker-split-policy-5.88-build.json)
