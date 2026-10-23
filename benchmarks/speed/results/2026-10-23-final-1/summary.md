| case | client | done | wall s | MB/s | conns | requests | rejected | drops | wasted MiB | sha ok | note |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|---|---|
| single-8 | udm | True | 9.025 | 44.62 | 9 | 51 | 0 | 0 | 0.7 | True | segments=50 splits=2 |
| uncapped-8 | udm | True | 4.967 | 81.07 | 15 | 63 | 0 | 0 | 9.0 | True | segments=62 splits=14 |
| outage-8 | udm | True | 26.866 | 14.99 | 29 | 78 | 25 | 8 | 8.6 | True | segments=69 splits=21 |
| no-range-1 | udm | True | 6.915 | 38.82 | 2 | 2 | 0 | 0 | 0.5 | True | segments=1 splits=0 |
