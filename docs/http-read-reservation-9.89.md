# Pending HTTP reads own their assigned bytes — 9.89

Native **9.89**, Chromium and Firefox source manifests **0.73.53**. All five native products are built and their product versions verified. This candidate is not installed or released; no installer is created. The browser increment records this delivery and does not introduce browser behavior changes.

## Problem and implementation

The split scheduler excluded a worker's current disk write from reassignment, but its next HTTP read could already be pending beyond that fence. Committing the preceding write also cleared the fence while the next read remained outstanding. Another worker could therefore receive bytes already requested by the first worker.

`Transfer.cpp` now reserves the live read interval under the same manager lock used by the split scheduler. It rechecks the actual segment end immediately before issuing the read, bounds the request to remaining ownership, and retains the pending-read fence when committing the previous write. An exhausted shortened assignment issues no further body read. `HttpReadReservation.hpp` checks bounds without adding past the signed offset limit. The declared-response one-byte EOF/excess-data check remains in place.

Response teardown still clears ownership. Validators, overlap verification, checksum generation, publication barriers, completion semantics and durable recovery are unchanged. Reserving pending reads can leave a helper less tail work; it does not eliminate bytes buffered by HTTP or the origin. No pacing or flush suppression is introduced.

## Qualification

**213 assertions pass**, including the frozen-transfer controls:

| Coverage | Assertions |
| --- | ---: |
| Read reservation, staging admission/reuse, transfer cleanup, publication availability and prepared cleanup | 116 |
| Real HTTP plain, verified, checksum rejection and range hashing | 17 |
| Real HTTP gzip, single-response reuse and encoded-offset Pause/Resume | 43 |
| New 4 MiB sequential, 64 MiB split and 64 MiB Pause/Resume workflows | 17 |
| Alternating frozen/current/current/frozen 64 MiB split controls | 20 |

The real split and resume runs observe multiple origin range requests, exact output length, independent SHA-256, saved Complete/checksum state and joined manager shutdown. Paused output is not prematurely published. A focused matrix includes the former fence counterexample, pre-read reassignment, exhausted bounds, EOF and overflow edges. These are focused checks, not a full suite, fresh UI acceptance or a power-loss test.

All fixture output is on **C:** because D: had nearly filled. A derived private driver initially mislabeled the six suites as D:; the corrected record labels C:, preserves the original metadata and both driver hashes, and changes no assertion or executable. Sanitized drivers retain placeholders in place of local paths and are provenance records rather than directly runnable scripts. [Build, sources, results and metadata correction](validation/http-read-reservation-9.89/).

## Efficiency control and unresolved gate

The control substitutes only qualified 9.88 `Transfer.obj` into the same current harness and other objects. It is not the whole 9.88 product, a normal App process or IDM. Each output is 67,108,864 bytes. All samples are retained:

| Order / variant | Origin body bytes | Requests | Engine through joined stop, ms |
| --- | ---: | ---: | ---: |
| 1 / frozen transfer | 67,369,829 | 15 | 2,211.667 |
| 2 / current | 67,330,564 | 16 | 1,967.763 |
| 3 / current | 68,819,234 | 12 | 2,658.526 |
| 4 / frozen transfer | 67,408,108 | 15 | 2,792.933 |

The second current sample sends substantially more surplus bytes. Two samples per variant do not establish an efficiency or speed win; **PERF-EXTRA remains open**. Engine timing ends after `Manager::stop()` joins owned work; the independent proof hash is read afterward. Parent lifetime including that proof is retained separately. This is not the normal-product availability/exit/post-exit-flush gate and does not close the known IDM finishing deficit.

Next attribute remaining surplus to transport buffering, cancelled or extended assignments and overlap validation using per-request ranges. Require repeated alternating normal-product comparisons against IDM on the same volume before claiming fewer wasted bytes without total-time, finishing or recovery regression. Preserve durability and all losing samples. [Current ordered backlog](idm-parity-execution-plan-20261107.md).

Sanitized evidence is exported. Automatic approval review rejected both recursive fixture cleanup and a narrower payload-only deletion with “blocked by policy”; neither command ran. Temporary fixture payloads and isolated catalogs therefore remain. Current native products, required core objects, source and build provenance remain available. The host installation and personal catalogs are not replaced by these checks.
