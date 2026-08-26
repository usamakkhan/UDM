# Current validation

The active 0.7.0 implementation is C++/MFC. See [native-conversion.md](native-conversion.md) for its 64 native checks, 36 browser checks, real host and MFC download validation. The report below is historical C# evidence; its older driver status and test counts do not describe the native release.

---
# Validation record — UDM 0.6.1

Current desktop suite: **99 C# checks passed** on 2026-09-18. Browser results are **26 API**, **4 early-capture** and **6 UMP/capture checks**. Controlled tests include queue/project/authentication workflows, SABR transcript assembly/rejections/quota cancellation, device-protocol bounds and actual loopback process attribution. Tests cover capture-only native handoff, preservation of early metadata, separate ad/video catalogs, nominal versus cropped dimensions, missing URLs, prevention of publishing the wrong resolution, and persistent custom categories. The application, native host and network-monitor CLI compile. The original x64 WFP driver separately compiles with MSVC/WDK, passes C/C++ static analysis and InfVerif /w, and has NX/dynamic-base/CFG flags in its PE header. No driver was installed or kernel-loaded; no Driver Verifier/VM results are claimed. No yt-dlp call or Node runtime remains in application/setup code.

The 0.5 main window, tabbed configuration, progress and completion dialogs were inspected through the Windows computer-use tool. The main window renders at 746 × 444 on the current desktop; dialog scaling and toolbar wrapping were corrected during review. The icon builder produces 29 PNGs and an ICO. Full keyboard, every dialog and mixed-monitor DPI behavior remain unverified.

Static reference inspection decoded 105 dialog resources without parser errors and recorded hashes/imports/exports for ten binaries. This verifies the inspection artifact, not the complete behavior of the proprietary reference.

A live 360p format-18 handoff displayed File Info, but the media request was rejected before any bytes arrived; follow-up GET probes returned HTTP 403 with and without Range. No new video or speed result was produced. The desktop now preserves HTTP codes in media errors. The real native-host stdio/pipe ping also passed on the running 0.5 build.

The updated desktop, Network integration window and driver-status message were inspected live after Computer Use recovered. The monitor displayed real browser TCP connections and UDP endpoints, with disabled WFP controls while the driver is absent. The earlier 0.6 native-host stdio/desktop-pipe round trip passed; no new live YouTube download is claimed.

The driver and catalog were test-signed using UDM's own non-exportable development key. Inf2Cat reported no errors/warnings; both CMS signatures verified. Windows reports the certificate chain as untrusted, as expected: no root certificate was installed. Elevated diagnostics confirmed administrator access, Secure Boot off, Test Mode off, no loaded UDM driver and zero configured Hyper-V VMs. A separate opt-in live-driver smoke executable was compiled; its traffic fixture passed locally, but the kernel checks have not run.

These controlled fixtures do not establish live YouTube compatibility. See [current capture status](browser-capture.md).

## Historical 0.3 validation

The following results describe the older resolver-based build. Its automatic link refresh and extraction tests do not apply to 0.4.1.


## Completed checks

- The desktop and native host compile with the installed .NET Framework C# compiler.
- **55 C# checks passed** against loopback HTTP fixtures and real local storage, DPAPI, pipe, FFmpeg and FFprobe APIs.
- **16 browser checks passed** using mocked extension APIs.
- **1 real native-host check passed**: fragmented stdio framing, native executable, same-user pipe and a ping response from the running desktop.
- The main window, icons, Add Downloads dialog and Chrome player panel were visually inspected through the Windows computer-use tool.
- The icon build produces 25 PNG icons plus a multiresolution ICO.
- Chrome's unpacked extension was loaded and the native host registered for this user. Its player button successfully added a real 1080p download. A full browser exit/restart resolved the initial native-host lookup failure.
- Two completed real YouTube runs were inspected with FFprobe and contain H.264 1920×1080 video and AAC audio. Their MP4 hashes match. The browser-initiated output hash was independently recomputed.

See [benchmark.md](benchmark.md) for timings and verification. An additional browser attempt received HTTP 403. After the bounded link-refresh fix, resuming that job refreshed its rejected playback links and completed with the same verified output hash. The original failure is retained as a reliability finding.

The 0.3 checks also cover real worker byte accounting, durable browser confirmation, pending-request deduplication, destination/category-folder selection, temporary versus remembered limits, resume restoring the remembered limit, and destination collisions. The live dialog flow completed another 1080p MP4 with audio; its hash and tracks were independently verified. Main-window, progress-tab, limiter and completion-dialog appearance were checked through the computer-use tool. Full UI-control behavior and all DPI settings remain unverified.

The C# suite covers filename sanitization, batch expansion, unsupported URLs, overnight scheduling, DPAPI protection, exact parallel assembly, multiple range requests, ignored ranges, unknown-length transfers, redirects, missing validators, dropped connections, empty resources, changed ETags, incorrect Content-Range, bad hashes, HTTP errors, overwrite protection, Basic auth, plaintext-secret avoidance, grabber origin/depth/deduplication, record removal, pause/restart/resume, scheduler eligibility and named-pipe handoff.

The initial sandboxed run could not connect to the restricted pipe. The same tests passed outside the sandbox without relaxing the pipe ACL. An earlier Internet-zone-marking bug was found by the transfer suite and fixed using the Windows file API for the alternate data stream.

Media tests generate video/audio fixtures, download both through UDM, merge them with FFmpeg, verify tracks with FFprobe, check persisted timing, preserve inputs after failed merging, and prevent overwriting existing output. Regression checks cover nested ArrayList metadata, title punctuation, rejected-URL status classification, duplicate native handoffs, different quality selections and separate ordinary URL jobs.

The browser suite verifies opt-in behavior, pause/acknowledge/cancel ordering, resumption after host failure, excluded hosts, private/blob skips, permission-gated cookie access, manifest JSON, concurrent observations, ad/stale-stream avoidance through page-ID resolution, UMP exclusion, quality and sender validation, navigation cleanup and targeted player permissions. Mock checks do not prove all live browser behavior.

## Not verified

- Broad HTTPS/CDN compatibility, repeated controlled IDM/UDM benchmarks and universal speed claims.
- Every cause of HTTP 401/403; the current build requires fresh browser capture after rejection and does not run an external resolver.
- Live FTP, proxy authentication matrices, scanner execution or all scheduling scenarios.
- Live Edge/Firefox installation, store distribution, general automatic-capture permission flows and account-specific media.
- Driver compilation, static analysis, VM loading, network behavior, verifier tests or signing.
- Installer execution, upgrade/uninstall behavior or store distribution.
- Complete accessibility, DPI, long-path, disk-full, power-loss, large-download or nested media cleanup testing.

Repeat targeted checks after relevant source changes. The compiler emits an existing CS4014 warning for the fixture's intentionally concurrent connection handler. This remains a development build with the explicit gaps in [analysis.md](analysis.md).
