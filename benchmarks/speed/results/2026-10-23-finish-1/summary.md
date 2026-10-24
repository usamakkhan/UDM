| case | client | done | wall s | MB/s | conns | requests | rejected | drops | wasted MiB | sha ok | note |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|---|---|
| single-8 | udm | True | 11.699 | 34.42 | 9 | 52 | 0 | 0 | 2.4 | True | segments=51 splits=3 |
| uncapped-8 | udm | True | 8.412 | 47.87 | 11 | 58 | 0 | 0 | 5.4 | True | segments=57 splits=9 |
| drop-8 | udm | True | 15.624 | 25.77 | 54 | 107 | 0 | 47 | 6.0 | True | segments=59 splits=11 |
| no-range-1 | udm | True | 4.642 | 57.83 | 2 | 2 | 0 | 0 | 0.2 | True | segments=1 splits=0 |
