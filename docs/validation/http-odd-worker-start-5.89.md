# First HTTP split for odd worker counts

The three-worker case in 5.88 split one half of the file and left the other half intact. In 5.89, the first helper receives the share corresponding to the smaller half of the planned worker set. For three workers, the original response retains two-thirds and the first helper receives one-third. For five, the split is three-fifths and two-fifths. Even worker counts still divide the first response at its midpoint. The helper validates its response headers before taking ownership, and the existing overlap check protects publication. [5.88 comparison](http-worker-split-policy-5.88.md)

All 83 native checks passed, including the first helper's exact request offset and the final SHA-256 for the three-worker case. The loopback benchmark alternated 5.88 and 5.89 three times per size. All 24 files matched the expected SHA-256.

| File and initial workers | Median initial range spread, 5.88 / 5.89 | Requests per trial, 5.88 / 5.89 | Median wall time, 5.88 / 5.89 |
|---|---:|---|---:|
| 12 MiB, three | 3.000 / 0.750 MiB | 4, 4, 4 / 3, 3, 3 | 2.034 / 2.402 s |
| 20 MiB, five | 3.375 / 1.750 MiB | 6, 6, 6 / 5, 6, 5 | 2.435 / 2.043 s |
| 24 MiB, six | 1.250 / 1.250 MiB | 6, 6, 8 / 6, 7, 6 | 1.835 / 1.891 s |
| 192 MiB, eight | 3.000 / 5.250 MiB | 8, 8, 8 / 8, 8, 8 | 4.254 / 4.458 s |

Initial range spread is the difference between the largest and smallest initial assignments reconstructed from the saved starts. Both odd-worker cases had narrower initial ranges and fewer requests in most trials. The 12 MiB median wall time rose, and the 20 MiB median fell. The six- and eight-worker control cases use identical split rules in both builds and show run-to-run variability. Direct paired timing against IDM remains open.

The fixture and origin logs are in the Windows local temporary folder under `udm-http-odd-worker-start-5.89`.

[Production build receipt](http-odd-worker-start-5.89-build.json)
