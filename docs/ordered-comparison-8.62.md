# Ordered versus normal build performance, 8.62

**Do not enable the candidate by default yet.** All 36 transfers complete correctly, but the candidate does not meet the requested no-regression result. Its median 64 MiB split wall time improves on both drives, while its C: finishing tail regresses and one of six split pairs is slower overall. These are UDM-versus-UDM results, not an IDM comparison.

## Matched experiment

The [harness](../benchmarks/speed/compare-ordered-body.py) runs the frozen normal and private ordered 8.62 application products, with hashes checked before each launch. Both use isolated catalogs and destinations, identical settings, an 8 MB/s-per-connection synthetic HTTP origin with 20 ms response latency and a known seed. Each drive has three repetitions of 4 MiB sequential, 64 MiB sequential and 64 MiB split, with two variants per repetition: 2 × 3 × 3 × 2 = 36 transfers. First-client order reverses across trial, case and drive. No other qualification run was launched concurrently by this harness; unrelated machine activity is not controlled.

All applications exit normally. Timings use actual process-exit timestamps from GetProcessTimes mapped to the origin clock, with drift checked. A separate explicit output flush is measured after exit. Exact file length, independently expected SHA-256, completed catalog digest, final counters and absence of pending publication journals pass. Every owned payload is removed only after those checks. No GUI timing, registry changes, installation or administrator prompt is involved.

The private writer is actually used and joins complete 64 MiB accepted/written coverage in all split candidate runs. Both sequential controls remain on the synchronous path, so their differences cannot be attributed to ordered IO. Their variability is evidence against treating a single elapsed-time difference as a mechanism-level speed gain.

## Results

Values below are medians of three runs, in milliseconds. “Wall” runs from launch to actual exit. “Durable tail” runs from the origin's last body send through application exit and the separate output flush.

| Drive / case | Normal wall | Ordered wall | Normal durable tail | Ordered durable tail |
| --- | ---: | ---: | ---: | ---: |
| C: sequential 4 MiB | 1,875.5 | 1,584.3 | 310.8 | 186.3 |
| C: sequential 64 MiB | 9,187.3 | 9,615.8 | 410.0 | 135.7 |
| C: split 64 MiB | 3,069.0 | 2,686.6 | 438.2 | 615.0 |
| D: sequential 4 MiB | 1,067.9 | 1,035.3 | 213.5 | 217.7 |
| D: sequential 64 MiB | 8,912.1 | 8,957.4 | 211.0 | 280.6 |
| D: split 64 MiB | 2,616.5 | 2,343.5 | 725.7 | 710.8 |

C: split exit-tail medians are 417.8974 ms normal versus 600.74945 ms ordered: **182.85205 ms longer** for the candidate. Its whole-life median is approximately 382.4 ms shorter, showing why ending measurement at network reception or selecting just one finishing phase would give an incomplete conclusion.

The [paired deltas](validation/ordered-comparison-8.62/paired-deltas.json) retain all eighteen pairs: seven ordered runs are slower in total wall time, including one split pair. The C: second split pair takes 2,686.6 ms ordered versus 2,075.6 ms normal. No slower pair or outlier is removed from the aggregate. The [terminal summary](validation/ordered-comparison-8.62/terminal.json), [all results](validation/ordered-comparison-8.62/results.json) and individual drive/case/trial/variant JSON records retain raw precision, full origin request/byte traces, catalogs and finishing diagnostics. `passed` means the comparison and correctness checks finished; it does not mean the promotion/performance gate passed.

## A remaining stall is not just the final output flush

The [first C: normal sequential 64 MiB case](validation/ordered-comparison-8.62/C-single-64m-1-normal.json) has a 7,220.427 ms exit tail. Its final published-file flush is only 228.964 ms. Existing diagnostics also record:

- 3,672 ms from application last-byte observation to worker join.
- Another 1,296 ms from worker join to the joined-stage timestamp.
- Three optional prefinish writeback flushes totaling 5,823.436 ms across the transfer.
- Publication-intent work totaling 1,396.062 ms, including roughly 907 ms at its temporary-file flush stage and 487 ms at publication.

These intervals have different scopes and can overlap; do not sum them as a decomposition of the tail. They establish that the final body flush alone does not explain the delay. They do not identify disk queueing versus a filesystem filter or scheduling as the cause. The mandatory durability/publication ordering remains intact.

The next implementation investigation should distinguish optional prefinish IO and its join from blocked receiver/writer work and publication-journal IO. Use the captured phases to target the blocking operation; do not replace it with more timing thresholds or declare a tail-only win. Any functional change needs the next paired native/browser increment and matched builds, followed by the affected correctness and full-lifetime comparisons.

## Cleanup and decision

[Cleanup](validation/ordered-comparison-8.62/cleanup.json) confirms that all **36 payloads / 1,660,944,384 logical bytes** were removed during the runs. After evidence capture, 110 remaining generated metadata files / 420,452 logical bytes and both owned comparison roots were removed with hash, hardlink and reparse checks. These are logical byte counts, not physical reclaimed-space or disk-wear measurements. No origin payload files or FFmpeg/source copies were created.

[Preservation](validation/ordered-comparison-8.62/preservation.json) verifies 524 qualified inputs, ten products, 43 distinct core objects, three protected installed/personal files and the new harness source. Native/browser source versions remain 8.62 / 0.72.26; installed UDM stays unchanged. No installer, deployment, push or release occurred.

Retain the private candidate and its reliability evidence, but leave normal builds synchronous. Universal speed parity, installed IDM comparisons and browser timing remain unproven. General IDM parity remains the active objective.

The subsequent [8.63 wait-attribution increment](join-waits-8.63.md) separates receiver, ordered, optional-flush and hasher waits without changing IO policy. Its 696 selected checks and four actual App diagnostic samples pass; the long stall did not recur. The prior no-regression promotion failure remains authoritative.
