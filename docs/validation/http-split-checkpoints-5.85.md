# HTTP range split checkpoints

A live split divides only bytes beyond the committed and in-flight writes in the shared staging file. The helper can now open its request immediately after the split is assigned. The existing periodic checkpoint records the updated ranges and progress together. Pause still saves the live layout. A refused helper can also return an untouched tail to its donor without flushing the catalog first.

An abrupt exit between checkpoints can require re-requesting bytes beyond the last saved offsets. The older catalog still describes valid ranges and keeps its saved bytes. The recovery test used the earlier catalog with the newer staging file and reproduced the exact original SHA-256. A separate reload used the saved pause layout and also reproduced the original bytes.

All 79 native HTTP transfer checks passed, including the new recovery checks, connection refusal, HTTP 403, and checkpoint failure during a range reset. All 12 downloads in the paired loopback comparison matched the expected SHA-256.

| File | Median first to sixth accepted request, 5.84 / 5.85 | Median requests, 5.84 / 5.85 | Median wall time, 5.84 / 5.85 |
|---|---:|---:|---:|
| 24 MiB | 468 / 335 ms | 9 / 11 | 2.213 / 2.269 s |
| 192 MiB | 327 / 477 ms | 8 / 8 | 4.100 / 5.207 s |

Each file size ran three alternating trials per build. The longest first-to-sixth interval at 192 MiB was 2,618 ms on 5.84 and 506 ms on 5.85. The wire timings were mixed, and neither size had a faster median total time in this sample. The comparison used two UDM builds. Direct IDM timing remains open.

The fixture log and request traces are in the Windows local temporary folder under `udm-http-split-checkpoints-5.85`.

[Production build receipt](http-split-checkpoints-5.85-build.json)
