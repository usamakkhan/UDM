# Remaining publication delay: common-clock attribution of 9.60

This is diagnostic evidence, not a new speed fix. Production sources and the qualified 9.60 products are unchanged. The experiment that suppressed optional writeback is rejected. The first-availability and previous-build no-regression gates in the [9.60 comparison](publication-availability-9.60.md) remain open.

## What was measured

Eight actual App transfers use isolated catalogs and deterministic loopback data: three broad timelines (4 MiB sequential, 64 MiB sequential and 64 MiB split), one more detailed split timeline, then four split controls in normal/suppressed/suppressed/normal order. All eight outputs match their independent expected size and SHA-256; the required final output writeback succeeds in every case. The payloads are removed after verification.

The native diagnostic records receiver joins, optional writeback retirement, prefix-hasher retirement, metadata, final output writeback, intent, rename, catalog save, accepted-file pin release and shutdown on QueryPerformanceCounter. The parent origin clock is mapped to that counter with an explicit error bound. Ready flags, process identity, frequency, capacity and zero dropped events are checked. The first three diagnostic sources are instrumented private copies; later epochs retain the same App entry and change only their instrumented Transfer unit. Production code is not edited by those private copies.

The detailed epochs also time slow file writes, credit operations, progress ownership, response reads and optional flush calls. A slow operation uses the trace record's counter/finished fields as its operation interval; ordinary markers use those fields for marker emission. They must not be interpreted interchangeably. Concurrent worker intervals overlap and must not be summed as independent wall-clock delay.

These binaries contain extra instrumentation and a mapped trace file. Their timings are for attribution, not uninstrumented performance qualification. The new controls do not compare with IDM or an installed update. Global disk queueing, filesystem/filter work, zero-fill/allocation and scheduling are not distinguished by this user-mode trace.

## First timeline batch

The following portions occur after the last body send recorded by the origin. Receiver join covers the entire reception in the source, so only its overlap after that send is shown. This is not proof that all bytes had reached the client at that instant. All request attempts, including range overfetch, remain in the origin record.

| Region after last origin body send | 4 MiB sequential | 64 MiB sequential | 64 MiB split |
| --- | ---: | ---: | ---: |
| Receiver join | 2.227 ms | 2.217 ms | 135.179 ms |
| Optional writeback retirement | 0.243 ms | 0.226 ms | 0.194 ms |
| Prefix hasher retirement | 0.001 ms | 0.001 ms | 0.000 ms |
| Metadata | 1.528 ms | 2.032 ms | 1.474 ms |
| Required output writeback region | 56.516 ms | 56.930 ms | 153.893 ms |
| Durable intent region | 27.133 ms | 18.863 ms | 30.809 ms |
| Rename and name pin | 1.017 ms | 1.395 ms | 0.869 ms |
| Catalog save region | 33.228 ms | 33.113 ms | 35.407 ms |
| Accepted-file pin release | 0.062 ms | 0.031 ms | 0.042 ms |

The sequential publication path contains three substantial serial durability regions: final output writeback, durable intent, and catalog save. Metadata and file-handle release are small in these samples. The already-fixed post-acceptance pin hold is not the remaining roughly 100 ms publication cost.

A split sample spends 135 ms joining receivers after the last server send, but that long tail does not repeat consistently: the detailed repeat records about 16 ms, and later controls range from roughly 2 to 86 ms. The detailed trace observes file writes over 200 ms overlapping optional whole-file data flushes. That overlap is evidence to investigate, not proof that the optional flush caused each wait.

Normal shared readers can open the renamed file before the catalog checkpoint and exclusive-reader release. Path visibility, shared-reader availability, exclusive-reader availability, completion presentation and process exit remain separate metrics. The original exclusive-open benchmark has not been relaxed or replaced.

## Controlled optional-flush suppression

Both variants use one hash-pinned diagnostic binary. The private UDM_TEST_NO_OPTIONAL_DATA_FLUSH environment switch is captured by value when constructing the optional callback. It exists only in the diagnostic source copy. In the suppressed variant, those callbacks return without making NtFlushBuffersFileEx calls. Required final FlushFileBuffers, publication, catalog save, worker lifetime, post-exit durability and hash checks still run.

Normal runs record seven real optional calls each; suppressed runs record zero real optional calls and seven suppressed callbacks each. The stock PrefinishWriteback statistics count callbacks in that private experiment and must not be presented as physical flush counts.

| ABBA sample | Full process elapsed | After last send through exit | Required final flush | Longest observed file write |
| --- | ---: | ---: | ---: | ---: |
| split-control-a | 2518.6 ms | 235.9 ms | 102.7 ms | 261.0 ms |
| split-skip-b | 2569.1 ms | 788.4 ms | 626.7 ms | 259.9 ms |
| split-skip-c | 2532.2 ms | 725.9 ms | 613.9 ms | 291.3 ms |
| split-control-d | 2296.5 ms | 298.0 ms | 100.1 ms | 263.2 ms |

