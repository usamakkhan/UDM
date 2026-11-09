# YouTube startup after verified timed media

The first incomplete streaming window now verifies and saves a selected timed segment before opening the other configured windows. Initialization remains reusable in the cache. File Info and the session recovery dialog distinguish retained setup data from progress on the selected timeline.

## Baseline from the cold public 5.39 runs

| Selection | Peak requests | Rejected window | Verified bytes | Verified timeline |
|---|---:|---:|---:|---:|
| 360p H.264 MP4 | 8 | 3 | 2,051 | 0 ms |
| 1080p H.264 MP4 | 8 | 4 | 2,051 | 0 ms |

Both runs received an attestation challenge on a secondary window after the first window accepted initialization payload. Each cache contained only the audio initialization record. The 5.40 update changed recovery selection and retained that startup policy. [Prior cold timing](sabr-one-click-refresh-5.40.md)

## Validation scope

The native fixture exercises a challenge after selected initialization for both video with audio and audio only. Healthy synthetic streams still overlap their responses after the first verified timed segment. Tests use isolated application and browser profiles.

All 257 streaming and recovery checks and 163 native presentation checks passed. The Chromium and Firefox 0.69.5 bundles each passed 66 transport and 31 streaming audio checks. Five production executables and both browser bundles are staged. [Native build receipt](sabr-timed-startup-5.41-build.json)

## Cold public 5.41 runs

| Selection | Click to actionable File Info | Click to visible Refresh | Verified bytes held | Verified timeline from zero | Rejected window |
|---|---:|---:|---:|---:|---:|
| 360p H.264 MP4 | 752 ms | 5,235 ms | 257,592 | 9.985 s | 3 |
| 1080p H.264 MP4 | 1,004 ms | 5,680 ms | 317,574 | 3.000 s | 4 |

Both runs opened their peer windows after verifying timed media, reached eight concurrent requests and then received an attestation challenge on a peer's first request. Every millisecond of recorded coverage belonged to the first window. Closing File Info preserved the verified checkpoint and the hold. Both runs left exact size and publication pending. All temporary host registrations were removed.

The next recovery test should use a fresh browser session and reuse the verified zero-based prefix on a single connection. The app currently seeks to the verified cursor only when the original transfer used one configured window. The latest samples show why recovery should distinguish a contiguous prefix from verified coverage in disjoint windows. A paired installed IDM comparison remains open.
