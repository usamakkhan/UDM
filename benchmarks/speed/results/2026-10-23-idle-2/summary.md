| case | client | done | wall s | MB/s | conns | requests | rejected | drops | wasted MiB | sha ok | note |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|---|---|
| single-8 | udm | True | 8.412 | 47.87 | 10 | 55 | 0 | 0 | 2.9 | True | segments=54 splits=6 |
| uncapped-8 | udm | True | 7.521 | 53.54 | 12 | 59 | 0 | 0 | 5.2 | True | segments=58 splits=10 |
| outage-8 | udm | True | 21.728 | 18.53 | 31 | 84 | 58 | 8 | 11.8 | True | segments=75 splits=27 |
| no-range-1 | udm | True | 14.157 | 18.96 | 2 | 2 | 0 | 0 | 0.2 | True | segments=1 splits=0 |