Within this small controlled batch, median full elapsed time increases from 2407.5 to 2550.7 ms when optional calls are suppressed. Median finishing tail increases from 266.9 to 757.2 ms. Median required final flush rises from 101.4 to 620.3 ms. The experiment fails both finishing and total-time criteria and is not promoted.

Long writes around 260–291 ms persist in the suppressed runs. Optional writeback therefore does not solely explain the observed write stalls. The experiments support keeping the existing optional drainage policy while the write path and publication ordering are investigated. Two samples per variant and one machine do not establish a universal performance law or exclude other concurrent system I/O.

[Every ABBA observation and derived median](validation/publication-critical-path-9.60/ablation-summary.json), [scope and verification status](validation/publication-critical-path-9.60/status.json).

## Next implementation and admission plan

1. **Keep the measured endpoints.** Record path visibility, normal shared-read access, exclusive access, actual GUI completion, process exit and post-exit output durability separately. Normal speed qualification must use the ordinary products with compilation stopped, exact executable hashes, rotating prior/current/IDM order and all samples retained. A 2 ms polling observer cannot prove a universal 1 ms bound.
2. **Locate the write stalls more precisely.** writeAt in Transfer.cpp is a synchronous positioned WriteFile. The current trace times that call separately from disk-credit claim/acknowledgement and model progress work. Add write offset/count and file creation/allocation context before attributing waits to zero-fill, concurrent ranges or filters. Do not enable privileged valid-data-length operations or change Windows security settings to skip allocation work.
3. **Requalify the existing ordered writer as a candidate, not an assumption.** OrderedBodyWriter already offers bounded accepted buffers, ascending writes, joined draining, pause ownership and failure propagation behind UDM_ORDERED_BODY_QUALIFICATION. Its earlier 8.62 whole-lifetime promotion failed. A current comparison must preserve real byte credits, partial recovery, memory bounds, hash requirements and every endpoint above; passing component tests alone cannot enable it by default.
4. **Separate preparable intent work from rename acceptance.** Investigate overlapping intent I/O with final body writeback using owned immutable input snapshots and an owned, joined operation. The existing PublicationPreparation read pin excludes write access, so merely starting FlushFileBuffers on another write handle would conflict with it. A metadata-only pin is not an equivalent protection. Any interval allowing writes before the final strict pin needs a reliable final identity and full-content verification before publication. Its extra read/hash cost must be measured, not hidden.
5. **Define interruption/recovery before enabling early intent.** An intent written before body writeback has completed must never be mistaken for an already-durable completed file. The retired FlushBeforePublish recovery path is not an invitation to restore the old producer unchanged. Bind the exact generation, staging object, destination inputs, required digest, successful body writeback and cancellation outcome. Recover with a real body barrier and content validation. Retain ambiguous bytes and journal state without overwriting a changed or foreign file.
6. **Keep final acceptance explicit.** A separate durable receipt/WAL acceptance design could move catalog indexing off the file-availability path, but it is not implemented or approved by this evidence. It must prove exact durable completion and confirmed rename, preserve current settings and download metadata, distinguish accepted-but-unindexed output from failure, and handle user edits after acceptance without deleting or silently rewriting their bytes. Do not treat mere path existence or a boolean in an unverified journal as acceptance.
7. **Test every new boundary.** Required controls include cancellation and shutdown while either worker is held, forced body/intent/index flush failure, source/name replacement, changed folder/project root, duplicate/replacement publication, internal tracks, copied/across-volume output, required checksum mismatch, unknown size, compressed output and fresh-process recovery before/after each durable boundary. Hardware/network/removable behavior still needs matching live evidence. All accepted work must remain owned and joined; no detached helper or borrowed window may outlive its owner.
8. **Promotion must retain failed gates.** Build all native products and advance both source versions when a real app improvement is made. Compare with the previous qualified product and IDM across the three established cases, including all background work. If first availability or total elapsed regresses, retain the observation and continue the investigation. This attribution round makes no new speed-fix or installation claim.

Publication preparation/acceptance work remains the priority ahead of toolbar discovery and smaller settings/UI optimizations. Full inside/outside IDM parity, installed/browser checks, visual timing and clean-checkout GUI build profiles remain open.

## Evidence and cleanup

Lossless trace records are retained as header hex, used-record hex and zero-tail length with the raw trace SHA-256. The original binary trace can be reconstructed byte for byte without retaining an opaque executable or the temporary fixture. Input hashes, private-source reconstruction patches, defines, compiler logs, normalized operation spans and origin records are included. Marker insertion/setup failures that occurred before compilation produced no transfer and are explicitly excluded from the eight retained runs.

All eight verified payloads (452 MiB logical) are removed. Cleanup also removed 131 generated files: 186.3 MiB of unique file content (284.7 MiB when hardlink aliases are counted separately). This is not a physical disk-space or wear measurement. Current and previous native products, required core objects, both personal UDM catalogs, installed files and shared ffmpeg/ffprobe remain unchanged. No media-helper copies, installer, installation, commit, push or release were created. Production remains 9.60 / extension source 0.73.24; installed UDM remains 8.68.
