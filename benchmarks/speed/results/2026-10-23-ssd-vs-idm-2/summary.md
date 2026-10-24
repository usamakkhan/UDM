| case | client | done | wall s | MB/s | conns | requests | rejected | drops | wasted MiB | sha ok | note |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|---|---|
| single-8 | idm | True | 17.649 | 22.81 | 14 | 14 | 0 | 0 | 4.5 | True |  |
| single-8 | udm | True | 7.547 | 53.35 | 10 | 52 | 0 | 0 | 1.2 | True | segments=51 splits=3 |
| uncapped-8 | idm | True | 3.237 | 124.39 | 5 | 5 | 0 | 0 | 2.2 | True |  |
| uncapped-8 | udm | True | 4.95 | 81.34 | 13 | 59 | 0 | 0 | 7.7 | True | segments=58 splits=10 |
| resume-8 | idm | True | 8.857 | 45.46 | 22 | 22 | 0 | 0 | 194.7 | True |  |
| resume-8 | udm | True | 5.5 | 73.21 | 17 | 63 | 0 | 0 | 53.4 | True | segments=52 splits=4 |
| outage-8 | idm | True | 13.653 | 29.49 | 22 | 22 | 2149 | 8 | 3.2 | True |  |
| outage-8 | udm | True | 13.691 | 29.41 | 19 | 61 | 65 | 8 | 2.1 | True | segments=52 splits=4 |
| drop-8 | idm | True | 8.853 | 45.48 | 59 | 59 | 0 | 44 | 6.8 | True |  |
| drop-8 | udm | True | 8.256 | 48.77 | 54 | 102 | 0 | 48 | 4.2 | True | segments=53 splits=5 |
| no-range-1 | idm | True | 1.825 | 147.09 | 4 | 4 | 0 | 0 | 0.5 | True |  |
| no-range-1 | udm | True | 3.432 | 78.22 | 2 | 2 | 0 | 0 | 0.2 | True | segments=1 splits=0 |
| slow-start-2 | idm | True | 20.864 | 12.87 | 11 | 11 | 50 | 0 | 1.9 | True |  |
| slow-start-2 | udm | True | 19.436 | 13.81 | 4 | 52 | 53 | 0 | 1.5 | True | segments=51 splits=2 |
