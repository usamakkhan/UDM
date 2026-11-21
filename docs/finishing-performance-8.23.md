# Adaptive finishing overlap: 8.23

The installed 8.22 comparison still failed sequential and split 64 MiB full-process medians. Preserve [its complete results](validation/finishing-8.22/comparison.json), including all outliers. The [measurement protocol](finishing-performance-8.21.md) runs through actual application exit, including background work and automatic shutdown; it is not GUI dialog timing.

## Sequential writeback runway

The fixed near-end flush left only 2 MiB (roughly 250 ms at the owned origin rate) before EOF. Measured flushes sometimes take longer: 8.22's 64 MiB sequential cases waited 469 and 828 ms after the last byte for outstanding work. A first-flush duration of at least 50 ms now permits moving the second threshold earlier. Calculate runway from credited bytes / elapsed transfer time, twice the measured first-flush cost, plus the existing minimum lead. Cap runway at half-body. The threshold can only move earlier, never before half-body. Keep at most two operations and the required final flush. Record adjustment count and scheduled lead bytes.

A held-worker regression test reproduces a costly first flush, releases it and supplies progress below the old final-2-MiB point. The second worker must actually execute, remain bounded to half-body and exactly two calls, and admit no operation after finish. Normal milestone, coalescing, nonblocking progress, failure and real FlushFileBuffers checks remain. Variable later disk stalls cannot be predicted perfectly; installed measurements must establish the actual outcome.

## Advisory hash overlap

For a completed same-file hard-link publication with no required checksum gate, start optional SHA-256 catch-up before publication. All body writers and the previous trailing hash thread have joined. This task alone accesses the hasher; the main worker does not access it until the future joins. Its read handle permits rename and final writeback. Required server/user/replacement hashes remain before publication. Cross-volume copies keep the existing serial hash of the published copy.

Use an owned child Cancel linked to the parent download. Declare a scope guard after the future so unwind signals cancellation before the future joins; the source/staging cleanup and hasher outlive both. Publication failure cannot leave detached work or stop the parent download for a different reason. Thread/allocation failure retains the old serial path. The worker waits for the future before deleting parts, recording final SHA-256, final checkpoint, Finished event or exit. Full hash duration and remaining foreground wait are recorded separately, so overlap does not hide work from the benchmark.

## Validation and limits

Installed and verified 8.23 after all 1,778 native checks passed. The [installed comparison](validation/finishing-8.23/comparison.json) retained all eighteen successful output/final-record hashes and all timing outliers. Median UDM/IDM finishing tails: 189.162/401.268 ms for 4 MiB; 169.373/357.317 ms for sequential 64 MiB; 431.701/413.071 ms for split 64 MiB. Full elapsed medians were lower for UDM in all three cases, but split tail and individual trials remained slower. The performance target was not met. Its eighteen payloads were later individually hash-checked and removed in the [8.24 storage cleanup](validation/finishing-8.24/storage-cleanup.json).

Desktop 8.23; Chromium and Firefox manifests 0.71.87. Browser runtime JavaScript is unchanged. Build, native checks, installation, real split overlap execution, final saved SHA-256 and full-exit comparison are separate gates. Existing transaction/ownership/recovery and gzip/prefetch checks remain required. Exhaustive injected failures, power-loss, slow/network/removable volume and cancellation interleavings are not established by ordinary healthy comparison runs. Desktop-control initialization still prevents fresh live IDM dialog timing. No universal zero-delay claim, setup package or release.
