# IDM analysis and UDM parity report

Inspected September 18, 2026. Reference installation: `C:\Program Files (x86)\Internet Download Manager`.

## Executive finding

IDM is a suite of cooperating components, not one executable. Reproducing the desktop layout would cover only a fraction of its behavior. Download transport, persistence, browser capture, media parsing, site crawling, Windows integration and signed driver distribution each need separate acceptance criteria.

The installed main executable reports **6.43.10.2**, is an x86 PE image, and had a valid Authenticode signature during inspection. An initial expired-trial dialog limited that first inspection. Later direct observation confirmed that IDM was downloading the user's YouTube video with eight receiving connections and merging its streams. The trial dialog did not mean the transfer had stopped. This report combines local metadata, public documentation and [live observations](idm-live-observation.md); it is not a complete examination of every proprietary algorithm.

`installed-components.json` records file sizes, version descriptions and SHA-256 hashes. `binary-metadata.json` records selected PE architectures and signature results. The later [deep inspection](deep-inspection.md) adds ten PE import/export tables, input hashes and 105 decoded dialog templates, plus read-only inspection of the installed Chrome extension. No proprietary binary was modified or incorporated in UDM.

## Component map

| Installed component group | Evidence and role | Original UDM equivalent / work required |
|---|---|---|
| `IDMan.exe` | Main application; version metadata | Native WinForms desktop and task manager |
| `IDMMsgHost.exe`, two host JSON manifests | Browser native host; Chromium origins and Firefox extension allowlist | Original framed-JSON host and same-user pipe |
| `IDMGCExt*.crx`, `IDMEdgeExt.crx`, `idmmzcc*.xpi`, `IDMOpExt.nex` | Packaged browser integrations across generations | Original Chromium/Firefox source; Chrome YouTube handoff verified live, other browser matrices remain |
| `idmcchandler*`, `idmmzcc7*` | Version descriptions identify browser click catchers | Modern extension events cover a subset; arbitrary-process interception is missing |
| `IDMIECC*`, `IEMonitor.exe`, `downlWithIDM*`, `IDMGetAll*`, `IE*.htm` | Legacy IE/BHO/menu/click-monitoring components | Not implemented; separate legacy support decision |
| `idmbrbtn*` | Version description identifies download panels | Original floating YouTube panel, quality selector and popup discovery |
| `IDMNetMon*`, `idmnmcl.dll`, `IDMIntegrator64.exe` | Network monitor/module/loader metadata | Native messaging integration now; complete internal-equivalent monitoring is unknown and unfinished |
| `idmBroker.exe`, `MediumILStart.exe`, helpers | Broker/settings/helper descriptions | Single-user, unelevated application; privilege-broker parity is unfinished |
| `IDMShellExt*`, file-type modules | Shell and file-type integration metadata | Optional URL protocol installer; Explorer shell extension is missing |
| `idmtdi*`, `idmwfp*` | TDI/WFP INF packages, catalogs and architecture-specific drivers | See driver report; original x64 WFP monitor compiled and static-analyzed, not kernel-tested/signed |
| `IDMVMPrs*`, `idmvconv.dll` | Parser/module metadata; exact internals not observed | Original browser capture, UDM stream downloads and FFmpeg merging; SABR/UMP and generic playlists remain |
| `libssl.dll`, `libcrypto.dll` | OpenSSL version metadata reports 1.1.1g | Windows/.NET HTTP/TLS stack; no reuse of bundled libraries |
| CHM help files, language resources and toolbar bitmaps | Documentation, localization and theme assets | Original English UI, documentation and original vector icons |

Names and descriptions support a component's general role; they do not prove its internal algorithms or complete call graph. In particular, network-monitor and media-parser behavior was not reconstructed from names alone.

## Feature-by-feature status

