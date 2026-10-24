| case | client | done | wall s | MB/s | conns | requests | rejected | drops | wasted MiB | sha ok | note |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|---|---|
| single-8 | idm | True | 16.47 | 24.45 | 18 | 18 | 0 | 0 | 5.9 | True |  |
| single-8 | udm | True | 7.464 | 53.95 | 10 | 53 | 0 | 0 | 1.4 | True | segments=52 splits=4 |
| uncapped-8 | idm | True | 4.681 | 86.02 | 15 | 15 | 0 | 0 | 4.3 | True |  |
| uncapped-8 | udm | True | 14.966 | 26.9 | 11 | 58 | 0 | 0 | 6.0 | True | segments=57 splits=9 |
| drop-8 | idm | True | 14.377 | 28.01 | 57 | 57 | 0 | 44 | 17.3 | True |  |
| drop-8 | udm | True | 14.508 | 27.75 | 54 | 101 | 0 | 46 | 8.1 | True | segments=54 splits=6 |
| no-range-1 | idm | True | 1.837 | 146.13 | 4 | 4 | 0 | 0 | 0.5 | True |  |
| no-range-1 | udm | True | 3.633 | 73.89 | 2 | 2 | 0 | 0 | 0.2 | True | segments=1 splits=0 |
