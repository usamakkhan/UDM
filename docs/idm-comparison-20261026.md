# UDM versus actual IDM — 26 October 2026

## Conclusion

UDM has substantial implementation coverage, but complete functional, visual and performance parity is **not established**. The most immediate delivery gap is that the installed application is older than the current source. Do not rebuild old missing-feature lists indiscriminately, and do not call a feature equivalent merely because its controls or regression tests exist.

This review combines fresh inspection of the installed applications, current source, retained IDM observations, prior paired transfer measurements and 12 freshly executed browser suites. It is a broad comparison, not an exhaustive execution of every IDM feature. The separate Awesome Downloader comparison does not serve as an IDM reference.

## Exact subjects and fresh observations

- Reference executable: installed IDM **6.43**, file version **6,43,10,2**; Authenticode status **Valid**. This is the local reference, not a claim about the newest available IDM release.
- Installed UDM: **1.0.4**, SHA-256 `1416BF94947D94D9D13A12598B919E2A32A1935F6D54080A5B21201F99A8990C`; Authenticode **NotSigned**. This matches the October 22 installation recorded previously.
- Installed Chromium extension files: **0.63.37**. This read does not independently prove what an already-running browser has loaded.
- Current source: HEAD `7209abe` plus local changes; extension source **0.63.38**. Existing Grabber UI and `.gitignore` edits were preserved.
- Both main windows were opened and observed. IDM has no ordinary status bar; the installed UDM still shows its download-count/rate/quota status bar. Installed UDM also shows an Other category, extra Images category, expanded queues, paused-state wording and idle `--` values. Several of these are already addressed by newer source according to the GUI audit; installation must be qualified separately.
- UDM's actual Configuration dialog still has More, OK, Cancel and Apply. Newer source documentation describes a different frame/button arrangement. This is direct evidence that source screenshots and installed behavior cannot be treated interchangeably.
- IDM temporarily showed Not Responding after Options was clicked. The main window recovered and exposed its accessibility tree, but the second attempt again failed to produce a configuration dialog. Cause unknown; this is not a general IDM performance finding. No process was killed.
- UDM Configuration was closed with Escape without saving. No existing downloads, queues, settings, files or browser sessions were changed intentionally. Personal catalog contents are not copied into this report.

## Comparison by subsystem

“Implemented” below means a source path and/or retained qualification exists. It does not mean the current installed binaries have just passed an IDM side-by-side test.

