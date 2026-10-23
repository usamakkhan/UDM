| case | client | done | wall s | MB/s | conns | requests | rejected | drops | wasted MiB | sha ok | note |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|---|---|
| single-8 | idm | True | 9.305 | 43.27 | 15 | 15 | 0 | 0 | 1.6 | True |  |
| single-8 | udm | True | 13.617 | 29.57 | 8 | 9 | 0 | 0 | 0.7 | True | segments=8 splits=0 |
| single-16 | idm | True | 9.073 | 44.38 | 49 | 49 | 0 | 0 | 5.7 | True |  |
| single-16 | udm | True | 9.825 | 40.98 | 16 | 17 | 0 | 0 | 0.7 | True | segments=16 splits=0 |
| uncapped-8 | idm | True | 4.795 | 83.97 | 14 | 14 | 0 | 0 | 4.6 | True |  |
| uncapped-8 | udm | True | 6.736 | 59.78 | 9 | 13 | 0 | 0 | 3.2 | True | segments=12 splits=4 |
| resume-8 | idm | True | 0.02 | 20132.66 | 19 | 18 | 0 | 0 | 0.7 | True |  |
| resume-8 | udm | True | 9.288 | 43.35 | 16 | 19 | 0 | 0 | 6.7 | True | segments=9 splits=1 |
| outage-8 | idm | True | 16.848 | 23.9 | 32 | 32 | 1734 | 8 | 2.2 | True |  |
| outage-8 | udm | False | 9.061 | 44.44 | 8 | 9 | 24 | 8 |  |  | HTTP send failed (Windows error 12030). segments=8 splits=0 |
| drop-8 | idm | True | 10.961 | 36.74 | 64 | 64 | 0 | 44 | 10.0 | True |  |
| drop-8 | udm | False | 27.093 | 14.86 | 30 | 31 | 0 | 25 |  |  | HTTP read failed (Windows error 12030). segments=8 splits=0 |
| no-range-1 | idm | True | 4.395 | 61.08 | 4 | 4 | 0 | 0 | 0.5 | True |  |
| no-range-1 | udm | True | 6.939 | 38.69 | 2 | 2 | 0 | 0 | 0.2 | True | segments=1 splits=0 |
| slow-start-2 | idm | True | 23.455 | 11.44 | 9 | 9 | 65 | 0 | 0.8 | True |  |
| slow-start-2 | udm | False | 5.466 | 49.11 | 2 | 3 | 24 | 0 |  |  | HTTP send failed (Windows error 12030). segments=8 splits=0 |
| public-alpine-8 | idm | True | 63.313 | 4.06 | 8 |  |  |  |  |  |  |
| public-alpine-8 | udm | True | 161.924 | 1.59 | 1 |  |  |  |  |  | segments=9 splits=1 |
| public-kernel-8 | idm | True | 40.569 | 3.65 | 8 |  |  |  |  |  |  |
| public-kernel-8 | udm | True | 61.734 | 2.4 | 1 |  |  |  |  |  | segments=9 splits=1 |
