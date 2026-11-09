# HTTP initial range balance

The 5.86 request traces showed uneven first waves. With six workers, repeatedly halving the largest available range left some responses with roughly twice the payload assigned to others. Version 5.87 limits a newly assigned helper to one worker's share while the fresh download fills its initial worker set. Smaller donors still split in half. Late and slow-connection help keep their existing split rules, and the 5.86 routine tail budget remains in effect.

All 81 native checks passed, including exact SHA-256 for the fresh slow-connection fixture that used nine requests. All 16 files in the paired 5.86 and 5.87 loopback trials matched their expected SHA-256.

| File | Median initial range size spread, 5.86 / 5.87 | Requests per trial, 5.86 / 5.87 | Median wall time, 5.86 / 5.87 |
|---|---:|---:|---:|
| 24 MiB | 3.875 / 1.250 MiB | 7 / 6 | 2.434 / 2.234 s |
| 192 MiB | 3.000 / 4.750 MiB | 8 / 8 | 5.054 / 4.910 s |

Range spread is the difference between the largest and smallest initial assignments. The benchmark reconstructs those assignments from the saved range start offsets, excluding later splits. All five 24 MiB pairs reached six peak origin connections on 5.87, down from seven on 5.86. The 192 MiB range spread widened in this sample even though the median wall time fell. Direct paired timing against IDM remains open.

The fixture and origin logs are in the Windows local temporary folder under `udm-http-initial-range-balance-5.87`.

[Production build receipt](http-initial-range-balance-5.87-build.json)