| Area | IDM reference | UDM evidence / assessment | Still required |
| --- | --- | --- | --- |
| Main window and categories | Fresh installed observation; retained full menu capture | Familiar native layout. Fresh installed differences listed above; later source work exists. | Install qualified current build and compare same DPI/profile conditions. |
| Toolbar, columns, sorting, Find | Retained actual IDM captures in GUI audit | Toolbar, column, Find and sorting implementations and dedicated UI tests exist. | Keyboard, mixed selection, custom layouts and installed persistence comparisons. |
| Add URL, batch, clipboard, basket | IDM guide and retained dialogs | `Ui.hpp`, batch and clipboard tests, `BasketUi.hpp` supply these workflows. | Real browser drag/drop and installed clipboard behavior matrix. |
| File Info and filename preview | Actual reference captures; IDM starting-download guide | `DownloadInfoUi.hpp`, filename and ZIP preview tests. | Same-input, same-DPI screenshots and timing on the current install. |
| Dialog speed and focus | Prior user observations favor IDM; no controlled paired latency distribution | Prior UDM 1080p test observed standalone File Info and hidden main list. | Instrument click → handoff → visible dialog; compare cold/warm median and tail latency. The recorded 7,136 ms observation bound includes tool delay. |
| Before-confirmation downloading | IDM behavior previously observed; dependent on settings/path | UDM `beginPrefetch` and prior real 1080p result prove bytes received and merging before Start. | Paired settings and cancel/later/restart/folder-change cases on installed build. |
| Exact final media size | Existing claim about IDM is not universal evidence | UDM prior 1080p test changed from track total to exact 56,731,539 bytes before Start; final file matched and decoded. | Compare multiple videos, formats and qualities; measure time to exact size separately from time to dialog. |
| Progress, limiter, completion | Retained reference dialogs and official options | `ProgressUi.hpp`, completion/limiter/tray tests. | Fresh real transfers, pause/close semantics, speed sampling and completion actions. A prefetch progress jump must not be mistaken for measured instantaneous network throughput. |
| HTTP/HTTPS segmentation | IDM documents dynamic segmentation; paired local origin evidence exists | UDM dynamic splitting, range validation, partial reuse, single-file staging and reconnect paths exist. | Fresh repeated equal-route/equal-storage A/B using the actual candidate. |
| Connection drops and outage | October 18 actual IDM benchmark completed difficult cases | Earlier UDM failures prompted retry/outage improvements; October 23 records discuss later results. | Requalify latest installed build; do not report old failures as current source defects. |
| Sequential and disk throughput | Paired benchmarks retained | 256 KiB reads, unlocked worker writes, HTTP/3 restriction for multirange and buffered cross-volume publication are in later source. | Same temporary drive, connection cap, cache and machine load; per-case results rather than a blanket speed claim. |
| Pause, resume, restart integrity | IDM observed in prior testing; killed-IDM run invalid in old benchmark | UDM pause transaction/checkpoint/validator tests exist. | Real paired interrupted-transfer cases without disrupting personal downloads. |
| Automatic browser capture and POST | IDM guide describes takeover and key overrides | Source supports bounded POST, cookies, context identity, prepare/commit receipts and recovery. Fresh selected suites passed. | Chrome/Edge/Firefox installed tests with redirects, expiring sessions and one-use responses. |
| YouTube detection and ads | User reported IDM detecting earlier; prior live observations | Fresh ad/page and readiness suites pass. No fresh live ad comparison. | Repeat real pre-roll/skipped/unskipped ads and SPA navigation; prevent wrong-video capture. |
| YouTube formats / SABR | Prior reference selections; exact coverage not exhaustive | UDM direct/SABR handoff and audio selection tests pass; prior 1080p playback evidence exists. | Paired selected format, final size, full decode and expired-session tests. |
| Other-site HLS/DASH | IDM advertised video capture; no universal per-format guarantee | UDM nested masters, indexes, alternate audio, subtitles and live HLS paths have specific tests. | Same public fixtures through both applications; codec/DRM/live limitations remain explicit. |
| Multiple videos in a feed | Previous external-project finding, not yet actual IDM proof | New 0.63.38 twimg poster matching passed local tests. | Live X/Twitter and actual IDM comparison; not installed in this task. |
| Scheduler and queues | Retained actual Scheduler screenshots | `SchedulerUi.hpp` has queue/parallel/retry/time/completion controls. October 29 source recheck: native time pickers with seconds and spin are present. The text-edit description is superseded; queue controls still need installed comparison. | Label these as partial UI equivalence, not identical; overnight/DST/wake live tests. |
| Categories, duplicates, history | Retained dialogs and source tests | Category rules, duplicate handling, history operations and durable catalog paths exist. | Installed profile migration and mixed-state action comparisons. |
| Site Grabber / offline sites | Retained IDM wizard; some later reference steps were not reached | Native crawler, rendered capture, offline ZIP/media/captions and result actions exist. Local Grabber edits are still uncommitted. | Full same-site wizard-to-offline-output comparison, authentication and project resume. |
| ZIP and audio metadata | IDM preview/reference research | UDM remote ZIP, ID3v1/v2 and audio metadata tests exist. | Actual fixture comparison. Old research saying ID3 extraction is absent is stale. |
| Proxy, authentication, FTP | IDM options and prior reference research | UDM WinHTTP/curl, proxy/PAC, Windows login and FTP paths exist; fresh browser proxy suite passed. | Domain Kerberos, difficult PAC/auth routes and real FTP/proxy failure matrix. |
| Network monitor takeover | Retained research; browser plaintext and wire-level TLS must be distinguished | HTTP observation/capture paths exist. HTTPS endpoint monitoring is not HTTPS response takeover. | VPN/HVCI, sleep and physical network-change qualification. Do not infer a requirement for machine-wide TLS interception from IDM integration. |
| CLI, COM, Explorer, URL scheme | Prior reference interface research | Automation/server and shell integration source exists. | Current installed cold activation, registration/repair/uninstall and 32/64-bit client cases. |
| Export/import and backup | Retained IDM format research | `IdmExport.hpp`, import UI, catalogs and backup tests exist. | Real IDM-created export → UDM → IDM round trip with supported metadata checked. |
| Signing and updater | Fresh installed signature check; IDM update workflow documented | Installed UDM unsigned. Current source has signed manifest/updater and packaging changes. | Public publisher signing, installed update/rollback and production upgrade acceptance. |
| Accessibility, DPI, localization | Retained UI observations; no full current matrix | Native controls and UI tests exist; English-only is an intentional scope choice. | Mixed-monitor DPI, keyboard-only, screen-reader/high-contrast qualification. |
| Physical fault recovery | No comprehensive paired current reference evidence | Fault-injection and catalog tests cover narrower cases. | Disk-full, device removal, sleep/hibernate, power loss and cancellation with owned fixtures. |

