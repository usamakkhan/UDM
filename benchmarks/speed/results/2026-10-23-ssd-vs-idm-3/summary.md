| case | client | done | wall s | MB/s | conns | requests | rejected | drops | wasted MiB | sha ok | note |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|---|---|
| single-8 | idm | True | 8.905 | 45.22 | 14 | 14 | 0 | 0 | 1.7 | True |  |
| single-8 | udm | True | 7.552 | 53.32 | 9 | 29 | 0 | 0 | 1.4 | True | segments=28 splits=4 |
| uncapped-8 | idm | True | 2.936 | 137.14 | 8 | 8 | 0 | 0 | 2.5 | True |  |
| uncapped-8 | udm | True | 3.488 | 115.44 | 13 | 38 | 0 | 0 | 7.0 | True | segments=37 splits=13 |
| resume-8 | idm | True | 8.452 | 47.64 | 22 | 22 | 0 | 0 | 204.5 | True |  |
| resume-8 | udm | True | 5.149 | 78.2 | 18 | 37 | 0 | 0 | 43.7 | True | segments=27 splits=3 |
| outage-8 | idm | True | 13.651 | 29.5 | 22 | 22 | 2054 | 8 | 3.2 | True |  |
| outage-8 | udm | True | 13.394 | 30.06 | 19 | 40 | 65 | 8 | 5.0 | True | segments=31 splits=7 |
| drop-8 | idm | True | 17.9 | 22.49 | 66 | 66 | 0 | 46 | 24.0 | True |  |
| drop-8 | udm | True | 9.673 | 41.63 | 57 | 79 | 0 | 47 | 21.5 | True | segments=31 splits=7 |
| no-range-1 | idm | True | 2.032 | 132.1 | 4 | 4 | 0 | 0 | 0.5 | True |  |
| no-range-1 | udm | True | 3.642 | 73.71 | 2 | 2 | 0 | 0 | 0.2 | True | segments=1 splits=0 |
| slow-start-2 | idm | True | 20.665 | 12.99 | 8 | 8 | 46 | 0 | 0.9 | True |  |
| slow-start-2 | udm | True | 19.329 | 13.89 | 4 | 28 | 48 | 0 | 0.9 | True | segments=27 splits=2 |
