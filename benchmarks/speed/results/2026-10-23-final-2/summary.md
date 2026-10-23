| case | client | done | wall s | MB/s | conns | requests | rejected | drops | wasted MiB | sha ok | note |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|---|---|
| single-8 | udm | True | 13.716 | 29.36 | 14 | 64 | 0 | 0 | 7.5 | True | segments=63 splits=15 |
| uncapped-8 | udm | True | 5.362 | 75.09 | 16 | 63 | 0 | 0 | 9.7 | True | segments=62 splits=14 |
| outage-8 | udm | True | 15.923 | 25.29 | 27 | 76 | 53 | 7 | 10.5 | True | segments=68 splits=20 |
| no-range-1 | udm | True | 6.157 | 43.6 | 2 | 2 | 0 | 0 | 0.2 | True | segments=1 splits=0 |
