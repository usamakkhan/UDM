| case | client | done | wall s | MB/s | conns | requests | rejected | drops | wasted MiB | sha ok | note |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|---|---|
| single-8 | idm | True | 9.698 | 41.52 | 14 | 14 | 0 | 0 | 1.4 | True |  |
| single-8 | udm | True | 13.797 | 29.18 | 17 | 70 | 0 | 0 | 9.7 | True | segments=69 splits=21 |
| uncapped-8 | idm | True | 5.024 | 80.15 | 12 | 12 | 0 | 0 | 3.3 | True |  |
| uncapped-8 | udm | True | 6.331 | 63.6 | 10 | 57 | 0 | 0 | 5.5 | True | segments=56 splits=8 |
| resume-8 | idm | True | 8.64 | 46.6 | 22 | 22 | 0 | 0 | 204.0 | True |  |
| resume-8 | udm | True | 9.078 | 44.35 | 17 | 69 | 0 | 0 | 53.1 | True | segments=51 splits=3 |
| outage-8 | idm | True | 20.562 | 19.58 | 25 | 25 | 2115 | 8 | 7.3 | True |  |
| outage-8 | udm | True | 19.339 | 20.82 | 19 | 63 | 57 | 8 | 2.4 | True | segments=54 splits=6 |
| drop-8 | idm | True | 8.834 | 45.58 | 59 | 59 | 0 | 44 | 7.2 | True |  |
| drop-8 | udm | True | 20.007 | 20.13 | 49 | 99 | 0 | 46 | 3.9 | True | segments=52 splits=4 |
| no-range-1 | idm | True | 1.815 | 147.9 | 4 | 4 | 0 | 0 | 0.5 | True |  |
| no-range-1 | udm | True | 4.442 | 60.43 | 2 | 2 | 0 | 0 | 0.2 | True | segments=1 splits=0 |
| slow-start-2 | idm | True | 21.068 | 12.74 | 8 | 8 | 33 | 0 | 1.2 | True |  |
| slow-start-2 | udm | True | 21.263 | 12.62 | 4 | 52 | 45 | 0 | 1.0 | True | segments=51 splits=2 |
