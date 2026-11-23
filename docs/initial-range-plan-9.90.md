# Bounded initial ranges — 9.90 rejected for installation

This intermediate candidate built all five native programs as **9.90** and advanced both source extension manifests to **0.73.54**. It was **not installed**. The source then advanced to the queued-piece follow-up, 9.91 / 0.73.55.

## Attribution behind the change

The fresh 9.89 split controls now reconcile every extra origin byte against the final segment map. Deliberate adjacent-range proofs account for only **896 / 960 / 704 / 896 bytes**. Sends beyond final parent ownership account for **260,069 / 220,740 / 1,709,666 / 298,348 bytes**. Their sums exactly match the saved surplus totals, with complete resource coverage and no holes. Client charged surplus equals the proof-prefix totals in these observations. This distinguishes socket-accepted delivery beyond a shortened owner from bytes charged through UDM's read loop; it does not identify where the unconsumed bytes were buffered.

[Full per-request attribution](validation/http-read-reservation-9.89/split-control/final-owner-attribution.json). The read-only analyzer refuses unknown resume offsets, retries, missing coverage and inconsistent totals. Six analyzer tests pass.

## Implementation and correctness

9.90 assigned one fixed initial range per worker after a helper proved the same representation, retaining the original full response. This prevents the initial helper tree from repeatedly requesting tails that another initial worker is about to receive. Initial range admission prepares the new segment/owner maps before publishing either; if allocation or range proof fails, the original full response retains its previous assignment. A primary response already ahead of the proposed boundary retains the existing dynamic fallback.

All **244 native assertions** passed: 127 focused backend assertions; 17 plain/verified/checksum-rejection/ranged HTTP assertions; 43 gzip assertions; 37 assertions across seven real download scenarios; and 20 frozen/current ABBA control assertions. The real scenarios include 64 MiB/eight workers, 16 MiB/three workers, unadvertised ranges, a two-connection origin limit, ignored ranges, a primary-response head start and 64 MiB Pause/Resume. Exact hashes and durable Complete/checksum records pass. The ordinary accepted-helper cases also verify the bounded request headers.

## Performance rejection

64 MiB output, same current harness/other objects, only frozen qualified 9.89 Transfer object substituted for the control. C: engine timings end after joined manager shutdown, before the independent proof hash. Parent lifetime including proof is retained separately.

| Order / variant | Origin bytes | Requests | Engine through joined stop, ms |
| --- | ---: | ---: | ---: |
| 1 / frozen 9.89 transfer | 67,304,875 | 14 | 1,981.193 |
| 2 / current 9.90 | 68,113,390 | 12 | 2,810.986 |
| 3 / current 9.90 | 67,293,740 | 14 | 2,795.078 |
| 4 / frozen 9.89 transfer | 67,356,771 | 13 | 2,523.238 |

**Both current samples lose total engine time.** The first also sends more surplus bytes. A passing correctness suite does not qualify this scheduling policy for installation. These two samples per variant are engine controls, not a normal App/IDM comparison or a finishing/availability acceptance gate. No samples are omitted.

One large fixed assignment per worker leaves no queue for faster connections until a worker completes; slow responses still require shortening. The next candidate queues bounded pieces while retaining concurrency limits, identity checks and slow/stalled-response recovery. This is a work-assignment change, with no new sleep/pacing interval or weakened flush barrier.

[Sanitized evidence and exact hash-verified intermediate source snapshots](validation/initial-range-plan-9.90/). The private build driver's final console sentence inherited “9.89”; its captured build manifest and actual compiled resource identify 9.90. Temporary payloads remain because automatic approval review blocked cleanup on the preceding increment; no alternative deletion path is used.
