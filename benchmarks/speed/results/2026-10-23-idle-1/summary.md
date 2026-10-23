| case | client | done | wall s | MB/s | conns | requests | rejected | drops | wasted MiB | sha ok | note |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|---|---|
| single-8 | udm | True | 8.042 | 50.07 | 10 | 51 | 0 | 0 | 0.7 | True | segments=50 splits=2 |
| uncapped-8 | udm | True | 15.032 | 26.79 | 11 | 59 | 0 | 0 | 5.8 | True | segments=58 splits=10 |
| outage-8 | udm | True | 18.328 | 21.97 | 19 | 65 | 57 | 8 | 3.6 | True | segments=56 splits=8 |
| no-range-1 | udm | True | 8.464 | 31.71 | 2 | 2 | 0 | 0 | 0.2 | True | segments=1 splits=0 |
