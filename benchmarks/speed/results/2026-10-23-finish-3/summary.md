| case | client | done | wall s | MB/s | conns | requests | rejected | drops | wasted MiB | sha ok | note |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|---|---|
| single-8 | udm | True | 12.776 | 31.52 | 12 | 58 | 0 | 0 | 5.1 | True | segments=57 splits=9 |
| uncapped-8 | udm | True | 10.498 | 38.36 | 16 | 64 | 0 | 0 | 9.2 | True | segments=63 splits=15 |
| drop-8 | udm | True | 14.739 | 27.32 | 51 | 101 | 0 | 46 | 4.2 | True | segments=54 splits=6 |
| no-range-1 | udm | True | 4.845 | 55.4 | 2 | 2 | 0 | 0 | 0.2 | True | segments=1 splits=0 |
