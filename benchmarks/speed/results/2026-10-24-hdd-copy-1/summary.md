| case | client | done | wall s | MB/s | conns | requests | rejected | drops | wasted MiB | sha ok | note |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|---|---|
| single-8 | idm | True | 29.406 | 13.69 | 14 | 14 | 0 | 0 | 2.0 | True |  |
| single-8 | udm | True | 10.546 | 38.18 | 9 | 25 | 0 | 0 | 0.2 | True | segments=24 splits=0 |
| uncapped-8 | idm | True | 10.019 | 40.19 | 20 | 20 | 0 | 0 | 4.5 | True |  |
| uncapped-8 | udm | True | 7.539 | 53.41 | 12 | 33 | 0 | 0 | 7.9 | True | segments=32 splits=8 |
| no-range-1 | idm | True | 4.696 | 57.16 | 4 | 4 | 0 | 0 | 0.5 | True |  |
| no-range-1 | udm | True | 10.021 | 26.79 | 2 | 2 | 0 | 0 | 0.2 | True | segments=1 splits=0 |
