# UDM 9.77 capped split-download lifecycle observation

Two normal-product downloads used the same owned two-connection loopback origin policy, a 32 MiB deterministic representation, 2 MiB/s per accepted connection, 20 ms response latency and direct routing. The first requested two connections; the second requested eight. These are one observation per configuration, in that order, without retries of the benchmark or a statistical performance claim.

| Observation | Two configured connections | Eight configured connections |
| --- | ---: | ---: |
| Final size, independently verified SHA-256 | 33,554,432 bytes, correct | 33,554,432 bytes, correct |
| Accepted connections / successful requests | 2 / 2 | 2 / 2 |
| Refused opens | 0 | 3 |
| Peak accepted connections | 2 | 2 |
| Sent body bytes | 33,619,984 | 33,619,984 |
| Launch to actual process exit | 8,582.195 ms | 9,008.755 ms |
| Last origin body send to actual process exit | 209.393 ms | 579.696 ms |
| Exit to explicit output flush completion | 29.769 ms | 105.856 ms |
| Explicit post-exit flush call itself | 18.567 ms | 83.245 ms |

Both jobs finish with two segments and zero successful dynamic splits. The surplus 65,552 body bytes are present in both configurations; the primary response already in flight can send beyond its shortened assigned range, and the helper includes its required overlap. Extra failed opens in the eight-connection case occur at approximately 0.463, 3.502 and 6.522 seconds. They do not multiply successful requests or increase received body overhead in these observations. This does not exercise sixteen active workers, multiple split jobs, authentication, total outage, cancellation or capacity changes.

## What the finishing records establish

The eight-connection observation remains slower, and is retained. Its recorded final writeback flush takes 270.494 ms versus 105.562 ms. The pre-publication metadata interval takes 157.828 ms versus 8.474 ms. That interval includes the elapsed work around the zone marker/server-date path; it is not an isolated measurement of one Windows call. Advisory hashing takes 119.620 versus 44.923 ms in its overlapping worker, while the owner join remains approximately 0.020–0.024 ms. These overlapping durations must not be added together as independent serial delays.

The evidence points to variable storage/metadata work rather than a late successful HTTP request, but does not prove that requesting eight connections causes the storage difference. Fixed order, one sample each, scheduling, filesystem filters and disk contention remain confounders. Do not add pacing, remove flushes or change admission defaults on this evidence. The next attribution step is to separate zone-stream open/write/close, timestamp operations, writeback and final catalog work on the current source, then correlate the blocking operations with storage activity. An isolated no-regression comparison must follow any actual fix.

## Measurement and correction

The driver waits for the actual owned primary process to exit normally, maps its Windows exit timestamp to the monotonic origin clock, checks clock drift, then opens and explicitly flushes the completed output before hashing it. The final saved catalog must report Complete and the full byte count. This includes application-owned work ending before process exit and a separate output durability observation; it does not establish completion of unrelated Windows services, GUI timing, or hardware persistence beyond the operating-system flush contract. The short interval from exit to explicit flush also includes observation/polling overhead.

The initial private driver generated an instance tag longer than the documented 32-character limit. That launch exited before adding a job and produced no payload. The driver was corrected to shorten its tag and capture stderr and exit code. Its original source and correction record are retained. No production source changed, and no failed transfer was silently discarded as a successful timing sample.

## Evidence and cleanup

[Both complete records](validation/capped-split-lifecycle-9.77/results.json), [terminal status](validation/capped-split-lifecycle-9.77/terminal.json), [driver](validation/capped-split-lifecycle-9.77/run-capped-split-lifecycle-9.77.py), [initial setup correction](validation/capped-split-lifecycle-9.77/setup-correction.json), [final audit](validation/capped-split-lifecycle-9.77/final-audit.json), [export hashes](validation/capped-split-lifecycle-9.77/export-manifest.json).

The two 32 MiB payloads were individually size/hash checked and deleted after each record was saved: 67,108,864 logical bytes. No payload remains under this owned work folder. Both application processes exited normally and both origins were closed; no forced app termination was used. All 694 captured build inputs, 42 current core objects, five current products and eleven protected installed/catalog/helper files still match their recorded hashes. Compact private diagnostic catalogs remain. Version stays 9.77 / browser manifests 0.73.41 because this is qualification only. There is no installer, installation or IDM timing result.
