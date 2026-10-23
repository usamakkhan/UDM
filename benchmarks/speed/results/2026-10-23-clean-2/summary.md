| case | client | done | wall s | MB/s | conns | requests | rejected | drops | wasted MiB | sha ok | note |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|---|---|
| single-8 | udm | True | 12.063 | 33.38 | 12 | 57 | 0 | 0 | 4.6 | True | segments=56 splits=8 |
| uncapped-8 | udm | True | 9.395 | 42.86 | 11 | 57 | 0 | 0 | 5.7 | True | segments=56 splits=8 |
| outage-8 | udm | True | 16.618 | 24.23 | 23 | 70 | 57 | 8 | 8.4 | True | segments=61 splits=13 |
| no-range-1 | udm | True | 5.278 | 50.86 | 2 | 2 | 0 | 0 | 0.2 | True | segments=1 splits=0 |
