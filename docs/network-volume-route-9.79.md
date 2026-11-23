# UDM 9.79: preserve network fallback without an extra file open

UDM 9.79 keeps UNC shares, extended UNC names and mapped remote drives on the original volume lookup path. They no longer receive the preliminary attribute open introduced by 9.78's opened-volume optimization. Ordinary and extended local drive paths, plus local volume-GUID paths, retain the guarded fast route. Both extension source manifests advance to 0.73.43; JavaScript behavior is unchanged. Five native products build as 9.79. The candidate remains **uninstalled**, and the overall finishing-speed gate remains open.

## Implementation and ownership

[FixedVolume.hpp](../native/FixedVolume.hpp) now checks the path form and, for a drive-letter route, its drive type before attempting the opened-file query. A remote mapping does not directly decide the result: it selects the original path-resolution fallback, whose classification still governs the caller. No drive-letter classification is cached. Unsupported device/namespace forms also retain fallback. The existing guarded GUID validation, handle closure and known non-fixed classifications remain.

The [new route tests](../native/FixedVolumeTests.cpp) assert zero attribute opens and zero final-name queries for ordinary/extended/forward-slash UNC inputs and mapped remote drive forms. They also check that a mapped-route guard preserves the fallback's actual result and that extended local/GUID names keep the fast path. No real remote endpoint is contacted by these synthetic cases.

Receive-side handle reuse was reviewed separately. The ordinary HTTP path finishes and closes `prefinishFile` before returning to the outer transfer's content/hash/staging/publication work. That handle is therefore unavailable at final eligibility. Carrying a writer across this boundary would require an explicit owned handoff and retirement before restrictive publication ownership; it is not safe to replace that work with a cached boolean or a borrowed raw handle. This increment leaves those lifetimes and all durable barriers unchanged. It does not claim to resolve the larger body/intent/catalog waits.

The prior 9.78 implementation attempted an open before its network fallback. This correction removes that added operation for recognized share routes; it is **not a measured live-SMB throughput improvement**. Live SMB, reconnect, mapped-route changes and removable-media timing remain unqualified. See the [isolated change from qualified 9.78](validation/network-volume-route-9.79/build/route.patch); its old header and fixture bytes match their qualified hashes.

## Qualification

All **1,559 assertions in 23 affected suites** pass on D:, including 27 volume checks. The same 27 checks pass on C:. An earlier 27-check D: preflight is retained separately rather than added to the suite count.

A compiled read-only cost control uses existing files on both drives, ten alternating-order comparisons each. All twenty classifications agree with the legacy implementation, use one closed attribute handle and avoid fallback on these local files. Median legacy/current API times are 0.380/0.080 ms on C: and 0.891/0.115 ms on D:. This is a warm API check, not an application-speed or IDM ranking.

Two uninstrumented normal-App 4 MiB downloads, one with an owned C: catalog/destination and one on D:, complete through the actual worker and normal process exit. Each uses exactly one successful request, serves exactly 4,194,304 bytes, matches the independent SHA-256 and final Complete catalog, preserves the exact Internet-zone marker, and passes an explicit post-exit output flush. Both payloads are hash-checked and removed. These are local correctness smokes; they do not repeat the 64 MiB performance matrix or supersede any losing 9.78 sample.

## Current blockers and next work

A fresh supported Windows-control session still fails with “failed to write kernel assets” (OS error 3). IDM's observed current-session process is a COM-launched `-Embedding` instance with a service-host parent. A scoped scan of developer-area command lines found the active build controller and no older diagnostic client. This does not establish which application is keeping IDM alive. It was not terminated. The earlier bounded elevated diagnostic worker has a matching terminal record after three requests and is not an active reusable session; no replacement or UAC prompt was launched.

The [9.78 full-lifetime losses](fixed-volume-9.78.md) remain open. Next attribute the remaining intent/body/catalog waits and framework destruction using valid current observations, and qualify real remote destinations before treating this routing correction as complete network performance acceptance. No installed/browser/UI or complete inside/outside parity claim follows.

## Evidence and cleanup

[Build](validation/network-volume-route-9.79/build/build-result.json), [suite results](validation/network-volume-route-9.79/build/suite-results.json), [C: route checks](validation/network-volume-route-9.79/suites/FixedVolumeTests-C.json), [read-only costs](validation/network-volume-route-9.79/cost/results.json), [normal C:/D: smokes](validation/network-volume-route-9.79/smoke/terminal.json), [final audit](validation/network-volume-route-9.79/final-audit.json), [export hashes](validation/network-volume-route-9.79/export-manifest.json).

Transfer and Bridge are rebuilt; forty core objects are retained. All 696 current inputs are captured before compilation and verified afterward. The existing registered fixture runs with the new cases; no post-compilation registry amendment is required in this increment. The current five programs, resource and 42 referenced core objects remain.

After sanitized evidence export, 908 disposable test/build/catalog files are individually path/type/hash checked and removed, totaling 302,017,670 logical bytes. This also retires four superseded 9.78 helper programs and two superseded core objects; their frozen App remains for comparison. [Intentional retirement record](validation/network-volume-route-9.79/retired-9.78-artifacts.json). Private fixture catalogs are removed; empty owned directories can remain. Physical space reclaimed was not measured. Installed binaries/manifests, personal catalogs and shared media helpers retain their protected hashes. No installer, installation, commit, push or release occurred.