## Fresh regression evidence

Twelve suites passed, **655 checks**, with no failures: media 37; HLS site integration 57; integration parity 33; capture transaction 127; transport handoff 62; video readiness 17; YouTube ad/page capture 192; audio tracks 31; site deadlines 28; capture recovery 27; download sessions 31; browser proxy 13. Nested handshake cases are not counted a second time.

[Sanitized results](validation/idm-comparison-20261026.json). These execute current source through local harnesses. No native rebuild, full native suite, new paired download benchmark or live browser download was run in this review. The benchmark harness closes IDM and includes destructive interruption cases, so it was not launched against the user's open application.

## Corrections and next priorities

1. **Delivery first:** build/test/package the current source, then install and verify hashes. Existing version number 1.0.4 alone cannot identify which fixes are installed.
2. **Measure the user-visible gap:** real browser click-to-visible-dialog and time-to-exact-size, prefetch bytes at Start, main-window activation, ad detection and format correctness. Use repeated same-video selections and actual timestamps, not tool-call duration.
3. **Re-run transfer comparisons:** current candidate against actual IDM, same local origin, temporary storage, output integrity and controlled machine load. Include dropped connections, outage, refused extra connections and sequential transfers.
4. **Finish the UI acceptance matrix:** installed options, Scheduler controls, Grabber steps, keyboard/DPI and selection-state behavior. A “Done” row that lists different controls should not imply identical interaction.
5. **Release qualification:** signing, updater rollback, clean-machine install/repair/uninstall; separate driver/VPN/HVCI and physical recovery environment.

The blanket completion/speed assertion in `parity-decisions-2026-10-24.md` was corrected. Historical snapshots remain historical, rather than being silently rewritten as current test results. No parity percentage is given: the workflow list is finite, unequal in scope and not exhaustive.

## Sources

- [IDM official feature description](https://www.internetdownloadmanager.com/features2.html): dynamic segmentation and advertised integration; marketing speed claims are not measurements for this PC.
- [IDM starting downloads](https://www.internetdownloadmanager.com/support/using_idm/starting.html): File Info, later/start, preview and capture entry points.
- [IDM options](https://www.internetdownloadmanager.com/support/options.html): settings categories and integration controls; documentation is not a substitute for installed-version behavior.
- [Retained actual GUI comparisons](gui-parity-2026-10-18.md).
- [Earlier paired transfer benchmark](speed-benchmark-2026-10-18.md) and [later performance work and comparisons](speed-benchmark-2026-10-23.md).
- [Installed 1080p file-size/prefetch/full-decode evidence](validation/browser-06337-1080p-20261023.json).
