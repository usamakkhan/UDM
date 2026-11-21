# Ordered HTTP response qualification, 8.62

The private ordered-body candidate passes **24 actual Manager cases and 126 native assertions**, twelve policies on each of C: and D:. Every completed file is independently verified as exactly 8,388,608 bytes with the expected SHA-256. This is test-only qualification against the unchanged 8.62 core, separate from the earlier 1,780-check build and fourteen-case storage-fault suite. Normal builds still use synchronous writes.

## Response ownership and fallback

The [controller](../benchmarks/speed/ordered_response_control.cpp) drives the real Manager queue, transfer, publication and catalog pipeline. The [origin](../benchmarks/speed/response_policy_origin.py) synthesizes immutable versioned bytes without making server payload files. The primary connection is slower than helper connections so admission and reassignment actually occur. Each case uses an isolated catalog and owned destination.

| Policy | HTTP requests on each drive | Verified behavior |
| --- | ---: | --- |
| Valid ranges | 4 | Joined ordered writes, exact complete version 1 file |
| Explicit Accept-Ranges: none | 1 | Original full response completes through synchronous fallback |
| Range header omitted, Range ignored | 2 | Rejected helper does not discard the original full response |
| Range support advertised, Range ignored | 2 | Original full response completes; ordered writer remains valid for that body |
| One-use URL, ranges advertised | 2 | Helper receives 403; original response still completes |
| One-use URL, range header omitted | 2 | Helper receives 403; synchronous original response still completes |
| No validator, one-use URL | 1 | No helper request; exact original full body |
| Server capacity one | 1 plus one rejected TCP connection | Helper connection refusal preserves the original body |
| Wrong helper Content-Range start | 2 | Malformed helper rejected; original bytes retained |
| Wrong helper ETag and body | 2 | Hostile version 2 helper cannot contaminate version 1 output |
| Truncated helper | 5 | Declared body ends early; retries complete exact output |
| Representation change with held old save | 3 | Old checkpoint blocks reset; replacement full response reused without another GET |

All observed requests are GETs; there are no HEAD probes. The fallback/one-use oracles require exactly one original request without Range and a fully served original body. This proves reuse for these cases, rather than inferring it from final success. It does not mean every case transfers only one file's worth of bytes. Ignored helper responses serve discarded data, retries overlap, and representation changes receive old bytes before the new full response. The [request and byte summary](validation/ordered-responses-8.62/results.json) retains those costs, including the small C:/D: difference in truncated-response bytes.

An advertised range capability can leave the ordered writer active even when a helper is rejected. A missing capability can select synchronous fallback. The assertions distinguish those paths; helper rejection is not claimed to clear all capability diagnostics.

## Held old-generation checkpoint

The representation-change case detaches an actual Manager save and holds its `snapshot-prepared` callback outside the model lock. The server waits for that hold before returning version 2. After old ordered reception has paused, the fixture verifies that the save is still pending, reads the old parts prefix, then checks that the old ETag and parts remain intact and no completed destination or completion acknowledgment exists. It releases the save and joins it before the replacement finishes.

The orchestrator independently compares the held prefix with version 1 and the complete final SHA-256 with version 2. There are exactly three HTTP requests: the original version 1 body, its helper, and a stale-If-Range request returning the new full version 2 body. The transfer retains that unread replacement response for the next generation instead of opening another full GET. The replacement generation uses the synchronous path and does not retain stale ordered diagnostics.

Returning a full representation when an If-Range condition fails, and allowing a server to ignore Range, follow [RFC 9110 sections 13.1.5 and 14.2](https://www.rfc-editor.org/rfc/rfc9110.html). The wrong-ETag helper deliberately violates the former rule to exercise rejection separately.

This closes the tested representation-change/reset race. It does not prove folder changes, refresh-address commands, removal, direct shutdown, child/media bindings or every checkpoint-failure ordering.

## Evidence, interruption and cleanup

The [build receipt](validation/ordered-responses-8.62/build-result.json) pins both fixture sources and the unchanged private core. Ordinary cases have five native assertions; each held-change case has eight. The orchestrator additionally verifies actual file length/hash, catalog/native digests, response policies, original-body ownership and a separate post-exit output flush. Individual `C-*.json` and `D-*.json` records retain sanitized catalog and origin details.

An earlier orchestration process disappeared after nine verified C: cases. Process absence was checked; its incomplete tenth root is recorded as [interrupted](validation/ordered-responses-8.62/interrupted-case.json), not counted as either a pass or a backend failure. The nine completed cases were revalidated before continuing the remaining fifteen. No successful case is counted twice.

[Cleanup](validation/ordered-responses-8.62/cleanup.json) removed **128 generated files / 219,930,803 logical bytes** in 25 owned fixture roots, including the interrupted root and the controller executable/object. Per-file hashing, hardlink-group checks, reparse refusal and resolved root boundaries scoped deletion. Logical bytes do not establish physical reclaimed allocation. Fixture sources and sanitized evidence remain.

[Preservation](validation/ordered-responses-8.62/preservation.json) verifies 524 qualified inputs, both new fixture sources, ten products, 43 distinct core objects and three protected installed/personal files. Native/browser source versions remain **8.62 / 0.72.26**. Installed UDM remains 8.32; no installer, installation, extension deployment, administrator prompt, push or release was involved.

## Next gates

1. Exercise refresh-address, folder changes, removal and direct shutdown with queued data and held checkpoints, including rollback failures. Keep file retirement and catalog ownership explicit.
2. Compare balanced whole-lifetime sequential and split runs on both drives, including process exit, final background work and a separate output flush. Retain all slower results and request/byte costs. These correctness fixtures are not performance comparisons.
3. Resume installed UDM/IDM and browser comparisons when deployment is authorized. Only after the promotion gates pass should a new versioned normal build enable the selected path. General IDM parity remains unproven.

The subsequent [direct-shutdown qualification](ordered-shutdown-8.62.md) closes the scoped held-save / Manager-stop / fresh-process-resume case with 20 further assertions. Other shutdown failure orderings remain open; it is separate from the 24 response cases above.

The subsequent [operation lifecycle qualification](ordered-lifecycle-8.62.md) closes the scoped active/retiring folder, refresh-address and removal command boundaries and post-retirement save rollback. Broader failure interleavings and performance comparisons remain separate.
