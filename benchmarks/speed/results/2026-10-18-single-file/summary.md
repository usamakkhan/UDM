| case | client | done | wall s | MB/s | conns | requests | rejected | drops | wasted MiB | sha ok | note |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|---|---|
| single-8 | udm | True | 7.757 | 51.91 | 9 | 54 | 0 | 0 | 2.1 | True | segments=53 splits=5 |
| uncapped-8 | udm | True | 4.052 | 99.37 | 9 | 55 | 0 | 0 | 4.2 | True | segments=54 splits=6 |
| resume-8 | udm | True | 7.059 | 57.04 | 17 | 67 | 0 | 0 | 71.5 | True | segments=49 splits=1 |
| no-range-1 | udm | True | 2.879 | 93.24 | 2 | 2 | 0 | 0 | 0.2 | True | segments=1 splits=0 |
