# Bounded initial HTTP work queue — 9.91

Native **9.91**, both browser source manifests **0.73.55**. This follows the rejected 9.90 one-range-per-worker policy; [its losing controls and exact intermediate source are retained](initial-range-plan-9.90.md). This candidate is not installed or released.

## Assignment and ownership contract

The first full response remains usable immediately. Before initial parallel admission, construct a bounded queue using the existing **4 MiB fresh-range scale**, capped at two pieces per configured worker and 64 pieces overall. One worker still means one piece. Large-file offsets are divided before addition; the spans are contiguous, balanced and cover the final byte without overflow.

The first helper asks for exactly the second piece, with the existing 64-byte adjacent boundary proof. Its Content-Range total is checked against the whole representation, independently of its shorter requested end. ETag/date, encoding and exact returned range checks remain in place. Only a validated helper may publish the queue. The complete new segment and owner maps are prepared before either replaces live ownership. No primary progress or pending read may cross the proposed first boundary. If the primary is already ahead, retain the existing dynamic fallback instead of rewinding it. If the helper is refused, ignored or cancelled, retain the original full-response path.

Concurrency remains the configured worker limit; queued pieces are not additional simultaneous connections. A free worker first takes an unstarted piece, including the existing adjacent preference. Faster connections can thus do more useful work without repeatedly bisecting healthy responses. The current slow/stalled-response assistance, connection-refusal pacing, representation validation, overlap checks, saved range plans and recovery remain available. Pending read/write ownership from 9.89 is retained. No new delay, sleep, optimistic Complete state, synthetic rate or removed durability barrier is introduced.

## Qualification and delivery

All five native products build as 9.91 and their actual resource versions and SHA-256 values are retained. Both source manifests are 0.73.55. **244 native assertions and six Python accounting tests pass**: 127 focused backend assertions, 17 plain/verified/checksum-rejection/ranged HTTP assertions, 43 gzip assertions, 37 real-scenario assertions and 20 frozen/current control assertions. This is focused qualification, not a full suite.

The seven real scenarios cover 64 MiB/eight workers; 16 MiB/three workers; unadvertised ranges; a two-connection origin limit; ignored ranges; a primary-response head start; and 64 MiB Pause/Resume. All outputs have exact independent hashes and lengths, saved Complete/checksum records and joined worker shutdown. The ordinary accepted-helper cases additionally verify the actual bounded initial Range headers. Pause retains genuine incomplete progress and does not publish a partial output.

### Same-harness engine control

64 MiB output, C:, alternating frozen/current/current/frozen. Only the qualified 9.89 Transfer object is substituted for the frozen control; all other objects and the harness are current. This is neither the whole 9.89 App nor IDM. Engine timing ends after joined manager shutdown; independent proof hashing follows and parent lifetime including proof is recorded separately.

| Order / variant | Origin bytes | Requests | Engine through joined stop, ms |
| --- | ---: | ---: | ---: |
| 1 / frozen 9.89 transfer | 67,305,768 | 12 | 2,246.640 |
| 2 / queued 9.91 | 67,265,559 | 19 | 2,070.383 |
| 3 / queued 9.91 | 67,257,434 | 20 | 1,983.454 |
| 4 / frozen 9.89 transfer | 67,558,665 | 14 | 2,371.722 |

Both current samples improve total engine time and origin surplus in this set. More requests do not mean more wasted bytes: queued requests have bounded ends. The ownership analyzer reconciles complete coverage and all surplus; current proof prefixes are 1,152 / 1,216 bytes and beyond-owner sends are 155,543 / 147,354 bytes. Surplus remains, so this does not establish zero duplicate traffic or broad speed acceptance. Two samples per variant are insufficient to establish tail behavior.

### Normal app versus installed IDM

One pair per case, also on **C:**, with a deterministic owned origin. This is normal uninstrumented UDM 9.91 versus the installed IDM executable; other-session processes are not stopped. All six output hashes and lengths pass, all UDM saved catalogs report Complete with the expected digest, and all tracked processes exit. The harness restores IDM's two connection settings and verifies the restoration. IDM returns launcher code 1 despite correct files, as in earlier comparisons; that code's success semantics remain unestablished.

Each cell is **UDM / IDM**. Availability means a successful exclusive file open. Tails begin at the origin's last observed body send.

| Case | Total process lifetime, s | First availability tail, ms | Process exit tail, ms | Tail through post-exit flush, ms | Total through post-exit flush, s |
| --- | ---: | ---: | ---: | ---: | ---: |
| Sequential 4 MiB | 1.246 / 2.646 | 32.155 / 57.273 | 97.337 / 459.131 | 106.386 / 493.493 | 1.255 / 2.680 |
| Sequential 64 MiB | 8.789 / 9.886 | 38.049 / 46.739 | 83.880 / 304.497 | 94.411 / 339.038 | 8.799 / 9.920 |
| Split 64 MiB | 1.897 / 3.797 | 47.883 / 118.033 | 92.909 / 388.935 | 102.730 / 627.450 | 1.907 / 4.036 |

The split origins send 67,371,956 bytes in UDM and 69,233,527 in IDM for a 67,108,864-byte output. UDM improves all listed endpoints in these **single C: observations**. The harness's single-value “median” does not constitute a distribution. These results do not overturn earlier losing D: samples, establish GUI paint parity or measure every external Windows service, device-cache persistence or power-loss durability. The explicit flush is symmetric and follows observed app/thread exit. Request snapshots need not prove every origin handler has retired.

**The overall IDM parity gate remains open.** Next repeat alternating normal-product samples on D: when the storage/cleanup issue permits, retaining total time, availability, exit and post-exit flush plus all losing samples. Then qualify the installed desktop/browser handoff and UI/media matrix separately. No installer, installation, commit, push or release is included here. The source extension increment tracks this delivery; no browser behavior changes are introduced by this increment.

[Sanitized build/source records, all samples and exact source snapshots](validation/initial-range-queue-9.91/). Sanitized private drivers contain local-path placeholders and serve as provenance rather than directly runnable scripts. The host installed programs, manifests and shared tools are checked separately against preserved hashes; personal catalogs are not restored or replaced.

### Storage and cleanup limitation

Automatic approval review previously rejected both fixture-folder cleanup and a narrower payload-only deletion with “blocked by policy.” No alternative deletion route was used. Temporary files from 9.89, rejected 9.90 and current 9.91 remain: approximately **1,695.68 MiB** across their fixture folders, including catalogs and evidence. Current native programs/core objects are retained separately. [Exact retained-folder inventory](validation/initial-range-queue-9.91/cleanup-status.json). Final free space is approximately C: 4.18 GiB and D: 5.20 GiB; UDM-Lab remains off. No new lab start or host installation is claimed.
