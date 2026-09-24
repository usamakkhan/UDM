# UDM 0.16.0: video capture and measured transport improvements

UDM 0.16.0 is installed and running from `D:\UDM\release\UDM.exe`. It contains original UDM code and assets. No IDM binaries, extension code, driver or artwork are included.

## What changed

- A browser-level observer now captures bounded SABR/UMP POST request bodies, including worker traffic missed by page-level fetch/XHR wrappers. It requires a successful UMP response, current non-ad player identity, matching frame and document, and a capture no older than five minutes. Navigation invalidates pending captures. Request context stays in browser session storage until explicitly handed to UDM; saved desktop context uses the existing DPAPI protection.
- Before accepting streamed media, the native parser requires explicit matching video identity in format initialization. Capturing a successful browser request alone does not prove that its payload belongs to the requested video.
- Quality selection prioritizes a usable codec at the chosen height over an unavailable preferred codec. SABR addresses cannot be mistaken for raw MP4. Quality preparation now handles Shorts and embedded players. Existing format discovery still uses the current player's actual catalog.
- Ordinary HTTP reads use 16 KiB instead of 64 KiB. In the earlier controlled local tests this reduced median elapsed time by 9.0% for the steady fixture, 2.8% for delayed headers and 1.2% for the straggler fixture. These are local measurements, not proof of an equivalent Microsoft ISO speed gain. See [transport analysis](transport-analysis-2026-09-24.md).
- Periodic persistence skips unchanged state. Explicit saves still write atomically; changed state and failed writes remain eligible for checkpoint/retry.

No browser permissions were added. The extension ID is unchanged. Chrome's older C: registration path resolves through a junction to the current D: browser source; it is not a separate stale copy. Existing browser runtimes must reload the extension to execute the new files.

## What the IDM examination established

The observed IDM menu on the public test video offered six actual heights (1080p through 144p), four TTML subtitle entries and Download all. Its compact panel sits near the upper-right player edge. Static examination found browser request/body correlation and a native format parser handling `itag`, FPS, content length, initialization ranges, cipher fields and `isDrc`. Another native path handles `captionTracks`, subtitle URLs and language codes. These observations support additional subtitle/audio-variant work; they do not reveal the complete original source or prove IDM's exact live SABR algorithm.

The earlier runtime trace established eight long HTTP/1.1 connections for both apps, direct Winsock/Schannel in IDM and WinHTTP/Schannel in UDM. It did not establish a kernel driver as the cause of IDM's measured advantage. No driver or Windows security setting was changed for this release.

## Validation

- Native C++: **222 checks passed**, including missing/foreign video identity, unchanged checkpoints, atomic persistence failure and retry.
- JavaScript: **89 checks passed** across browser, page capture, UMP, media and streaming capture suites; extension preparation also passed twice with identity/capability/source consistency checks.
- Real Chrome/native integration: **six checks passed**, including a byte-identical direct MP4, 360p HLS output with audio, automatic handoff, and exclusion of a previous embedded document.
- Real Chrome worker capture: passed. The page observer saw zero requests while the browser observer captured one successful request, the exact POST body and expected video/document identity. This fixture routes synthetic HTTPS responses locally; it is not a YouTube service test.
- Installed app: native-host ping passed; all 13 download records retained their IDs, status, paths and recorded byte counts (eight complete, three failed, two paused). The idle state file remained unchanged over five seconds. Upgrade backup: `D:\UDM\backups\before-0.16.0`.

Local evidence is under `benchmarks/video-integration-20260924`: `native-build-test.log`, `browser-e2e/extension-results.json`, `streaming-browser-a/result.json`, `youtube-live-a/result.json`, `deployment.json` and `post-upgrade.json`. Benchmarks, browser profiles, captures, private history and downloaded tools are excluded from the portable package.

## Live verification and remaining gaps

The isolated live YouTube test encountered Google's unusual-traffic CAPTCHA before playback. No stream capture, download or speed conclusion can be drawn from that attempt. The original ambiguous result has been retained as `originalOutcome` and annotated with the reviewed blocker. The harness now recognizes the challenge redirect explicitly.

The everyday Chrome runtime reload is pending: Windows returned Access Denied to computer-control input, and the user was asked to reload the extension and refresh playback. Native-host connectivity is verified, but live YouTube completion through the new capture path is not. The new browser observer removes a demonstrated capture gap; it does not establish universal protocol compatibility.

Subtitle downloading, selectable language/DRC audio variants, live/DRM streams and full IDM parity remain incomplete. A successful fresh live capture, verified audio/video output and a controlled same-source throughput comparison are still needed before claiming YouTube compatibility or IDM speed parity.

## Reproduce controlled checks

```powershell
.\native\build.ps1 -Test
node .\tests\browser.test.cjs
node .\tests\capture.test.cjs
node .\tests\ump.test.cjs
node .\tests\media.test.cjs
node .\tests\streaming-capture.test.cjs
node .\tests\prepare.test.cjs
# Playwright and Chrome are needed for the isolated browser fixtures:
node .\tests\streaming.browser.cjs .\benchmarks\streaming-browser-new
# Set UDM_APP_EXE to the isolated native build before browser.native-live.cjs.
node .\tests\browser.native-live.cjs .\benchmarks\browser-e2e-new
# Ping the running production app; creates no downloads:
node .\tests\native-host.cjs
```
