| case | client | done | wall s | MB/s | conns | requests | rejected | drops | wasted MiB | sha ok | note |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|---|---|
| single-8 | udm | True | 14.638 | 27.51 | 19 | 67 | 0 | 0 | 11.4 | True | segments=66 splits=18 |
| uncapped-8 | udm | True | 6.103 | 65.98 | 11 | 58 | 0 | 0 | 5.6 | True | segments=57 splits=9 |
| outage-8 | udm | True | 17.591 | 22.89 | 20 | 65 | 56 | 8 | 9.1 | True | segments=56 splits=8 |
| no-range-1 | udm | True | 4.306 | 62.34 | 2 | 2 | 0 | 0 | 0.2 | True | segments=1 splits=0 |
