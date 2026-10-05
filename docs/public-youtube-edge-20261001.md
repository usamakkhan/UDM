# Public YouTube verification in Edge — 1 October 2026

The staged native 0.84.0 / browser 0.55.0 candidate now has fresh public-site evidence: selecting the actual UDM panel downloaded Big Buck Bunny at 360p and 1080p60, with separate video/audio tracks merged into complete MP4 files. Both files passed a full FFmpeg video/audio decode with error-as-failure enabled. The 1080p file contains 1920×1080 H.264 at 60 fps and AAC audio, lasting about 634.6 seconds.

| Selection | Output bytes | Click to admission | First received bytes observed | Completion observed |
| --- | ---: | ---: | ---: | ---: |
| 360p | 28,864,389 | 5.51 s | 8.09 s | 18.02 s, including ffprobe inspection |
| 1080p60 | 268,281,336 | 0.50 s | 3.13 s | 70.95 s, before inspection |

The fixture uses a monotonic clock and polls the private catalog every 250 ms. These are individual runs, not matched IDM speed comparisons. Native player retrieval and direct video/audio URLs were used; no external resolver subprocess was used. The 1080p diagnostics report a 4.53-second lookup but only 0.50-second click-to-admission time, consistent with the existing lookup prefetch. This lower admission delay must not be generalized to every cold selection.

The first attempt failed before admission while YouTube displayed a playback error. That failed evidence is retained. A no-extension baseline then played successfully; both later UDM runs also verified playback before selecting a format. Browser traces still contain CDN HTTP 403/DNS failures, and browser playback failed late in the successful 1080p download. The evidence does not establish the cause of those playback failures.

Added `project/tests/youtube-public.edge.cjs` to the staged candidate. It uses a fresh Edge profile, copied extension, unique native-host registration, private native catalog, trusted panel selection, exact quality/output checks and cleanup verification. It records only sanitized network status metadata. All temporary registrations are independently absent, and the five protected app/catalog hashes plus the personal Edge registration remain unchanged.

No production code, installer, installed extension or personal app was changed. Remaining work includes Dailymotion and other public platforms, SABR/live recovery, repeated/ad-associated capture, matched IDM performance, and the existing GUI/driver/deployment qualifications. This is verification progress, not full IDM parity.

Evidence: `D:\UDM-Workspace\candidates\release-084-055\control\public-youtube-edge-acceptance.json`. The two successful reports, first failed attempt, clean-browser baseline, screenshots, output hashes, stream metadata and private fixtures are retained under that candidate directory.
