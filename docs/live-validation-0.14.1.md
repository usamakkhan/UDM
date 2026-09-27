# UDM 0.14.1: live validation and Microsoft ISO comparison

Recorded 2026-09-23. UDM 0.14.1 fixes an HLS discovery race exposed by a real browser test and corrects the progress dialog's Pause/Resume availability. Browser integration and cross-site media validation included a new regression that fails on the previous code. Two consecutive real Chrome/host runs then passed all four end-to-end checks each.

## Changes found through live testing

The video panel could show an empty HLS menu even after playback began. A Chrome trace showed playlist response events arriving while their asynchronous session-storage writes were still pending; the menu read the old catalog. Discovery now waits for captures already queued for that tab before reading the catalog. The change is in both Chromium and Firefox extension files. It does not add a resolver, change site permissions, or relax stream validation.

The progress dialog now disables Pause when a job is paused, complete, or already pausing. Resume is disabled for active, queued, complete, pausing, or unresolved duplicate jobs. Both controls update immediately after an action and on the existing refresh timer. Paused and completed states were checked through the running MFC UI.

Desktop and extension version: **0.14.1**. The normal desktop was restarted with this build. The user's 10 download records are preserved. The extension files are updated on disk, but the everyday Chrome profile remains in another Windows session and was not reloaded; reload UDM Browser Integration there and refresh its video pages to activate the extension fix.

## Microsoft ISO results

The link was freshly generated on [Microsoft's Windows 11 download page](https://www.microsoft.com/en-us/software-download/windows11). Both apps requested `Win11_25H2_English_x64_v2.iso`, 8,471,603,200 bytes, with the same signed URL, eight configured connections and no speed limit. These tests used **UDM 0.14.0** and IDM 6.43; the patch above does not claim a transport speed improvement.

| App | First interval | Second interval | Mean |
| --- | ---: | ---: | ---: |
| UDM | 4.276 MiB/s | 4.188 MiB/s | **4.232 MiB/s** |
| IDM | 4.241 MiB/s | 3.705 MiB/s | **3.973 MiB/s** |

One MiB is 1,048,576 bytes. Each rate uses the increase in the saved application byte counter over approximately 60 seconds, starting 15 seconds after the sampler first observed an active transfer status. Run order was UDM, IDM, UDM, IDM, with the other app's test transfer paused. The total downloads ran longer than the measured windows so setup and UI pause timing did not shorten the windows.

**These results do not establish a general speed winner or full-download completion-time parity.** Important limits:

- The same hostname led to different remote CDN addresses: UDM used `151.101.22.172` and `2.18.67.202`; IDM used `23.15.3.197`, then `23.11.232.18` and `199.232.114.172`. The second IDM interval included a brief zero-connection observation and an endpoint change; the sampler cannot establish the cause.
- UDM used two fresh files. IDM began fetching during its File Info dialog; that setup transfer was excluded and its two measured intervals resumed the same partial file. This is a sustained-throughput check, not a matched startup test.
- UDM persisted counters about 40 times per window and IDM about 24–25 times. One transient UDM state-file read failed during atomic replacement; that missing sample was excluded and recorded. Endpoint timing and counter persistence limit precision.
- There are only two intervals per app. Ordinary activity in other Windows sessions was not disabled. GUI/process CPU samples include UI work and are not isolated backend CPU benchmarks.
- UDM stored parts on D:, IDM in its existing temporary folder on C:. The ISO was not completed in these new runs, so assembly, final publication and full-file completion time were not compared.

All test transfers are paused. IDM's new partial ISO remains available in its history; the UDM test jobs are confined to separate test state. Existing completed ISOs were not overwritten.

## Integrity

The existing complete reference ISO was freshly hashed and matched the English 64-bit hash shown on Microsoft's page:

`768984706B909479417B2368438909440F2967FF05C6A9195ED2667254E465E3`

Every saved UDM partial byte was then compared with that reference at its assigned original offset: **445,317,120 bytes** in sample A and **532,807,680 bytes** in sample B, **978,124,800 bytes total** across 16 parts. All matched. This verifies the received portions, not completion of either new ISO. IDM's proprietary partial-file layout was not treated as a verified final ISO.

## Backend comparison

Both apps demonstrated eight concurrent HTTPS connections in the measured periods and usable pause/resume behavior. UDM's source uses asynchronous WinHTTP, shared sessions, validated byte ranges, persistent segment ownership and redistribution of unfinished tails. The earlier read-only [IDM transport inspection](reverse-engineering-0.10.md) found custom Winsock/OpenSSL paths and observed HTTP range splitting on controlled fixtures. That evidence does not recover IDM's full algorithm or prove which internal path handled every request in this test.

This test supplies no evidence that a kernel driver is needed to achieve comparable ordinary HTTP throughput. No driver, Test Mode, licensing data or system network setting was changed. UDM remains an independently implemented app; complete IDM feature parity is still outstanding.

## Real browser validation and reproduction

The new `tests/browser.native-live.cjs` launches installed Chrome in a separate profile, loads the actual unpacked extension through Chrome's extension-testing protocol, launches an isolated native UDM instance, and serves synthetic local video. It exercises trusted clicks on the real closed-shadow video panel and the registered native messaging host, with no mocked Chrome APIs. It verifies:

1. A real extension/native-host/desktop ping round trip.
2. A panel-selected 360p MP4 with byte-identical output.
3. An observed HLS playlist assembled into approximately eight seconds of 360p H.264 video with AAC audio.
4. No page-script errors during those workflows.

The fixture manifest adds localhost permission only in its copied test extension; user site-permission prompts, the everyday Chrome profile, YouTube, DRM, live streams and universal platform coverage are not validated by this test. Firefox's files were synchronized and covered by shared unit checks; Firefox was not exercised live.

Prerequisites: current Windows Chrome, Node with Playwright available, built `release` binaries, registered UDM native host, and FFmpeg/ffprobe under `release/tools`. Run from the project root:

```powershell
node tests/browser.native-live.cjs benchmarks/a-new-empty-output-directory
node tests/media.test.cjs
node tests/browser.test.cjs
```

Use a new output directory for every live run. The script closes only its test browser, fixture server and isolated app. Local evidence, including raw samples, observer source, the failed-before/passed-after regression logs and private signed input URL, is under `benchmarks/live-0.14.0`. Two fixed live runs are under `benchmarks/live-0.14.1-fixed-a` and `benchmarks/live-0.14.1-fixed-b`. Signed URLs, browser profiles and partial downloads are excluded from the release package. Sanitized results are in [reference/live-validation-0.14.1.json](reference/live-validation-0.14.1.json).