The reference feature set includes segmented downloading, recovery, queues, scheduling, categories, browser handoff and site grabbing. [IDM features](https://www.internetdownloadmanager.com/features2.html)

| Feature | UDM status | Practical boundary |
|---|---|---|
| Add URL / batches / import / export | Implemented | Text lists and numeric ranges up to 1,000 URLs |
| HTTP / HTTPS | Loopback and real YouTube CDN transfer tested | Broad HTTPS matrix remains; current build selects TLS 1.2 |
| Parallel downloading | Implemented; tested | Independently scheduled fixed pieces; not IDM's dynamic splitting algorithm |
| Dynamic splitting of active segments | Missing | Scheduler currently assigns the next available piece |
| Pause/resume | Implemented; tested across restart | Requires ranges and strong ETag or Last-Modified; otherwise restarts |
| Changed file detection | Implemented; tested | Re-probes and restarts once; rejects inconsistent ranges |
| Retry / interrupted connection recovery | Implemented; tested | Bounded retries per piece; no indefinite retry policy |
| Unknown sizes / empty files / ignored ranges | Implemented; tested | Sequential fallback |
| Filename selection | Implemented | URL/manual names; Content-Disposition naming remains |
| Checksum verification | Implemented; tested | SHA-256 only |
| FTP | Source implemented, untested | Sequential fresh transfer; no REST resume/FTPS matrix |
| Multiple queues / ordering | Implemented | Flat queues, global and per-queue concurrency |
| Start time / weekday / overnight schedule | Implemented; boundary tests | Runs while UDM is running; no OS wake timers |
| Stop at schedule boundary | Implemented | Cancels active transfers, preserves progress, queues for next window |
| Synchronization queues | Missing | Conditional revalidation and repeated synchronization need design |
| Shutdown / modem disconnect on completion | Missing | No power or dial-up actions are taken |
| Bandwidth limits | Implemented | Global and per-file pacing; throughput tolerance not benchmarked |
| Download quota | Implemented | Fixed hourly accounting; not a rolling-window FAP model |
| Categories | Implemented | Built-in extension mapping and persistent custom names/folders; custom site/category rules missing |
| Clipboard offers / URL drag/drop | Implemented | Clipboard is opt-in; does not silently start downloads |
| Search / progress / speed / ETA | Implemented | Speed measured by periodic received-byte deltas |
| File Info / progress / completion dialogs | Implemented; live media flow verified | Destination/category/description/queue, real connection rows, range maps and open actions; background prefetch and all IDM completion actions remain |
| Per-download limiter preferences | Implemented; state tests | Temporary or remembered; video/audio share one aggregate gate; throughput tolerance remains unbenchmarked |
| Tray / notifications / sounds | Implemented | Live completion-notification scenarios not exhaustively tested |
| Browser context-menu download | Source implemented | Chromium/Firefox actual store/live verification outstanding |
| Automatic browser handoff | Source + mock tests | Opt-in; pause → durable handoff → cancel, with resume on failure |
| Referrer / cookies / Basic auth | Implemented | Credentials protected at rest; browser cookie partitioning not handled |
| Browser media discovery | Current-video metadata verified live | Early observer is present; actual qualities are detected. Adaptive SABR streams have no directly usable URL in the observed response. |
| YouTube MP4 video + audio | Direct-stream engine tested; historical 1080p output verified | Current browser-only 1080p handoff is blocked by unsupported SABR. No external resolver fallback; old timings do not apply. |
| HLS / DASH playlists / live / subtitles | Missing | Generic playlist assembly and subtitle selection remain unfinished |
| Full system-wide capture | Missing | Current WFP prototype does not implement capture |
| HTTP proxy | Implemented, untested proxy matrix | Windows proxy or explicit HTTP proxy; credentials supported |
| SOCKS / NTLM / Negotiate / Kerberos parity | Missing | Requires separate transport/auth implementation and tests |
| Site grabber | Implemented; tested | Static HTML, same-origin bounded traversal and extension filters |
| Offline website mirroring | Missing | No link rewriting, JS rendering or project persistence |
| Antivirus hook | Implemented | Launches configured scanner; no integrated clean/infected verdict gating |
| File execution | User action only | Completed downloads are never automatically executed |
| Shell / CLI | Partial | Original CLI, selected aliases and optional URL protocol registration |
| Localization / skins / column persistence | Missing | English UI with original icons; interactive column resizing available |
| Installer / uninstaller / updates | Partial | Per-user copy/protocol script; no signed installer or update channel |
| Accessibility / DPI | Partial | Native controls; main/add dialog visually inspected; full keyboard/DPI audit remains |

The 0.6 interface uses measured compact dialogs, a dark toolbar, nested categories and original icons. See [the deep layout inspection](deep-inspection.md), [dialog observations](idm-workflow.md) and [supplied setup/driver inspection](installer-analysis.md). It has no activation or trial-expiry system. The GUI is not an exact pixel copy; complete scheduler/properties/grabber parity, full settings-tab parity and a graphical installer remain unfinished.

The scheduler's synchronization and after-queue actions are materially broader than simply starting downloads at a time. [IDM scheduler documentation](https://support.internetdownloadmanager.com/support/idm-scheduler/idm_scheduler.html)

Likewise, IDM's grabber describes saved projects and website mirroring, whereas UDM currently provides a bounded link-discovery tool. [IDM grabber introduction](https://www.internetdownloadmanager.com/support/idm-grabber/idm_grabber.html)

Proxy/authentication variants, site logins, category rules and integration preferences require dedicated parity tests beyond the present settings dialog. [IDM options](https://www.internetdownloadmanager.com/support/options.html)

## Priority order for completing the replacement

1. **Stabilize file correctness and release behavior:** extend tests to real HTTPS/CDNs, large files, abrupt process termination, disk-full recovery, corrupt history, filesystem limitations and long paths. Add server-suggested filenames and publication recovery.
2. **Validate browsers:** install the development extension in clean Chrome/Edge/Firefox profiles, verify real downloads, permission changes, service-worker suspension, browser restart and authenticated redirects. Then complete store signing.
3. **Broaden transport:** implement/test FTP resume, FTPS if required, proxy types, authentication protocols and HTTP protocol negotiation. Benchmark rather than promise universal acceleration.
4. **Media pipeline:** implement VOD playlist parsing, rendition selection, segment acquisition, container assembly and expired-link refresh. Treat unsupported/encrypted formats as explicit unsupported cases.
5. **Scheduling and organization:** add persistent grabber projects, custom categories, synchronization queues, schedule wake behavior and optional completion actions.
6. **Integration/driver program:** define the system-wide capture behavior, then determine which portion needs WFP. Establish a service protocol, driver lifecycle tests, signing and rollback. The present prototype is only a starting experiment.
7. **Production experience:** finish localization, accessibility, theme/customization persistence, telemetry policy if any, crash recovery, signed installation and update rollback.

Full parity is not complete. This project provides an executable foundation, original integration source, reproducible checks and an explicit remaining-work map.
