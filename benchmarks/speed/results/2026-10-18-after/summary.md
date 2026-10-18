| case | client | done | wall s | MB/s | conns | requests | rejected | drops | wasted MiB | sha ok | note |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|---|---|
| single-8 | idm | True | 10.18 | 39.55 | 15 | 15 | 0 | 0 | 1.5 | True |  |
| single-8 | udm | True | 11.038 | 36.48 | 8 | 10 | 0 | 0 | 0.5 | True | segments=9 splits=1 |
| single-16 | idm | True | 6.853 | 58.76 | 25 | 25 | 0 | 0 | 5.3 | True |  |
| single-16 | udm | True | 7.664 | 52.54 | 16 | 18 | 0 | 0 | 0.7 | True | segments=17 splits=1 |
| uncapped-8 | idm | True | 2.621 | 153.63 | 7 | 7 | 0 | 0 | 2.5 | True |  |
| uncapped-8 | udm | True | 5.055 | 79.65 | 8 | 9 | 0 | 0 | 0.7 | True | segments=8 splits=0 |
| resume-8 | idm | True | 8.628 | 46.67 | 23 | 23 | 0 | 0 | 216.8 | True |  |
| resume-8 | udm | True | 11.192 | 35.98 | 16 | 18 | 0 | 0 | 2.0 | True | segments=8 splits=0 |
| outage-8 | idm | True | 14.056 | 28.65 | 22 | 22 | 1974 | 8 | 2.6 | True |  |
| outage-8 | udm | True | 19.543 | 20.6 | 16 | 17 | 24 | 8 | 0.0 | True | segments=8 splits=0 |
| drop-8 | idm | True | 9.632 | 41.8 | 59 | 59 | 0 | 44 | 12.8 | True |  |
| drop-8 | udm | True | 11.489 | 35.05 | 56 | 58 | 0 | 48 | 12.4 | True | segments=9 splits=1 |
| no-range-1 | idm | True | 1.814 | 147.98 | 4 | 4 | 0 | 0 | 0.5 | True |  |
| no-range-1 | udm | True | 4.491 | 59.77 | 2 | 2 | 0 | 0 | 0.2 | True | segments=1 splits=0 |
| slow-start-2 | idm | True | 21.268 | 12.62 | 9 | 9 | 30 | 0 | 0.6 | True |  |
| slow-start-2 | udm | True | 21.859 | 12.28 | 8 | 17 | 40 | 0 | 4.2 | True | segments=16 splits=8 |
| public-alpine-8 | idm | True | 64.802 | 3.96 | 8 |  |  |  |  |  |  |
| public-alpine-8 | udm | True | 64.734 | 3.97 | 8 |  |  |  |  |  | segments=18 splits=10 |
| public-kernel-8 | idm | True | 40.135 | 3.69 | 8 |  |  |  |  |  |  |
| public-kernel-8 | udm | True | 50.652 | 2.92 | 9 |  |  |  |  |  | segments=24 splits=16 |
