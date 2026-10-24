| case | client | done | wall s | MB/s | conns | requests | rejected | drops | wasted MiB | sha ok | note |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|---|---|
| single-8 | idm | True | 11.046 | 36.45 | 12 | 12 | 0 | 0 | 4.2 | True |  |
| single-8 | udm | True | 8.117 | 49.61 | 9 | 25 | 0 | 0 | 1.0 | True | segments=24 splits=0 |
| uncapped-8 | idm | True | 12.568 | 32.04 | 13 | 13 | 0 | 0 | 3.9 | True |  |
| uncapped-8 | udm | True | 8.891 | 45.29 | 18 | 47 | 0 | 0 | 13.9 | True | segments=46 splits=22 |
| no-range-1 | idm | True | 3.424 | 78.4 | 4 | 4 | 0 | 0 | 0.8 | True |  |
| no-range-1 | udm | True | 2.329 | 115.26 | 2 | 2 | 0 | 0 | 0.2 | True | segments=1 splits=0 |
