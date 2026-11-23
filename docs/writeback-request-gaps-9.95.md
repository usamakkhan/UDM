# Split writeback request gaps — unchanged 9.95 products

Replaying the existing qualified 8.65 phase reports identifies a substantial difference between the split and sequential flush observations. No new download, trace, elevation, lab start, installation or product change is involved. These are historical instrumented samples, not current 9.95 performance scores or an IDM comparison.

| Existing D: 64 MiB observation | Native final flush | Retained linked request union | No retained linked request | Observed off-CPU overlap within those gaps | Largest gap |
| --- | ---: | ---: | ---: | ---: | ---: |
| Eight connections | 927.2062 ms | 539.5862 ms | 387.6200 ms | 376.7150 ms | 109.4422 ms |
| Sequential | 83.8555 ms | 82.6586 ms | 1.1969 ms | 0.1855 ms | 0.1821 ms |

The split trace has 58 gaps. Its three largest are 109.4422, 59.8126 and 55.8779 ms, with observed off-CPU overlaps of 109.2319, 59.6072 and 55.6579 ms respectively. The largest starts 448.4105 ms after native flush entry. This localizes the unexplained portion to several waits between the retained requests, rather than one uninterrupted device-flush command. The recorded final disk-flush command union remains 21.2255 ms.

“No retained linked request” does **not** mean an idle disk, an application sleep, or a proven filter-driver delay. The source join retains only requests attributed to the measured issuing thread and wholly inside its native call. System-worker activity, crossing requests, unlinked operations, filesystem/filter work and other device traffic are outside this partition. The off-CPU model retains its separately documented version-5 prefix limitation. Neither full-schema qualification nor complete kernel causality is established.

## Concrete next investigation

1. Retain these specific gap boundaries as diagnostic targets. A whole-flush average obscures the split case's intermittent waits; the sequential sample presents a different observed pattern.
2. Prepare bounded attribution of file-role and system-worker requests before collecting another slow sample. Establish path/object lifetime and request-init/completion ownership without attributing every request on disk 1 to UDM. Preserve explicit loss, unsupported-schema, cap and incomplete-lifetime failures.
3. Do not guess the undocumented FileIo opcode-84/version-3 layout. The current host WMI inventory still lacks that event, and Microsoft's public parser lookup does not establish its meaning. The earlier self-process focused probe is separate from the rejected fresh normal-process full-FileIo captures.
4. Correlate retained request gaps with positive filesystem/filter/worker evidence. Until that evidence exists, keep queueing, system-worker writeback and filesystem/filter/scheduling explanations as alternatives; no new pacing or flush-suppression experiment is justified by this partition.
5. Judge a resulting production fix on normal-product availability, total process lifetime and symmetric post-exit flushing, repeated on both volumes. Keep existing rejected storage experiments and losing samples. UDM's durable body/publication/catalog barriers remain in force.

## Reproducibility and scope

`benchmarks/speed/analyze_request_gaps.py` partitions the already qualified public reports using decimal milliseconds. It merges overlapping/touching requests, accounts for leading/trailing gaps, intersects gaps with observed off-CPU intervals, and checks both unions against the source totals. It does not reconstruct raw QPC records, rehash deleted payloads or promote scoped analysis to full coverage. Eight read-only tests cover overlapping requests, exact partitioning, empty/full coverage, touching/zero-duration requests, invalid bounds and numbers, inconsistent totals, overlapping off-CPU records and rejected capture states.

[Source split/sequential phase reports and their original provenance](app-finishing-phase-analysis-8.65.md) · [New hashed replay results](validation/writeback-request-gaps-9.95/) · [Current IDM writeback-policy evidence](idm-writeback-contract-9.95.md).

Native 9.95 and both source extensions 0.73.59 are unchanged. No product improvement or version increment is claimed for research tooling. Installed-product, browser activation and parity acceptance remain open. No temporary test payloads were created or deleted by this work.
