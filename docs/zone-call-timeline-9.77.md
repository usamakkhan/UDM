# UDM 9.77: distinguish metadata calls from the broad finishing interval

## Finding

Do not attribute the earlier 157.828 ms `PrePublicationMetadata` interval to the Internet-zone stream. In the current source, that timer runs from the preceding hash phase through post-hash checks, optional audio tags, staging, finalization eligibility checks, advisory-hash launch, the saved phase snapshot and metadata handling. Its name describes a broader interval than one metadata operation. The eight-connection capped observation established a slow interval, not a slow zone syscall.

A private diagnostic of the current 9.77 source now measures the zone open, write and close separately, plus the phase-snapshot critical section and the actual metadata block. Two owned downloads completed with exact independent hashes, the expected complete Internet-zone bytes, normal process exit and explicit post-exit output flush. The enabled server-date case also verifies the final timestamp.

| Diagnostic observation | Capped 32 MiB, eight requested connections | Sequential 4 MiB, server date enabled |
| --- | ---: | ---: |
| Zone stream open | 0.208 ms | 0.256 ms |
| Zone write | 0.083 ms | 0.076 ms |
| Zone close | 1.354 ms | 1.263 ms |
| Whole zone function | 1.647 ms | 1.598 ms |
| Actual metadata block, including optional date | 1.668 ms | 2.027 ms |
| Phase snapshot critical section | 0.013 ms | 0.015 ms |
| Existing broad `PrePublicationMetadata` field | 5.003 ms | 5.270 ms |
| Recorded body flush call | 151.174 ms | 68.123 ms |
| Durable intent interval | 113.544 ms | 69.441 ms |
| Final catalog save interval | 38.976 ms | 30.953 ms |
| Last origin byte to actual process exit | 282.109 ms | 187.045 ms |
| Explicit post-exit output flush call | 7.283 ms | 27.140 ms |

These are instrumented operation observations, **not** a normal-product speed comparison, a reproduction of the earlier slow metadata interval, or an IDM result. Durations from concurrent body writeback, hashing and intent work overlap and must not be summed as serial cost. The date-enabled metadata residual is not an isolated timestamp syscall measurement.

## Remaining waits and next action

The final durable intent and body writeback dominate these observations. In the capped case, the worker still waits about 37 ms for writeback after its intent interval ends; the final catalog transaction follows. The mapped native events show exactly one final intent interval and one final catalog interval in each case, alongside earlier periodic catalog flushes during reception.

After the explicit window cleanup marker, the current `CFrameWnd::OnClose()` interval takes approximately 40.431 and 34.777 ms. That is framework/window destruction time in these observations; it does not establish an intentional sleep or make it safe to skip destruction. Normal worker shutdown and the remaining process exit work remain included in the exit endpoint.

Next isolate finalization eligibility/volume checks and advisory task startup within the broad interval, and correlate durable intent/body/catalog blocking with storage activity. Do not repeat the retired early-zone or disabled-drainage policies, remove durability barriers, or special-case the benchmark's background launch to conceal the remaining work. The earlier normal-product losing result remains authoritative and open.

The read-only volume query reports D: as healthy NTFS with 131,072-byte allocation units. A partition lookup did not establish its physical device, and `fltmc filters` was denied access. This does not identify a disk, filter or scheduling cause; no administrator request was issued.

## Isolation and evidence

Only private copies of Core and Transfer were edited. Three core units and the unmodified App entry were compiled with existing finishing-trace definitions; the remaining 39 normal core objects and normal resource were retained. No production source, behavior, version, installation or installer changed. Source versions remain UDM 9.77 and both manifests 0.73.41.

The initial private build driver had malformed nested backslash quoting. Python rejected it before directory creation, compilation or application launch. Its source and the correction are retained. This setup failure is not a discarded slow download.

Evidence: [both complete records and lossless traces](validation/zone-call-timeline-9.77/runs.json), [private Core delta](validation/zone-call-timeline-9.77/Core.cpp.patch), [private Transfer delta](validation/zone-call-timeline-9.77/Transfer.cpp.patch), [build inputs](validation/zone-call-timeline-9.77/inputs.json), [driver](validation/zone-call-timeline-9.77/run-zone-call-timeline-9.77.py), [terminal result](validation/zone-call-timeline-9.77/terminal.json), [final audit](validation/zone-call-timeline-9.77/final-audit.json), [export hashes](validation/zone-call-timeline-9.77/export-manifest.json).

Both raw trace rings have zero dropped records and were exported losslessly before deletion. The 32 MiB and 4 MiB payloads were individually hash-checked and removed. Twelve disposable diagnostic files were then individually path/type/hash checked and deleted, totaling 153,538,837 logical bytes; this includes multiple hard links to one diagnostic executable, so it is **not** a measurement of physical space reclaimed. Current five products, all 42 referenced core objects, all 694 captured inputs and eleven protected installed/catalog/helper files remain unchanged. No owned diagnostic app process remains. Compact private diagnostic catalogs and sanitized evidence remain.
