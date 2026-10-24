| case | client | done | wall s | MB/s | conns | requests | rejected | drops | wasted MiB | sha ok | note |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|---|---|
| single-8 | udm | True | 13.104 | 30.73 | 17 | 65 | 0 | 0 | 10.3 | True | segments=64 splits=16 |
| uncapped-8 | udm | True | 9.888 | 40.72 | 11 | 58 | 0 | 0 | 5.2 | True | segments=57 splits=9 |
| drop-8 | udm | True | 14.516 | 27.74 | 50 | 100 | 0 | 46 | 4.7 | True | segments=53 splits=5 |
