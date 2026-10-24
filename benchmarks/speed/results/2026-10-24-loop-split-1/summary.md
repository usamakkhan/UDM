| case | client | done | wall s | MB/s | conns | requests | rejected | drops | wasted MiB | sha ok | note |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|---|---|
| single-8 | udm | True | 7.886 | 51.06 | 9 | 25 | 0 | 0 | 0.2 | True | segments=24 splits=0 |
| uncapped-8 | udm | True | 2.667 | 150.98 | 11 | 31 | 0 | 0 | 4.4 | True | segments=30 splits=6 |
| drop-8 | udm | True | 9.483 | 42.46 | 56 | 73 | 0 | 48 | 8.7 | True | segments=24 splits=0 |
