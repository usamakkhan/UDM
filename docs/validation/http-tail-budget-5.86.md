# HTTP tail split budget

The retained IDM comparison observed seven requests and six peak connections in both 24 MiB range trials. Recent UDM traces reached six workers but continued splitting healthy responses as those workers became idle. Several extra requests covered less than 1 MiB. The earlier IDM runs used an existing profile and had unmatched settings, so the observed request count is a reference for this case. [Recorded IDM and UDM comparison](idm-transfer-audit-5.45.json)

UDM 5.86 preserves the initial range assignments and allows one ordinary tail split after that wave. Once this budget is used, an already accepted response must be stalled or demonstrably slower to be split again. The 750 ms stall threshold is a UDM policy choice. Recovery of unowned ranges and resumes use their existing paths.

All 81 native checks passed. The extra fresh-download fixture set four connections and served one range deliberately slowly. It observed nine data requests and the expected SHA-256, exercising help beyond the ordinary five-request budget.

The isolated loopback comparison alternated 5.85 and 5.86. All 16 downloads matched the expected SHA-256.

| File | Requests across trials, 5.85 / 5.86 | Median wall time, 5.85 / 5.86 |
|---|---|---|
| 24 MiB | 11, 10, 8, 9, 9 / 7, 7, 7, 7, 7 | 2.235 / 2.676 s |
| 192 MiB | 8, 8, 8 / 8, 8, 8 | 5.381 / 4.461 s |

The 24 MiB request count matched the recorded IDM count, while origin peak connections remained seven in these UDM runs. The smaller file took longer at the median. The larger file had the same request count and a shorter median in this sample. Direct paired timings with matching IDM settings remain open.

The native fixture runner, logs and origin request traces are in the Windows local temporary folder under `udm-http-tail-budget-5.86`.

[Production build receipt](http-tail-budget-5.86-build.json)
