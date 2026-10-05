# Expired-link recovery and stale metadata

A real transfer test exposed stale redirect metadata after saving a fresh address: the old server URL remained attached to the record until another transfer overwrote it. The actual refreshed download otherwise resumed correctly.

Address refresh now clears the previous redirected server URL, authentication origin/scheme and pending old login prompt. This occurs inside the existing rollback-capable save transaction. Saved parts, segment plans, validators, filename and history identity remain intact. Failed persistence restores the full previous record and catalog; retry and restart are covered.

The actual GUI test downloads part of an 8 MiB file through a redirect, expires the old link with HTTP 403, saves a new address through Refresh download address, and resumes using ranged requests. The final output matches the known source bytes and independently verified SHA-256.

Fresh checks: 14 metadata/rollback, 11 actual refresh-dialog transfer, 27 desktop, 7 cold-start, 47 setup-helper, 10 migrated-app, 15 monitor protocol and 9 native-host framing checks: 140 total. Monitor diagnostics used the existing signed runtime and did not load a driver. Early fixture invocation/runtime omissions are retained separately from the passing reports.

All four native components were rebuilt consistently and packaged. Four of 132 payload files changed; other inputs remain identical. The tested Core object is now the candidate build cache's Core.obj, with its predecessor preserved. Installed executables and both personal catalogs remain byte-identical. The candidate was not installed or published.

This controlled HTTP recovery test does not establish every public provider's signed-link behavior, browser recapture, speed parity or full IDM parity. Previous full-suite results remain historical; this turn ran focused engine and component regressions. Elevated lifecycle, multi-user behavior, manifest concurrency and intermittent live-HLS timing remain open.

[Acceptance, exact hashes and test counts](D:/UDM-Workspace/candidates/release-084-055/control/refresh-metadata-acceptance.json)
