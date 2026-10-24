| case | client | done | wall s | MB/s | conns | requests | rejected | drops | wasted MiB | sha ok | note |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|---|---|
| single-8 | idm | True | 16.05 | 25.09 | 18 | 17 | 0 | 0 | 4.7 | True |  |
| single-8 | udm | True | 18.26 | 22.05 | 12 | 38 | 0 | 0 | 7.2 | True | segments=37 splits=13 |
| uncapped-8 | idm | True | 9.572 | 42.07 | 16 | 16 | 0 | 0 | 5.1 | True |  |
| uncapped-8 | udm | True | 5.634 | 71.47 | 12 | 33 | 0 | 0 | 7.4 | True | segments=32 splits=8 |
| no-range-1 | idm | True | 3.838 | 69.94 | 4 | 4 | 0 | 0 | 0.8 | True |  |
| no-range-1 | udm | True | 3.859 | 69.56 | 2 | 2 | 0 | 0 | 0.2 | True | segments=1 splits=0 |
