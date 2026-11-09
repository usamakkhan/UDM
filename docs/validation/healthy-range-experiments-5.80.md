# Healthy HTTP range experiments

The recorded IDM comparison and UDM traces showed some extra late range requests on healthy origins. Two experiments tested leaving short tails on the current response when an ideal midpoint split could save little time. The 5.78 rule started after the first range finished and used a 150 ms estimated saving. The 5.79 rule also started once two responses were accepted and used 250 ms.

Each experiment alternated its candidate and the frozen 5.77 build three times for each file size. The owned loopback origin served 24 MiB at 5 MiB/s per connection with 20 ms setup latency and 192 MiB at 10 MiB/s with 25 ms setup latency. UDM used private catalogs and requested eight connections. All 24 files matched their expected SHA-256.

| Experiment | File | 5.77 requests | Candidate requests | 5.77 median | Candidate median |
|---|---|---|---|---:|---:|
| 5.78 | 24 MiB | 7 / 7 / 7 | 7 / 7 / 8 | 2.67 s | 3.04 s |
| 5.78 | 192 MiB | 8 / 8 / 12 | 9 / 8 / 8 | 4.58 s | 4.14 s |
| 5.79 | 24 MiB | 7 / 7 / 7 | 8 / 7 / 7 | 2.68 s | 2.78 s |
| 5.79 | 192 MiB | 8 / 8 / 8 | 8 / 8 / 10 | 4.73 s | 5.71 s |

The 24 MiB origins reached six concurrent connections and the 192 MiB origins reached eight. One 5.79 run took 8.318 seconds with eight requests. Its native trace recorded 1.329 seconds from the last byte to the join marker and another 1.406 seconds to the network-end marker. This shows that request count alone does not account for that run's elapsed time. Those intervals do not identify which operation caused the delay.

The 5.78 native run passed 72 of 73 checks. Its delayed-helper fixture missed the midpoint timing condition while compilation was active. The 5.79 native run passed all 73 checks with compilation complete. Both candidates lacked a repeatable request reduction. Version 5.80 restores `native/Transfer.cpp` byte-for-byte to the 5.77 build input, retaining its qualified HTTP 403 handling. The current IDM application was already running, so the new runs compare isolated UDM builds only. [5.78 build](healthy-range-tails-5.78-build.json) · [5.79 build](healthy-range-maturity-5.79-build.json) · [Restored build](restored-range-policy-5.80-build.json)

Both 5.80 smoke downloads matched the expected hash. The 24 MiB and 192 MiB transfers used 10 and eight requests respectively. These single runs verify the rebuilt executables on the same origins. Raw traces are in the Windows local temporary folders `udm-healthy-range-5.78`, `udm-healthy-range-5.79`, and `udm-healthy-range-5.80`.
