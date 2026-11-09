# HTTP 403 on an extra range

An origin can reject an extra range with HTTP 403 while it continues to serve two earlier requests. UDM now returns an untouched split to the original response when that response still covers it. Once a fresh transfer sees this denial without a Retry-After instruction, it stops making speculative splits for that transfer. Uncovered ranges still use the existing reassignment and retry paths.

The loopback fixture requested eight connections for a 32 MiB file and rejected the first extra range after the original and midpoint requests. All three builds published the same expected SHA-256.

| Build | Requests including the 403 | Transfer checks passed |
|---|---:|---:|
| 5.75 control | 11 | 70 of 72 |
| 5.76 with range restoration and a cooldown | 6 | 71 of 72 |
| 5.77 with speculative splitting disabled after denial | 3 | 73 of 73 |

5.75 missed the refused-range request bound and the forced-reset timing bound while the production compiler was active. 5.76 also missed the request bound because it could split again once the cooldown expired. The 5.77 run covered both original responses through completion, and its added recovery case verified exact bytes after the origin rejected expansion and subsequently reset accepted connections. The suite also passed the existing TCP cap, outage, sparse resume and overlap checks.

The three local logs are under `udm-http-403-5.76` in the Windows local temporary folder. These are controlled native runs. Healthy-origin request counts against IDM and public-server timing remain open. [5.76 build receipt](http-range-403-5.76-build.json) · [5.77 build receipt](http-range-denial-5.77-build.json)
