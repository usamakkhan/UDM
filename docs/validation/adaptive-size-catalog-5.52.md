# Recorded media size and catalog writes in UDM 5.52

File Info saves the pending confirmation and its working folder before starting prefetch. Each HLS or DASH part has a durable recovery receipt. UDM 5.52 uses that state to begin assembly while confirmation is pending. The Combining streams phase still updates in memory. Start, retained output and final publication still require their existing durable commits. Transfers that are already confirmed keep the assembly-phase save. Refresh validation also keeps its separate checkpoint.

The control links the current fault fixture against frozen 5.51 core objects. It locks the catalog scratch file after prefetch admission and before the worker starts. Both part receipts remain readable, but File Info never reaches exact size. The suite passes 48 checks and fails three assertions that depend on size and digest verification while the catalog is locked. The separate locked aggregate-index case passes under 5.51.

The candidate fixture checks that the same catalog lock permits exact-size proof, that attempting Start while the lock is held preserves the confirmation gate, and that reopening from the saved receipts reproduces the exact file without requesting the segments again.

All 52 adaptive refresh checks, 274 adaptive prefetch and recovery checks, and 163 native presentation checks passed. The new lock case confirmed that the target remains absent after Start fails, the verified size remains available, and the reopened job publishes the same bytes and digest with no segment requests. The build staged five versioned executables and 95 browser and setup-guide files. [Build receipt](adaptive-size-catalog-5.52-build.json)

The isolated Edge HLS fixtures passed seven TS and eight M4A checks. The exact sizes shown before Start were **1,961,216 bytes** for TS and **98,791 bytes** for M4A. Each matched the published size and staging SHA-256. The outputs had the selected audio language and expected timeline, and both decoded fully. The temporary native host registrations were removed.

## MP4 timing comparison

Three runs per version alternated which build ran first. Each fresh Edge profile selected the same controlled 720p HLS MP4 with Spanish audio. All six runs fetched nine segments, showed **6,560,076 bytes** before Start, and published bytes matching the staging digest. The fixture also checked the selected streams and decoded the complete timeline.

| Median across three runs | 5.51 | 5.52 |
| --- | ---: | ---: |
| Selection to enabled Start | 504 ms | 506 ms |
| Selection to first segment request | 763 ms | 1,025 ms |
| Selection to exact size | 3,071 ms | 2,862 ms |
| Last origin response finish to exact size | 1,279 ms | 1,181 ms |

The 5.52 exact-size times ranged from 2,447 to 3,650 ms. One catalog flush appeared after the origin finished and before the exact size in every run. The trace records both periodic and synchronous writes, so it does not identify the caller. The lock fixture isolates the removed dependency. The timing sample covers one controlled HLS selection.
