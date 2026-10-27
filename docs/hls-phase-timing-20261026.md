# Adaptive download phases and confirmation timing

Recorded HLS/DASH transfers now report Receiving segments, Combining streams,
Waiting for confirmation, and Complete at the corresponding stages. Completed
prefetches no longer retain a misleading waiting phase in download information.

Elapsed time excludes idle time spent holding the verified output in File Info.
Cancelling after assembly also preserves the network time measured when the
segments finished, rather than adding assembly and confirmation-wait time to it.
No artificial speed burst is introduced; the existing exact-size/prefetch bytes
and publication behavior are retained.

Validation: `Udm.NativeTests.exe --adaptive-prefetch-checks` passed **222 checks,
0 failures**, including the previous media/adaptive tests and new tests that
hold confirmation for two seconds before Start or Cancel. These verify elapsed
time excludes the wait, network time remains unchanged, and completed adaptive
downloads report Complete. Both test and desktop builds succeeded.

Candidate SHA-256:
`95133880EF37CAEB6539CE7A3EFE08B68CA37C8A7968F4FC000C8E14144999D3`.

Installation succeeded and the installed executable's SHA-256 matches the tested
candidate. Earlier live quality-matrix results cover the preceding build and are
not claimed as a live test of this timing-only follow-up.

[Validation record](validation/hls-phase-timing-20261026.json).
