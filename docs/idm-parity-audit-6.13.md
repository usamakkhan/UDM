# UDM 6.13 and IDM 6.43.10.2 parity plan

November 8, 2026. This is the execution plan for the latest development build. It extends the [P0–P6 architecture](idm-parity-architecture-plan.md), [detailed inventory](idm-parity-execution-plan-20261107.md), and [5.45 comparison](idm-parity-architecture-audit-5.45.md). Parity means equivalent behavior, reliability, and interaction under matched conditions. UDM uses its own name, implementation, and artwork.

## Candidate and evidence boundaries

| Item | Evidence and scope |
| --- | --- |
| UDM desktop | All five production targets built as 6.13. The loopback runner checked the staged desktop executable against the build receipt before launching it. [Build receipt](validation/schedule-wizard-transfer-settings-6.13-build.json) |
| Chromium and Firefox | Both 0.69.77 bundles are staged and hash-checked. The isolated Edge run loaded the staged Chromium bundle. Installed bytes, active personal-profile manifests, and Firefox behavior still require separate observations. |
| Local setup | Setup was built for this PC with the verified native and browser bundles and the existing signed network package. Its SHA-256 is `DC2DF0913243A544065F11E37CB4A43B2DD8D06D8ACD488D464EE80F8DEC0736`. Setup requested Windows elevation. See the installation checkpoint below. |
| Reference | The installed IDM executable reports 6.43.10.2. Current paired evidence comes from running it against the same owned origin as UDM. Current interactive reference observations require a targetable IDM window. Use the retained geometry and vendor behavior as separate evidence. |
| Scheduling | The one-time wizard accepts local start and optional stop dates, editable folder and filename, category, description, retries, and wake behavior. 6.12 added relative date/time shortcuts and a full destination preview. 6.13 added max connections and per-file speed limit for direct and waiting jobs. These are source and compile observations until the specific UI and reload steps pass. |

### Current paired transfer comparison

The 18 outputs matched the expected length and SHA-256. The apps ran sequentially on the same Windows machine and destination volume. The four 16–24 MiB cases alternated launch order across two trials. The harness polled in 100 ms steps and measured dispatch through hash verification. UDM used an isolated catalog and an eight-connection setting. IDM used its existing profile and its setting was unverified. The compiler and setup packager were active during portions of the comparison. These are diagnostic timings with limited samples. [Sanitized run records](validation/idm-transfer-audit-6.13.json)

| Case | UDM seconds | IDM seconds | UDM requests | IDM requests | Next question |
| --- | --- | --- | --- | --- | --- |
| 1 MiB control | 1.177 | 0.836 | 1 | 7 | Separate app dispatch, transfer, final publication, and harness polling. UDM served 1 MiB once. IDM's origin served 2,265,053 bytes. |
| 24 MiB, healthy ranges | 2.345, 2.519 | 2.536, 2.457 | 6, 6 | 6, 13 | UDM made four dynamic splits in both trials. Keep this workload in the regression matrix when adjusting the range planner. |
| 24 MiB, origin allows two connections | 4.216, 4.214 | 5.389, 5.371 | 2, 2 | 3, 3 | UDM had one refused connection each time, IDM had nine. Test whether repeated jobs can avoid the first refusal without guessing the cap. |
| 16 MiB, server ignores Range | 8.318, 5.212 | 5.697, 5.111 | 2, 2 | 4, 4 | Largest UDM timing spread. Identify whether fallback, process dispatch, local publication, or contention caused the first run. |
| 24 MiB, disconnect after 3 MiB per connection | 2.409, 2.384 | 3.570, 2.582 | 13, 12 | 17, 16 | UDM survived six server drops each time. IDM survived four. Measure resumed intervals and duplicate bytes alongside timing. |

The older 5.45 audit observed 13 healthy requests per trial with its fixture. The 6.13 run observed six. Origin caps, concurrent load, and settings need to be matched against a rerun of 5.45 before attributing the full difference to code. For context, IDM documents dynamic range division and connection reuse, including helping the remaining slow ranges. [Earlier run](validation/idm-transfer-audit-5.45.json) · [IDM segmentation](https://www.internetdownloadmanager.com/support/segmentation.html)

### Follow-up on the sequential transfer timing in 6.14

Five repeats of the ignored-range case on the staged 6.14 desktop took 4.862–5.243 seconds. All outputs matched SHA-256. The native HTTP entry to first progress interval was 109–110 ms, the body took 3.921–3.938 seconds, and the last byte and range verification shared the same recorded tick in all five repeats. The earlier 8.318-second observation did not recur.

The benchmark now distinguishes an origin that ignores ranges, an explicit opt-out, and range support without an advertisement. Each case used the same 16 MiB payload, 15 ms response latency and 4 MiB/s per connection. All six outputs matched SHA-256. [Benchmark cases and timing fields](../benchmarks/speed/README.md)

| Origin response | Wall seconds | Requests per run | Range requests per run | Origin bytes served per run |
| --- | --- | --- | --- | --- |
| No range header, range requests ignored | 4.856, 4.842 | 2 | 1 ignored | 17,039,360 |
| `Accept-Ranges: none` | 4.844, 4.845 | 1 | 0 | 16,777,216 |
| No range header, valid 206 responses | 2.026, 2.025 | 4 | 3 accepted | About 17.3 MB |

The first body chunk from the ordinary GET was sent about 0.79 seconds after dispatch across the cases. Removing every probe when the header is absent would lose demonstrated parallelism. NET-01 now needs matched cold and warm traces through instance startup, catalog recovery, request dispatch, and first body. Probe tuning should measure whether known origins can avoid repeat requests without treating an unadvertised origin as non-resumable. These 6.14 runs used UDM alone and support no new IDM timing claim.

### Command-line dispatch in 6.15

The desktop now posts a scheduling message after saving a silent CLI download or enabling a queue through the CLI. The message runs the existing queue policy on the desktop thread without taking a new transfer-rate sample. The periodic timer still handles later eligibility changes.

| Launch condition | Trials per version | 6.14 median to first request | 6.15 median to first request |
| --- | --- | --- | --- |
| New app process and private catalog | 5 | 710 ms | 276 ms |
| Command forwarded to a running app | 10 | 276 ms | 85 ms |

Each launch downloaded the same 32 KiB loopback payload in one GET with matching final bytes. The launch and CLI queue fixtures passed 20 checks, including paused additions, explicit queue start, native-host rejection of local-only commands, and silent completion while File Info stayed open. The comparison measures UDM command-line dispatch. Browser selection and first actionable File Info paint are separate timing gates. [Build receipt](validation/silent-queue-dispatch-6.15-build.json) · [Launch fixture](../tests/launch.native-live.cjs) · [Queue fixture](../tests/cli-queue.native-live.cjs)

### Browser queue dispatch in 6.16

The native bridge posts the same scheduling message when an accepted browser job is queued. Prepared ordinary captures stay idle until the browser commits. The existing scheduler handles queue eligibility and the existing presentation path handles confirmation, refresh and duplicate choices.

| Warm native-host operation | Trials per version | 6.15 median to first GET | 6.16 median to first GET |
| --- | --- | --- | --- |
| Direct browser admission | 6 | 315 ms | 53 ms |
| Commit a prepared browser capture | 6 | 285 ms | 24 ms |

The native host stayed connected to an isolated running desktop. Each sample saved the same 32 KiB payload in one GET and matched its bytes. Preparation created no job or HTTP request, a repeated commit returned the same job, and Download Later stayed paused with no request. This measures host-command dispatch on loopback. Browser click, worker startup, and File Info timings have different boundaries. Real Edge HLS tests passed 25 checks with File Info skipped, including complete MP4, TS, M4A and subtitle outputs. [Build receipt](validation/browser-queue-dispatch-6.16-build.json)

The native presentation fixture passed 168 checks, including dispatch without activation and no dispatch for pending confirmation, Download Later, or an uncommitted capture. A separate cold Edge HLS test kept File Info enabled. In that run the dialog was actionable 156 ms after admission while the main list stayed hidden. UDM received data before Start and displayed exactly 6,560,076 bytes. The final 720p MP4 matched that byte count and the pre-Start SHA-256, and decoded with the selected audio. This is one controlled admission-to-dialog observation. [Native presentation fixture](../native/CapturePresentationUiTests.cpp) · [Edge HLS fixture](../tests/hls-catalog.native-live.cjs)

### Keeping File Properties responsive in 7.13

File Properties previously resolved the Windows file icon and type name synchronously while creating its controls. That call can enter Shell extensions before the dialog becomes actionable. Microsoft recommends running it on a background thread to avoid blocking the interface. [SHGetFileInfoW documentation](https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shgetfileinfow)

The dialog now uses the existing `AsyncFileIcons` worker with a shared request queue and a bounded cache. It opens with the application icon and a generic file label, then applies the Windows details through its existing timer. Each dialog owns a copy of its icon. Closing while a lookup is pending leaves the worker free of window references. A move that changes the filename extension invalidates the displayed details, and late results update only the icon and type label.

All 362 Properties checks passed, including a deliberately blocked Shell resolver, an actionable visible dialog before the resolver returns, closing and reopening during the same lookup, preservation of edited fields, and cached reopening. The existing editing, authentication, scanner and reference geometry checks also passed at 96, 120, 144, 168 and 192 DPI. Visual review at 96 and 192 DPI confirmed that the Windows type label and icon appeared within the existing layout. The native build contains five executables reporting 7.13, with browser packages advanced to 0.70.77. [Build and validation receipt](validation/properties-responsive-7.13-build.json)

### Improving the download progress graphics in 7.10 and 7.11

Expanded progress keeps its connection view by default. **Show speed graph** switches that same area to a 30-second history of the transfer rate already calculated by UDM. The chart records at the existing refresh cadence, breaks the trace when the download stops receiving, and shows the selected sample's rate and age in the chart header. Hiding details and expanding them again preserves the selected view for that window.

The connection map now distinguishes downloaded coverage from connection-start markers with separate colors and shapes. Its quarter marks show scale for unfilled sections. Both displays support light, dark, and system high-contrast colors. The progress bar, status controls, and compact and expanded window heights retain their existing geometry.

All 250 native progress-dialog checks passed. The fixture verified the graph toggle and existing layouts at 96, 144, and 192 DPI. Visual checks of deterministic samples confirmed that the light and dark charts kept the hover reading above the trace and left a gap between receiving intervals. [Build and validation receipt](validation/progress-graphics-7.11-build.json)

After installation, UDM 7.11 downloaded a 64 MiB file over paced loopback HTTP in one body request. The native desktop-control helper failed to initialize, so a separate live transfer through the same 7.11 progress window code verified the graph and connection switch. The chart showed the changing rate and the selected sample in its header while the transfer was active. Both completed files matched their expected size and SHA-256. The temporary payloads and catalogs were removed. [Installation and live test receipt](validation/progress-graphics-7.11-build.json)

### Extending buffered cross-drive staging to 64 MiB in 7.9

7.9 extends the single shared range buffer from 16 MiB to 64 MiB for eligible fresh cross-drive HTTP downloads. It writes the destination staging file in chunks of up to 16 MiB, with cancellation checks between writes. Larger transfers, saved partials, jobs requiring pre-publication checksum verification, and transfers without a buffer slot retain the existing disk-copy path.

For 64 MiB files downloaded to D: with the catalog and partial files on C:, median staging fell from 100.8 ms in 7.8 to 29.8 ms in 7.9. The median visibility interval was 133–153 ms for 7.9, 202–220 ms for 7.8, and 313–325 ms for IDM 6.43.10.2. The 7.9 timing bounds were below IDM in all nine matched pairs at 4, 16, and 64 MiB. All 27 downloads matched the expected size and SHA-256. [Build and timing receipt](validation/cross-volume-64-7.9-build.json)

### Reducing cross-drive staging and cleanup in 7.7 and 7.8

The 7.6 installed-baseline rerun showed 24–73 ms staging the 16 MiB copy, followed by 426–545 ms in the combined post-publication cleanup phase. For a copy, the published file is already regular. The sparse partial on the source drive is discarded after publication. 7.7 removes normalization and writeback of that partial and retains writeback of the destination.

7.8 keeps the accepted HTTP range writes in a single shared staging buffer for eligible fresh cross-drive responses above 4 MiB and up to 16 MiB. The transfer still writes its resumable partial to disk. The range buffer supplies the destination copy and the deferred SHA-256 after publication. The file-copy path covers allocation failure, another transfer using the slot, saved partials, and jobs that require checksum verification before publication. The journal and catalog commit still precede presentation.

In the matched D: comparison with the catalog and partial files on C:, the upper bound for the 7.8 dialog timing was below the corresponding IDM lower bound in all six pairs. At 16 MiB, median staging fell from 71.2 ms in 7.6 to 6.7 ms in 7.8, and median dialog visibility moved from 202–216 ms to 149–166 ms. IDM recorded 192–209 ms. At 4 MiB, 7.8 recorded 118–133 ms, compared with 110–122 ms in 7.6 and 201–215 ms in IDM. All 18 files matched the fixture length and SHA-256. The temporary payloads and catalogs were removed. [Build and timing receipt](validation/cross-volume-cleanup-7.8-build.json)

### Staging small cross-drive downloads from memory in 7.6

With the catalog and resumable partial on C: and the target on D:, UDM must stage a copy before the journaled rename. For fresh confirmed HTTP transfers of up to 4 MiB, 7.6 can retain the bytes already hashed by the single writer and stage them in one write. The resumable partial remains available through publication. An incomplete buffer, resumed job, larger response, or transformed output uses the established file path. `StageOutput` and `CommitOutput` now distinguish the destination write from the journal, rename, and catalog commit.

In the initial three matched 4 MiB trials, the median time from the origin's final write to the observed final rename was 51 ms for 7.5, 32 ms for 7.6, and 36 ms for IDM. The 7.6 staging writes took 2.4–2.5 ms, and the durable publication steps took 22–64 ms. Windows closed the 7.5 elevation prompt before setup began, so that comparison used the verified executable staged for the installer.

After setup succeeded, the matched comparison was repeated with the installed 7.5 executable. At 4 MiB, median dialog visibility intervals were 109–138 ms for 7.5, 65–121 ms for 7.6, and 179–212 ms for IDM. At 16 MiB, they were 168–183 ms, 228–248 ms, and 161–175 ms respectively. The 16 MiB trials exercised the existing file-copy path. In the rerun, 7.6 staging took 2.2–3.0 ms at 4 MiB, while its durable publication step varied from 21 to 132 ms. One filesystem watcher callback arrived after the visible dialog, so the rerun does not establish a reliable publication-time lead. All 18 files in each comparison matched the fixture length and SHA-256. The temporary payloads and catalogs from both runs were removed. [Build and timing receipt](validation/cross-volume-stage-7.6-build.json)

### Removing the Shell lookup from completion first paint in 7.5

The completion dialog synchronously looked up the file-type icon in its paint handler. Windows documents that `SHGetFileInfoW` can block the interface and recommends calling it from a background thread. The dialog now paints with the application icon while its existing refresh timer starts and polls a shared Shell worker. File-type icons are cached by extension and copied into each dialog that uses them. [Microsoft API documentation](https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shgetfileinfow)

Publication eligibility and the dialog's display fields are read under one catalog lock. File actions initialize conservatively and refresh without waiting if the worker holds that lock. Open and drag still check current readiness when invoked. The dialog first-paint change leaves publication I/O on the worker path, which means it cannot remove time the destination volume spends committing the file. [Dialog source](../native/App.cpp)

After the test-file cleanup, three 4 MiB downloads to D: with the UDM catalog on C: produced median visibility intervals of 68–75 ms for the 7.5 build and 87–97 ms for IDM 6.43.10.2. UDM was slower on the first pair from the origin's final write. The observed final rename to dialog visibility took 46–59 ms for UDM and 74–81 ms for IDM across the three trials. Every output matched the fixture SHA-256, and the temporary files from this run were removed. [Build and timing receipt](validation/completion-first-paint-7.5-build.json)

### Using the write buffers for small HTTP downloads in 7.4

The installed 7.3 D: trace showed 16–47 ms waiting for the prefix hash thread and up to 101 ms in the final content check on the 4 MiB samples. A D: destination with the catalog and partial files on C: also had a 5.1-second publication outlier. That probe kept the two sources of delay separate.

For a fresh, confirmed HTTP transfer up to 4 MiB, 7.4 hashes the contiguous bytes from the sole writer after successful writes and retains a bounded content prefix. Completion uses the same content classifier and SHA-256 verification without reopening the partial file to read those bytes. An unexpected write offset invalidates the inline result. Saved partials, pending confirmation and larger downloads continue through the existing paths. Publication recovery and the scanner gate still apply.

The follow-up used the installed 7.3 executable and the staged 7.4 executable. Each entry is the median visibility interval from three byte-verified trials.

| Destination | Catalog and partials | Fixture | UDM 7.3 | UDM 7.4 | IDM 6.43.10.2 |
|---|---|---:|---:|---:|---:|
| D: | C: | 4 MiB | 90–97 ms | 62–74 ms | 122–137 ms |
| D: | D: | 4 MiB | 247–258 ms | 187–196 ms | 111–126 ms |
| D: | C: | 16 MiB | 258–266 ms | 181–190 ms | 128–144 ms |

The 4 MiB hash reader wait was zero across the 7.4 samples. The full content check took under 1 ms in the D: catalog run. The D: catalog placement still trailed IDM. The 16 MiB path provides a comparison control and retains its existing hashing behavior. [Timing record](validation/small-http-completion-7.4-timing.json) · [Build receipt](validation/small-http-completion-7.4-build.json)

### Showing Download Complete at file publication in 7.3

The 7.2 timing traces placed the delay after the file was committed. NTFS sparse-file normalization and writeback took 51 ms for the 4 MiB sample and 355 ms for the 16 MiB sample. The worker also computed the deferred SHA-256 before it dispatched the completion event. The dialog waited for those steps even though Windows already had the completed file at its destination.

For consolidated direct downloads, 7.3 can show Download Complete once publication and the catalog commit succeed and the file's zone and timestamp handling finish. The worker still owns the job through cleanup and hashing. Open, Open with, and drag remain gated until the worker finishes. Downloads with a configured scanner still use the scanner-aware completion path, and the final worker event still handles completion actions and sound. A dialog already shown at publication is tracked until the final event so closing it during cleanup cannot reopen it. [Build receipt](validation/early-completion-dialog-7.3-build.json)

The comparisons timed the last response body write at the loopback origin to the new window becoming visible. Each row gives the median visibility bounds from three runs per app. The 36 output files all matched the fixture sizes and SHA-256 digests.

| Destination | Fixture | UDM 7.2 | UDM 7.3 | IDM 6.43.10.2 |
|---|---:|---:|---:|---:|
| System SSD | 4 MiB | 139–167 ms | 111–122 ms | 189–211 ms |
| System SSD | 16 MiB | 264–287 ms | 109–164 ms | 216–241 ms |
| Project volume D: | 4 MiB | 695–735 ms | 1,197–1,218 ms | 194–216 ms |
| Project volume D: | 16 MiB | 534–541 ms | 229–238 ms | 181–192 ms |

On D:, 7.3 still spent up to 1,753 ms in publication before showing the dialog. The early event removes the wait after publication. The 4 MiB result on that volume still trails IDM. All 66 POST and 198 completion-dialog checks passed. The broader scanner run reproduced two failures also seen in the 7.2 baseline, in the live HTTP stop timing and the Main queue synchronization fixture. The table above uses the staged 7.3 production executable. The setup retry subsequently installed that same binary. The installed rerun is recorded separately. [Timing and verification record](validation/completion-dialog-timing-7.3.json)

### Saving the deferred hash with the final completion record in 7.2

The managed transfer records an advisory SHA-256 calculated after publication in the final worker checkpoint. The earlier save that publishes the completed file and its recovery journal still happens before the worker cleanup. Standalone transfers and media tracks retain their own hash checkpoint. The corrected test uses a POST redirected to GET, which produces a consolidated file without starting the trailing hasher. It verifies one hash-bearing checkpoint before the completion event and matches the saved digest to the finished file. All 65 POST checks, 194 completion-dialog UI checks and 225 browser recognition checks passed. [Transfer source](../native/Transfer.cpp) · [Build and validation receipt](validation/http-completion-checkpoint-7.2-build.json)

The two apps saved the same 1,048,576-byte loopback payload to D: after Start Download. A read-only observer bracketed when Windows first marked each completion dialog visible. The lower bound is the last poll without the new dialog, and the upper bound is the first poll that saw it. Both bounds are relative to the origin's last body write across the transfer's connections.

| App | Last body write to visible dialog | Output |
| --- | ---: | --- |
| UDM 7.2 | 259–287 ms | 1,048,576 bytes |
| IDM 6.43.10.2 | 321–338 ms | 1,048,576 bytes |

Both files had SHA-256 `db275f0774c66e23aa8c55f2b46f40a796009f2f7b9c4e2d741b2402cf0e1a46`. UDM used a single body request and IDM split the transfer across ranges. The result is one matched pair. It measures window visibility after the origin sent the body and does not measure first paint or establish a general performance ranking. [Build and validation receipt](validation/http-completion-checkpoint-7.2-build.json)

#### Installed 7.2 completion timing

The 7.2 setup exited successfully. All four installed runtime executables and 95 browser package files matched the staged build. The installed native host reported 7.2, Chrome and Firefox were registered to the installed manifests, and the existing user catalog kept its exact SHA-256. Setup used the packaged 7.2 setup helper from its temporary extraction directory. [Build and validation receipt](validation/http-completion-checkpoint-7.2-build.json)

| File size | Installed UDM 7.2 | IDM 6.43.10.2 |
| --- | ---: | ---: |
| 256 KiB | 531–557 ms | 198–232 ms |
| 4 MiB | 436–473 ms | 262–335 ms |
| 16 MiB | 543–552 ms | 304–331 ms |

Both apps saved identical bytes at every tested size. Windows denied desktop input partway through the initial run, so the 4 MiB and 16 MiB IDM transfers were started through the vendor's COM API with confirmation prompts suppressed. Both displayed the actual completion dialog. The 16 MiB UDM sample used an individual download queued in a private catalog and exercised the installed app's normal completion path. The timing starts at the last body write, after the different start paths. Each size has one sample per app. [Installation and timing record](validation/installed-completion-timing-7.2.json) · [IDM API documentation](https://www.internetdownloadmanager.com/support/idm_api.html)

### Starting a fresh HTTP response without another catalog write in 7.1

A new HTTP download whose parts folder is empty now starts writing the first response using the already-saved download record. The first periodic or Pause checkpoint records the new range plan and received bytes. If the app exits before either checkpoint, the original record still identifies the download and recovery starts at byte zero. The earlier plan commit still applies to resumes, partial-file resets, and legacy part migration. [HTTP transfer startup](../native/Transfer.cpp)

The loopback regression counts catalog flushes before the first body bytes and after a delayed helper joins. It also reopens the original catalog alongside the interrupted parts and checks the recovered file against the origin's SHA-256. [Native transfer fixture](../native/TransferChecks.hpp)

All 178 transfer checks and 15 first-response checks passed. With the same fixture linked against the 7.0 transfer object, all four startup cases detected an extra catalog flush. Both 0.70.65 browser bundles passed 225 recognition checks. [First-response fixture](../tests/initial-response.native-live.cjs) · [Build receipt](validation/http-first-body-7.1-build.json)

Eight paired 256 KiB loopback trials alternated the staged 7.0 and 7.1 desktops, each with a fresh private catalog. The median from HTTP transfer entry to the first file write fell from 102 to 16 ms. Median launch-to-completion was 610 ms on 7.0 and 607 ms on 7.1. All 16 downloads used one GET and matched the expected SHA-256. The timings describe the local startup path and varied between runs. [Build and validation receipt](validation/http-first-body-7.1-build.json)

### Keeping active HTTP connections moving during range admission in 7.0

Periodic catalog writes already serialize and flush outside the manager lock. The deferred midpoint range still forced a synchronous catalog write under that lock, both when it divided the remaining bytes and when it first proved range support after the primary worker passed the midpoint. Both paths now update the live plan and leave persistence to the periodic or Pause checkpoint. The old on-disk plan covers the same offsets as a single range. Overlap validation, the initial plan commit, and the transaction for resetting or migrating saved parts still run at their existing boundaries. [HTTP range admission](../native/Transfer.cpp) · [Checkpoint worker](../native/PeriodicCheckpoint.hpp)

The loopback fixture delays the midpoint response until the primary has received bytes. It records any catalog flush after the helper is released and while the file is still receiving. The four cases cover advertised and unadvertised range support with the helper arriving before or after the midpoint. Each checks the connection sequence, range redistribution, and the completed file's SHA-256. Linking the fixture with the 6.99 transfer object reproduced a flush in three cases. The 7.0 object eliminated all three. An older recovery assertion was updated to expect the original unsplit plan with range support still unverified. Reopening that earlier checkpoint still recovered the exact file. [Native transfer fixture](../native/TransferChecks.hpp) · [Production build receipt](validation/http-helper-checkpoint-7.0-build.json)

The revised transfer suite passed all 174 checks, including forced-stop recovery, pause and reopen, validator mismatch, connection caps, and reconnection after drops. The browser recognition suite passed 225 checks with both extension manifests at 0.70.64. [Production build receipt](validation/http-helper-checkpoint-7.0-build.json)

### Hashing adaptive parts during writes in 6.98

UDM now builds the completed segment's SHA-256 as each write succeeds and commits that digest with the receipt. A resumed part seeds the hash with its verified saved prefix. Cache reuse still verifies the saved bytes, and publication still verifies the final output. This change removes a second read of each newly completed part. It was built into 6.99. [Production build receipt](validation/automatic-download-notice-6.99-build.json)

| Build and browser | Selected format | Selection to actionable | Selection to exact size | Exact bytes shown and saved |
| --- | --- | ---: | ---: | ---: |
| 6.97, isolated Edge | 360p TS | 849 ms | 2,779 ms | 1,961,216 |
| 6.97, isolated Edge | Audio-only M4A | 980 ms | 5,255 ms | 98,791 |
| 6.99, isolated Chrome | 1080p MP4 | 1,132 ms | 2,456 ms | 13,196,381 |

Each controlled loopback run matched the completed file's SHA-256 to the verified output before Start. The audio-only run requested no video segments. In the 1080p run, the first segment started 1,308 ms after selection, the main list stayed hidden, and both video and audio decoded completely. These timings describe separate formats and browser runs. [HLS fixture](../tests/hls-catalog.native-live.cjs) · [Native dialog observer](../tests/observe-cold-file-info.py)

### Keeping single popup handoffs quiet in 6.97

The browser popup no longer shows an intermediate sending or preparing message for a single link or video. It keeps the final result and errors, ignores repeat clicks on a link while its handoff is pending, and allows retry after failure. A selected video stays disabled while awaiting the result. The Chromium and Firefox popup fixtures covered pending, duplicate-click, retry and success behavior. [Popup fixture](../tests/popup-download-feedback.test.cjs)

An isolated Edge profile used the staged 6.97 desktop and browser 0.70.61. The fixture delayed delivery of the real native acknowledgement and double-clicked Download with UDM. The popup stayed quiet during the hold, sent one native add, then displayed the acknowledgement and completed the exact 4,097-byte file. Browser recognition passed 225 checks and the handshake fixture passed seven. [Build receipt](validation/quiet-popup-handoff-6.97-build.json)

### Refreshing the exact size during media assembly in 6.96

Download File Info now refreshes every 80 ms during final assembly or verification of an active media prefetch. It restores the normal 250 ms cadence when the verified exact size becomes available or prefetch stops. The assembled and verified size labels still represent separate states. [Windows timer behavior](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-settimer)

The staged production build passed all seven cold Edge HLS checks and 225 browser recognition checks. Start was actionable at 1,018 ms. The 720p MP4 displayed the exact 6,560,076 bytes at 2,435 ms, 491 ms after the last segment finished. The final file matched the pre-Start SHA-256 and decoded with the selected audio for the full clip. The matching 6.95 C: run recorded 554 ms from the last segment to exact size and 1,308 ms from selection to exact size. These single runs show that end-to-end timing varied. The planned same-volume comparison on D: could not finish because the 6.96 trial ran out of space. [Cold Edge fixture](../tests/hls-catalog.native-live.cjs) · [Build receipt](validation/file-info-size-pulse-6.96-build.json)

### Keeping background relaunches hidden in 6.95

A second `--background` launch with no download or queue command used to send Show to the existing UDM instance. The duplicate launcher now exits quietly. Explicit foreground launches still show the existing window, and launches with download or queue commands still forward them. The staged 6.94 desktop reproduced the window appearance. All three 6.95 relaunch runs kept the original desktop hidden until an explicit foreground launch. Eight live launch checks, seven browser handshake checks, and 225 browser recognition checks passed. [Relaunch fixture](../tests/browser-startup.native-live.cjs) · [Build receipt](validation/background-relaunch-6.95-build.json)

An isolated Edge session clicked the 720p HLS MP4 choice using browser 0.70.59 and the 6.95 native presentation fixture. File Info had an enabled Start button 200 ms after the click and the main list stayed hidden. The first received bytes were observed at 440 ms and the exact size appeared at 906 ms. The displayed 6,560,076 bytes and verified staging SHA-256 matched the final file. The file decoded across the full eight-second timeline with the selected audio track. The test used synthetic HLS and a running, hidden desktop fixture. [Edge media fixture](../tests/hls-catalog.native-live.cjs)

The same media fixture now supports a closed production desktop through `UDM_TEST_COLD_DESKTOP=1`. A read-only native probe confirmed UDM was closed after the Edge quality and audio menus opened. The actual selection started the staged 6.95 executable. In one cold run, File Info became actionable after 557 ms, the first segment request arrived after 619 ms, and the exact size appeared after 1,308 ms. The main download list stayed hidden. The last segment finished 554 ms before the exact size was observed, an interval that includes assembly, verification and dialog refresh. Before clicking Start, the observer read 6,560,076 bytes from the dialog and hashed the staged MP4. The final MP4 matched the size and hash, decoded for eight seconds with the selected audio, and all seven Edge checks passed. The temporary browser host registration was removed. [Cold fixture](../tests/hls-catalog.native-live.cjs) · [Windows observer](../tests/observe-cold-file-info.py)

### Starting the desktop from a cold browser handoff in 6.94

The native host now checks the desktop's instance mutex when the browser pipe is absent. It starts a closed desktop without the old one-second prelaunch wait, waits for a running desktop's pipe, and uses a separate per-instance launcher mutex to coordinate simultaneous requests. Native commands that require an already running desktop still return without launching one. Once a request has been written to the pipe, a failed acknowledgement still does not trigger replay.

The 6.93 baseline reproduced an unwanted main-window appearance with two simultaneous cold browser offers. Both hosts could launch a desktop and the second background desktop forwarded a show command to the first. The 6.94 fixture exercises this path with a private catalog and two Download Later offers. [Native host fixture](../tests/browser-startup.native-live.cjs)

In three concurrent 6.94 runs, both offers were recorded exactly once and one hidden desktop remained. In five alternating single-request trials per build, median time from host invocation to a saved offer fell from 1,528 ms to 395 ms. A read-only preference request against a closed desktop fell from 1,089 ms to 74 ms and kept UDM closed. Warm diagnostics held at 42 ms. These timings cover native admission of paused offers. They do not measure a video click or File Info paint.

The native production build, host handshake, eight live launch checks, seven browser handshake checks, and 225 browser recognition checks passed. [Live launch fixture](../tests/launch.native-live.cjs) · [Build receipt](validation/cold-browser-startup-6.94-build.json)

### Committing command-line admission once in 6.93

An ordinary command-line add previously saved the initial job, duplicate decision, and final queue or confirmation status separately. The command-line path now defers those intermediate writes and saves the final record once. Automatic overwrite keeps its existing restart journal because it can retire a previous generation of partial files.

The 44-check launch fixture verified that the catalog backup still holds the pre-admission state, that a failed write removes the staged job or pending prompt, and that the hang-up action is armed only after a successful retry. Both browser bundles passed 225 recognition checks. The production build and host handshake passed. [Launch fixture](../native/LaunchQueueTests.cpp) · [Build receipt](validation/atomic-cli-admission-6.93-build.json)

Ten paired 256 KiB loopback trials alternated cold 6.92 and 6.93 processes with private catalogs. Median launch to first body write went from 409 to 370 ms, and median completion went from 506 to 486 ms. All 20 files matched the expected size and SHA-256 with one GET each. Timing varied across trials. The installed IDM 6.43.10.2 process was making connection attempts in the current profile, so these runs used UDM only. [Benchmark harness](../benchmarks/speed/README.md)

### Adapting parallel reads to slow connections in 6.92

IDM documents letting a free connection help a slow connection by taking half of a remaining range. UDM already splits that way. Its mixed-speed trace showed a slow worker could still be waiting on a large read issued before its range changed. UDM now sizes each parallel read using that response's recent delivery time. It starts at 32 KiB, targets about 125 ms of observed delivery, and stays between 16 KiB and the original 256 KiB ceiling before applying the existing range and limiter caps. [IDM segmentation and connection reuse](https://www.internetdownloadmanager.com/support/segmentation.html) · [WinHTTP read semantics](https://learn.microsoft.com/en-us/windows/win32/api/winhttp/nf-winhttp-winhttpreaddata)

The native transfer fixture passed 170 checks, including dynamic splitting, connection reuse, exact SHA-256, checkpoint and outage recovery, and origins that refuse additional connections. The production host handshake reported 6.92. Browser recognition passed 225 checks. [Transfer fixture](../native/TransferChecks.hpp) · [Build receipt](validation/adaptive-range-read-6.92-build.json)

Paired loopback runs alternated 6.91 and 6.92 with eight requested connections and separate test catalogs. All 22 files matched the expected size and SHA-256.

| Workload | Runs per build | Median wall time, 6.91 → 6.92 | Median first-to-last body write, 6.91 → 6.92 | Median excess origin bytes, 6.91 → 6.92 |
| --- | ---: | ---: | ---: | ---: |
| `mixed-speed-32m` | 5 | 6.050 → 3.555 s | 5.316 → 2.904 s | 971,822 → 410,537 |
| `alternating-drop-50m` | 3 | 2.597 → 2.512 s | 1.991 → 1.734 s | 1,245,899 → 420,144 |
| Uncapped 64 MiB | 3 | 1.312 → 1.200 s | 0.235 → 0.241 s | 4,128,794 → 3,814,037 |

The mixed-speed case took 41% less wall time at the median and made the same median of 22 requests. The uncapped body timing was close on this sample. These are UDM-to-UDM loopback measurements. [Benchmark harness](../benchmarks/speed/README.md)

### Protecting replacement backups in 6.91

When a duplicate replaces a completed download, UDM chooses a numbered previous-version filename in the same folder. It now checks normalized catalog destinations and previous-version reservations when choosing that filename. It checks again before the filesystem swap so a conflicting reservation already stored in an older catalog leaves the original file and staged replacement available. [Windows path normalization](https://learn.microsoft.com/en-us/dotnet/standard/io/file-path-formats#path-normalization)

The native overwrite fixture passed 87 checks. It skipped two aliased reservations during numbering, blocked a swap when another reservation claimed the chosen backup name, and successfully published after the conflict was cleared. It also covered rollback after a failed catalog write and recovery after interruption. Both browser bundles passed 225 recognition checks. [Duplicate fixture](../native/DuplicateChecks.hpp) · [Build receipt](validation/duplicate-backup-reservation-6.91-build.json)

### Protecting destination reservations in 6.90

Download File Info checks the normalized destination against other downloads and their reserved previous-version paths. A Save As draft can refer to the same Windows path using a different separator, letter case, or a parent directory segment. Use server name and final confirmation now check the same catalog reservations. New downloads also use the normalized path when choosing a numbered filename. These checks cover catalog reservations before any output exists. [Windows path normalization](https://learn.microsoft.com/en-us/dotnet/standard/io/file-path-formats#path-normalization)

The Windows File Info fixture passed 622 checks. It verified automatic numbering, refusal of a server suggestion and a manually entered path, protection for previous-version paths, and refusal of an aliased destination while a real prefetch was awaiting Start. The native build and host handshake passed, and the two browser bundles passed 225 recognition checks. This checkpoint used an isolated UDM fixture. [File Info fixture](../native/FilenamePreviewUiTests.cpp) · [Build receipt](validation/file-info-destination-alias-6.90-build.json)

### Confirming the displayed server filename in 6.89

The File Info button rechecks the available server filename when clicked. If a GET response replaces the preview name before the label refreshes, the first click now refreshes the suggestion and keeps the Save As draft. The next explicit click can use the newly displayed name. The check uses the same response and preview eligibility as the label. The chosen name remains a draft until confirmation. [HTTP filename suggestions](https://www.rfc-editor.org/rfc/rfc6266.html#section-4.3)

The Windows fixture passed 592 checks. A controlled response-metadata update for the same URL changed the suggested filename between display and click. The old click kept the typed name and folder. After refresh, the new click adopted the displayed name while leaving the catalog untouched until confirmation. Browser recognition passed 225 checks. [File Info fixture](../native/FilenamePreviewUiTests.cpp)

### Keeping the filename when choosing a recent folder in 6.88

The editable Save As control used to build each history entry from the filename present when the dialog opened. Choosing a recent folder could therefore overwrite a later filename edit. The history entries now use the current draft name. Choosing a folder updates the destination and remembered category folder together, including when Windows sends the selection notification before updating the edit text. The server filename remains a separate suggestion until explicitly adopted. [IDM starting downloads](https://www.internetdownloadmanager.com/support/using_idm/starting.html) · [Win32 combo boxes](https://learn.microsoft.com/en-us/windows/win32/controls/about-combo-boxes)

The first implementation rebuilt the entire combo box and would have cleared native Undo. The final implementation replaces only the history list entries and preserves the edit control's text and caret. The Windows fixture passed 590 checks, including native Undo, a typed Unicode name, a stale selection notification, and the saved Download Later destination and category folder. Browser recognition passed 225 checks. [File Info fixture](../native/FilenamePreviewUiTests.cpp) · [Windows combo reset behavior](https://learn.microsoft.com/en-us/windows/win32/controls/cb-resetcontent)

### Checking main-list file readiness at drag start in 6.87

IDM documents dragging the saved file from Download Complete. UDM already checks that dialog's drag against the current scan and relocation state. The main list now uses the same readiness rule when building a file drag. UDM keeps ready paths in selection order and uses each file's current destination after a move. [IDM Download Complete](https://www.internetdownloadmanager.com/support/using_idm/completeD.html)

The Windows fixture passed 194 checks. It decoded the file-drop payload for an idle selection, mixed ready and scanning files, completed scans, and paths after Move/Rename. A pending scan prevented the drag before the next list refresh. A paused record stayed out of the payload even when its path existed. This run checked payloads and selection policy through the main-list helper. Browser recognition passed 225 checks. [Completion fixture](../native/CompletionDialogUiTests.cpp)

### Activating the focused completion action with Enter in 6.86

The retained IDM completion resource uses standard push buttons for Open, Open with, Open folder and Close. The UDM 6.85 fixture reproduced a dark-mode gap. Enter worked on the focused Open button in light mode and failed to invoke it in dark mode. UDM now routes Enter from the dialog's default command to the focused action when there is one. The action then goes through the same button callback and saved-file checks as a mouse click. The live reference keyboard comparison remains open because Windows denied activation of the captured IDM window. [IDM completion resource](reference/completion-dialog.json) · [Windows dialog keyboard handling](https://learn.microsoft.com/en-us/windows/win32/dlgbox/dlgbox-programming-considerations)

The 6.86 fixture passed 189 checks. Open and Open with reached the missing-file warning in both themes, leaving the dialog open. Enter on Close dismissed the dialog in both themes. This exercised Windows dialog key translation with a local test file. Browser recognition passed 225 checks. [Completion fixture](../native/CompletionDialogUiTests.cpp)

### Applying completion suppression while the dialog is open in 6.85

IDM documents turning off automatic completion dialogs by checking **Don't show this dialog again**. UDM now commits the setting when the checkbox changes, so a later download cannot open another completion while the checked dialog is still open. Existing completion windows reflect the shared setting on refresh. Unchecking restores automatic dialogs. A failed settings write restores the checkbox and leaves the current window open. Explicit completion requests can still show the saved file. [IDM Download Complete](https://www.internetdownloadmanager.com/support/using_idm/completeD.html)

The Windows fixture passed 183 checks, including a second automatic completion while the first window remained open, persistence before closing, synchronization across two open dialogs, reversal, and rollback on a catalog write failure. The initial test missed a simulated click in an inactive dialog. Activating the dialog before sending the click corrected the fixture. Browser recognition passed 225 checks. [Completion fixture](../native/CompletionDialogUiTests.cpp) · [Win32 button click behavior](https://learn.microsoft.com/en-us/windows/win32/controls/bm-click) · [Build receipt](validation/completion-suppression-6.85-build.json)

### Reusing the first legacy continuation in 6.84

Older catalogs can leave received bytes in numbered parts while the recorded byte count is zero. When the selected range has a partial numbered file and a strong saved validator, UDM now requests its continuation with overlap as the first request. The response still has to prove the selected range and identity. UDM copies the part into the consolidated file and saves the new offsets before removing the old part or letting a worker append the continuation. If migration changes the offset, the response is discarded. [HTTP If-Range](https://www.rfc-editor.org/rfc/rfc9110.html#section-13.1.5)

The first fixture released the catalog write lock before shutdown, which allowed shutdown to save the migrated offsets. The corrected fixture held the lock until the manager was destroyed and verified that reopening still saw the original catalog. The next attempt made one request starting 64 bytes before the end of the numbered part, preserved the final SHA-256, and removed the old part after commit. The full native transfer suite passed 170 checks, and browser recognition passed 225. [Transfer fixture](../native/TransferChecks.hpp) · [Build receipt](validation/legacy-resume-request-6.84-build.json)

### Choosing the first resume offset from saved bytes in 6.83

A sparse file may still have its original length after a disk allocation is lost. UDM now checks the chosen saved segment against the local file length and allocation map before opening the first continuation. It requests overlap immediately before the bytes that are actually present. The transfer still validates the server response and rechecks the saved plan before appending. Legacy numbered parts keep their migration path.

The full 6.83 transfer suite passed 165 checks. The sparse fixture recorded exactly one request, starting 64 bytes before the first hole, and the final SHA-256 matched. The same run covered the 6.82 refusal cases, legacy migration, dynamic ranges, connection drops and server connection limits. Browser recognition passed 225 checks. [Transfer fixture](../native/TransferChecks.hpp) · [Build receipt](validation/sparse-resume-offset-6.83-build.json)

### Recovering a refused saved range in 6.82

IDM documents starting from the beginning when an origin cannot resume. UDM now handles the same plain range refusals on reopen that it handles during an interrupted single-connection transfer. HTTP 403, 405 and 501 without a retry delay switch ordinary downloads to a full GET under the existing retry policy. The old checkpoint remains intact until that response is accepted. A pending address refresh or strict identity policy still requires validation of the existing bytes. Browser challenges and refusals carrying a retry delay continue through their existing error paths. [IDM resume FAQ](https://source.internetdownloadmanager.com/register/new_faq/sites2_6.html)

The isolated loopback fixture passed 56 saved-ETag checks and 28 strong-date checks. Reopening a saved ETag partial against each of the three plain refusals made one range request and one full GET. Each final SHA-256 matched. Retry-After, a marked browser challenge and a strict refresh stopped after the refused range. A permanent refusal of the subsequent full GET also left the old file and saved byte count intact. Browser recognition passed 225 checks. [Transfer fixture](../native/TransferChecks.hpp)

The broader 6.82 transfer run passed 164 checks and failed one request-offset assertion. The sparse-hole fixture published the correct final hash, but its first range started at the catalog offset beyond the hole. This led to the local allocation check in 6.83.

### Keeping progress commands consistent with their labels in 6.81

IDM documents separate Pause, Resume and Cancel actions in the download progress window. UDM now uses a shared action decision for the button label and its command. When queue or worker state changes the available action before the next refresh, the command refreshes the window and leaves the download alone. Pause and Stop wait check and act under the same catalog lock. Dismissal still stops an active download when the window shows Cancel. A dismissal while the window still shows Close leaves newly queued work in the queue. [IDM download controls](https://www.internetdownloadmanager.com/support/using_idm/using_idm.html)

The native fixture passed 245 checks. A queued transition between displaying Start and clicking it kept the job queued. A queue stop between displaying Pause and pressing Enter kept the job paused. After a refresh, the same Start and Pause commands still worked. Close preserved a job queued since the last refresh. The scanner, form restart and progress geometry checks passed in the same run. Both browser bundles passed 225 recognition checks in total. [Progress fixture](../native/ProgressDialogUiTests.cpp)

### Offering Resume for a saved partial in 6.80

IDM documents Resume as the action for restarting an interrupted download and reports the server’s resume capability separately. UDM now offers Resume on paused or failed partials while that capability is still unknown. The existing transfer engine validates the continuation before reusing bytes. An unstarted job or a known non-resumable transfer still offers Start. [IDM download controls](https://www.internetdownloadmanager.com/support/using_idm/using_idm.html)

The isolated Windows progress-dialog harness passed 239 checks. It verified Resume for paused and failed partials, Unknown in the capability field, and Start for an unstarted or confirmed unsupported transfer. The unchanged ETag transfer engine passed 25 loopback checks covering pause, reopen, first continuation, safe restart and the final hash. Both browser bundles passed 225 recognition checks in total. [Progress fixture](../native/ProgressDialogUiTests.cpp) · [Transfer fixture](../native/TransferChecks.hpp)

### Resuming an ETag-validated partial in 6.78

After reopening a partially downloaded HTTP file, UDM can send its first continuation with the saved strong ETag in If-Range even when the first GET did not establish range support. It checks the response interval, total length, ETag and overlapping bytes before keeping the partial data. A rejected ordinary continuation falls back to a full response, and a full response to the continuation can be used immediately. The protected replacement path still enforces its existing identity checks. [HTTP If-Range](https://www.rfc-editor.org/rfc/rfc9110.html#section-13.1.5)

The loopback fixture paused and reopened an actual partial in five cases. All 25 ETag checks passed, including exact final hashes, two total requests on a matching or ignored range, and clean restarts for changed or weak ETags and bad overlap bytes. All 28 strong-date checks passed on the same native build. [Transfer fixture](../native/TransferChecks.hpp)

### Keeping the Properties Open action current in 6.77

IDM documents opening the saved file directly from File Properties. UDM now uses one readiness check across File Properties, the main list, Download Complete and the progress window. File Properties refreshes Open while it is visible and rechecks the current job and saved path when the command runs. [IDM File Properties](https://www.internetdownloadmanager.com/support/using_idm/properties.html)

The isolated Properties fixture verified that a pending scan disables Open, a completed scan enables it in the same window, and a command already posted before a new scan starts does not use the saved path. The Properties fixture passed 351 checks. The progress fixture passed 237, including an inert scanner that continued to completion after UDM stopped monitoring it. Browser recognition passed 225 checks. [Properties fixture](../native/PropertiesDialogUiTests.cpp) · [Progress fixture](../native/ProgressDialogUiTests.cpp) · [Production build receipt](validation/properties-open-readiness-6.77-build.json)

### Activating a download row with Enter in 6.76

The main list handles the native Enter notification for its focused, selected row. It uses the same command as double-click. Completed downloads respect the Open or Properties preference, and an unfinished download opens its progress window. A focused but unselected row does not activate. [Windows list-view notification](https://learn.microsoft.com/en-us/windows/win32/controls/nm-return-list-view-)

The native fixture dispatched the list-view notification for both completed-file preferences, a focused row without selection, and a paused download. The first Properties test left a completed record in an ordinary download queue and hit catalog validation. After the fixture used a valid completed record, all 176 native checks passed. Browser recognition passed 225 checks. [Native fixture](../native/CompletionDialogUiTests.cpp) · [Production build receipt](validation/list-keyboard-activation-6.76-build.json)

### Keeping completion file actions current in 6.75

IDM documents Open and file drag from Download Complete. UDM now applies the same readiness rule in its completion dialog that it uses for the main list. Open, Open with and both drag handles become unavailable while a scan is pending or the file is being moved. Each action checks the current state and saved path again at invocation. Open folder and Close remain available during a scan. [IDM Download Complete](https://www.internetdownloadmanager.com/support/using_idm/completeD.html)

The isolated native fixture started a simulated scan before the dialog refreshed. Both queued button commands stayed in the dialog and both drag controls returned an empty path. When the scan state cleared, the same dialog re-enabled the actions and the drag controls returned the current Unicode destination. The existing move and missing-file checks passed as part of 170 native checks. Browser recognition passed 225 checks. [Native fixture](../native/CompletionDialogUiTests.cpp) · [Production build receipt](validation/completion-action-state-6.75-build.json)

### Checking main-list file actions during a completion scan in 6.74

The main download list uses the progress window's scanner readiness check for **Open** and **Open with**. Command invocation rechecks readiness and the saved path before handing the file to Windows. Both actions display the moved-file warning if a directory replaces the saved file. IDM documents optional antivirus scanning after completion. [IDM virus-checking options](https://www.internetdownloadmanager.com/support/using_idm/options.html#downloads)

The first fixture kept the main list hidden, which defers row population. After showing the fixture window without activating it, the native run passed 164 checks. The main-list cases verified that pending scans disable both file commands, that Open folder remains available, and that both file commands reject a substituted directory once the job is idle. Browser recognition passed 225 checks. [Native fixture](../native/CompletionDialogUiTests.cpp) · [Production build receipt](validation/main-list-open-actions-6.74-build.json)

### Keeping progress actions consistent during a completion scan in 6.73

IDM documents running a configured antivirus after downloading. UDM now separates the progress window's actions for an active scanner, a busy completed job, and an idle completed job. **Stop wait** only stops monitoring an active scanner. **Open** and **Open with** are unavailable while scanning is pending or the completed job is still active. The primary action checks whether the current state still matches the button the user saw. File actions check that the saved path is a file before asking Windows to open it. [IDM virus-checking options](https://www.internetdownloadmanager.com/support/using_idm/options.html#downloads)

The native fixture used an inert scanner process. It checked that the default keyboard command could not open a file with a pending scan and that **Stop wait** recorded an interruption, preserved the saved bytes, and allowed the scanner process to finish. It also exercised the busy completed-job action state. All 237 native and 225 browser recognition checks passed. [Native fixture](../native/ProgressDialogUiTests.cpp) · [Production build receipt](validation/progress-scanner-actions-6.73-build.json)

### Closing the progress window without changing an idle download in 6.72

IDM documents Cancel as stopping the download and closing its progress window. UDM now uses Cancel for active or queued transfers and Close when dismissal needs no pause. Title-bar Close, the Escape command, and the button all check the current state under the same lock. Closing a completed download leaves its scanner wait alone. [IDM progress controls](https://www.internetdownloadmanager.com/support/using_idm/using_idm.html)

The first fixture exposed that sending a title-bar close message did not destroy the modeless window. The corrected build passed 228 native checks, including title-bar Close on a paused record, the Escape command on a failed record, Close on a completed record, and Cancel on a queued record. The three close cases preserved their catalog hashes. Browser recognition passed 225 checks. [Native fixture](../native/ProgressDialogUiTests.cpp) · [Production build receipt](validation/progress-dismissal-6.72-r2-build.json)

### Showing the saved size of a replacement with a prefix in 6.71

A direct media replacement with no saved bytes can use the existing bounded prefix check on its first response. When the response length is known and the payload matches, File Info reports the saved size while the body is still arriving. A response without a length reaches the same exact size at clean EOF. Replacement transfers with saved bytes retain the existing range and overlap validation. [HTTP Content-Length](https://www.rfc-editor.org/rfc/rfc9110.html#section-8.6)

The loopback fixture served a 524,349-byte response containing 60 bytes of leading HTML before the M4A payload. File Info displayed **524,289 bytes** before Start. With a declared length it did so while the body was still arriving. With no declared length it waited for EOF. Each replacement used one GET and saved the 524,289-byte payload byte for byte. The native fixture passed 588 checks and browser recognition passed 225. [Request assertions](../tests/filename-preview-ui.cjs) · [Production build receipt](validation/replacement-prefix-size-6.71-build.json)

### Showing the replacement response size before EOF in 6.70

File Info keeps the previous size hidden until the active HTTP replacement supplies its own accepted headers. It labels the declared length **Response size** while the transfer still needs to validate the body. A replacement without a declared length shows **Final size pending** until clean EOF establishes the size. IDM documents background downloading while File Info is open. [IDM Downloads options](https://www.internetdownloadmanager.com/support/using_idm/options.html#downloads) · [HTTP Content-Length](https://www.rfc-editor.org/rfc/rfc9110.html#section-8.6)

The one-use origin withheld the body after declaring 4,097 bytes. The fixture saw that number with zero bytes received, clicked Start, and verified that the same GET published all 4,097 bytes. The chunked replacement showed **Final size pending**, then **File size: 4,097 bytes** before Start. It also published matching bytes from one GET. The native fixture passed 572 checks and browser recognition passed 225. [Request assertions](../tests/filename-preview-ui.cjs) · [Production build receipt](validation/replacement-response-size-6.70-build.json)

### Preserving the only request to a replacement link in 6.69

File Info reserves an unvalidated HTTP replacement URL for the transfer. It skips automatic and manual metadata requests and hides remote ZIP Preview while the link is waiting. Once prefetch is active, ZIP Preview can read the locally staged response through the existing receipt check. With prefetch off, the size remains pending until the user starts the download. This protects the temporary URLs documented in IDM's expired-link workflow. [IDM expired-link workflow](https://www.internetdownloadmanager.com/register/new_faq/sites2_3.html)

The owned origin expires a replacement after its first request, including HEAD. Both short and long slot waits used exactly one GET, and File Info showed 4,097 bytes before Start. Confirmation published matching bytes without another request. With prefetch off, File Info waited for Start and then used one GET to publish the complete response. The native fixture initially assumed Advanced was collapsed in a dialog with a saved login. After correcting that assumption, it passed 545 checks. Browser recognition passed 225 checks. [Request assertions](../tests/filename-preview-ui.cjs) · [Production build receipt](validation/protect-replacement-request-6.69-build.json)

### Reusing the first GET from a one-use replacement link in 6.68

IDM documents that some temporary download addresses can be requested only once. UDM now checks for saved bytes before opening a refreshed HTTP address. If the transfer has no saved bytes, it opens a full GET and keeps its body. An uninterrupted response uses one GET even when the origin advertises byte ranges and supplies an ETag. A replacement with saved bytes still validates the range and entity before appending. [IDM expired-link workflow](https://www.internetdownloadmanager.com/register/new_faq/sites2_3.html)

The controlled origin honors byte-zero probes and expires the link after the first GET. All three replacement cases sent one full GET, displayed 4,097 bytes before Start, and published byte-identical files. The prolonged slot wait also sent the existing metadata HEAD, which this fixture permits without expiring the link. Saved partial cases exercised matching ETags, matching modification dates, and rejection of changed identity or overlap. The native suite passed 560 checks and browser recognition passed 225. [Request assertions](../tests/filename-preview-ui.cjs) · [Production build receipt](validation/one-use-replacement-get-6.68-build.json)

### Avoiding a metadata request after refreshing an expired link in 6.67

IDM exposes Refresh download address for expired links. UDM now applies the same prefetch admission and lookup timing to a replacement saved inside File Info as it does to a newly opened link. The wait is one second when all slots are occupied and the replacement has no validated size. [IDM expired-link workflow](https://www.internetdownloadmanager.com/register/new_faq/sites2_3.html)

In the controlled HTTP 410 tests, the brief wait produced one replacement GET. The longer wait produced a HEAD followed by one replacement GET. Both kept the selected destination, removed the previous origin's login from the replacement request, and displayed 4,097 bytes before Start. The origin returned its full body in response to the initial range request. Start reused those bytes and the published files matched the fixture. The first full run reached the 90-second harness limit after the replacement checks. The same native binary passed 521 checks when rerun with a 150-second allowance. Browser file recognition passed 225 checks. [Native dialog fixture](../native/FilenamePreviewUiTests.cpp) · [Request assertions](../tests/filename-preview-ui.cjs) · [Production build receipt](validation/file-info-refreshed-slot-wait-6.67-build.json)

### Avoiding a metadata request during a short File Info wait in 6.66

When the transfer limit is full and File Info has no size hint, UDM retries prefetch admission before sending a metadata request. After one second it can send the existing HEAD lookup if the slot is still occupied. File Info remains actionable throughout. IDM documents starting a background transfer while File Info is open. The timing threshold is a UDM choice. [IDM Downloads options](https://www.internetdownloadmanager.com/support/using_idm/options.html#downloads)

The controlled origin recorded one GET and no HEAD when the fixture freed the only transfer slot after a short wait. File Info showed 4,097 bytes before Start and the published file matched all 4,097 response bytes. Cancel during the short wait made no request for the selected file. A longer wait still used HEAD followed by GET, with the GET replacing the provisional size. The native suite passed 518 checks and the browser recognition suite passed 225. [Fixture and request assertions](../tests/filename-preview-ui.cjs) · [Production build receipt](validation/file-info-short-slot-wait-6.66-build.json)

### Checking the completed file before opening it in 6.64

Download Complete checks that the saved path points to a regular file before either **Open** or **Open with**. If the file has been moved or a directory now occupies that path, the dialog shows the existing moved-file message and stays open. IDM documents both file actions in its completion dialog. [IDM Download Complete reference](https://www.internetdownloadmanager.com/support/using_idm/completeD.html)

The native completion fixture passed 157 checks with the change included in 6.65. Both file actions showed the moved-file message and left the substituted directory untouched. The fixture also verified that each action keeps Download Complete open after the file is moved. [Native fixture](../native/CompletionDialogUiTests.cpp) · [Production build receipt](validation/manual-link-account-scope-6.65-build.json)

### Measuring the opening prefetch rate in 6.63

The prefetch sampler combines short intervals until it has at least 100 ms of elapsed time. A timer tick without any new bytes keeps the sampling window open. Start or the end of network activity can close a shorter window and use its average when no earlier peak is available. Once a peak exists, a short final slice cannot replace it. Start persists the measured rate with the prefetched byte count. If the save fails, the prefetch keeps its previous sampling window. The progress window shows the opening rate with its before Start label and then returns to the live rate.

The native progress fixture passed 214 checks, including deterministic short and idle timing windows and the opening-to-live rate transition. The File Info suite passed 490 checks. Its real loopback Start cases retained the prefetched byte count and measured rate while publishing matching output from the original GET. The Chromium and Firefox recognition suite passed 225 checks. [Production build receipt](validation/prefetch-rate-window-6.63-build.json)

### Reusing complete browser metadata in File Info in 6.62

When the browser supplied a filename and size for the current direct download, opening File Info no longer sends a separate automatic metadata request. The browser size remains labeled as reported. An explicit Refresh details request still checks the server, and Start still takes its size and body from the actual GET. If the browser supplied just a size, an automatic lookup can still supply the missing filename.

In paired runs with isolated catalogs, the staged 6.61 desktop sent one HEAD and the staged 6.62 desktop sent zero HTTP requests during the same 2.2-second observation after admission. Both downloads remained awaiting confirmation with no published file. The native filename suite passed 479 checks. Its request log showed zero requests after Cancel, one GET and a verified 4,097-byte output after Start, one HEAD for explicit Refresh details, and one HEAD when the browser supplied a size without a filename. The browser recognition suite passed 225 checks. [Production build receipt](validation/browser-metadata-no-probe-6.62-build.json)

### Showing the server filename on the first paint in 6.61

Download File Info displays an available server filename and sets the Use server name button state before its first paint. A response bound to the current URL takes precedence over the browser hint. Replacement downloads and links awaiting validation suppress the suggestion. Adopting a suggestion updates the Save As draft. The user can still edit the destination or cancel before committing it. [IDM File Info workflow](https://www.internetdownloadmanager.com/support/using_idm/using_idm.html)

The native first-paint fixture passed 25 checks. It inspected the File Info window before the prefetch timer, exercised a reported filename, a matching Save As name, response-name precedence, pending validation and replacement, and verified that canceling after using the suggestion preserved the original catalog destination. The presentation, browser recognition and filename workflow suites passed 179, 225 and 471 checks respectively. The filename suite covered late metadata, existing-file collisions and retention of the original GET when the user confirmed a new name. [Production build receipt](validation/file-info-first-paint-name-6.61-build.json)

### Showing known media sizes on the first paint in 6.60

Download File Info now keeps the source URL in its initial display snapshot so that it can show media sizes already supplied with the browser selection. A reported track total appears as Selected tracks. An estimate includes its byte count and remains labeled Estimated size after the first refresh. Without a size hint, File Info displays Unknown. The transfer still verifies the assembled output before the dialog labels it Exact final size.

The native fixture inspected the browser-owned dialog immediately after its first paint and before it registered the prefetch timer. All nine checks passed for direct YouTube admissions with reported, estimated and unknown sizes. The native presentation suite passed 179 checks, and the browser recognition suite passed 225. A separate isolated Edge HLS run passed seven checks. Its 720p MP4 dialog became actionable 172 ms after admission and displayed an exact final size of 6,560,076 bytes before Start. The published file matched the byte count and staging SHA-256, decoded fully and contained the selected audio. This timing is one controlled run. [Production build receipt](validation/file-info-first-paint-size-6.60-build.json)

### Binding background capture to the observed request in 6.59

Background requests already belong to the page. UDM now hands off only an observed full GET and identifies it by the browser request ID, URL, tab, frame and document when available. This preserves the request headers from the actual GET when other requests use the same URL. Background POST, PUT and DELETE responses stay with the browser. Ordinary browser download capture still uses its existing request and body checks. [Chrome request IDs](https://developer.chrome.com/docs/extensions/reference/api/webRequest#request-ids) · [HTTP retry semantics](https://www.rfc-editor.org/rfc/rfc9110.html#section-9.2.2)

The extension regression reproduced a native Add with an empty request object after a background POST with no observed request record. The 6.59 Chromium and Firefox fixtures passed 602 focused checks, including rejection of missing or mismatched identity and capture of an observed GET after a same-address POST.

In paired isolated Edge runs, 6.58 and 6.59 each sent the page's POST once and created no download from it. Afterward 6.58 left the GET in the page. 6.59 handed the GET to UDM, preserved its Accept-Language header and saved a byte-identical 1,048,576-byte file. The origin saw the POST and two GETs, one from the page and one from UDM. [Production build receipt](validation/background-request-identity-6.59-build.json)

### Checking ordinary downloads against current desktop rules in 6.58

Browser capture reads current site rules with the same desktop preferences it uses for exclusions, file types and transfer capabilities. A cached allow rule can justify waking the desktop for a file outside the browser type list. The current rule still decides whether capture proceeds. An uncached file outside the type list checks an already-running desktop without launching it. An ignored or rejected download continues in the browser without a pause.

In paired isolated Edge runs, 6.57 took over downloads matching a removed allow rule or a newly added ignore rule. In 6.58 each completed in Edge with one origin GET and matching bytes. A deleted ignore rule restored normal PDF capture, and a new uncached allow rule captured a generic binary. UDM completed both captures with byte-identical files. The repeatable origin served one browser request and one desktop request per accepted capture. Firefox was exercised in the extension fixtures. The 605 focused checks passed. The broader POST capture fixture still fails its first assertion with both 6.57 and 6.58. [Production build receipt](validation/download-capture-policy-6.58-build.json)

### Checking background capture against current desktop policy in 6.57

IDM documents separate site and address exceptions for browser takeover. UDM already checked those before touching an ordinary browser download. The separate background rule path could still submit a download based on cached rules when desktop capture was disabled, the file type was excluded, or the current rule no longer authorized it. The extension now checks the local exclusion list, then reads current desktop exclusions, types and rules before submitting. A rejected attempt releases its deduplication entry. Concurrent matching events still share one attempt. [IDM File Types options](https://www.internetdownloadmanager.com/support/options.html)

The new regression reproduced a native Add under disabled desktop capture against the staged 6.56 browser. Both 6.57 bundles passed the 76 rule checks, including seven rejection-and-retry cases per browser, along with eight ordinary exclusion, 225 recognition, 75 transport and 14 menu-policy checks. The staged desktop and native host returned the configured background rules, capture switch, host exceptions and exact address exceptions from an isolated catalog. The separate broad browser fixture still lacks AbortController in its mock browser. [Production build receipt](validation/background-capture-policy-6.57-build.json)

### Reusing a saved date-validated range after reopening in 6.55

UDM now attempts a saved offset when its partial file has a strong Last-Modified date, including when range support was still unknown at the time of the interruption. One response supplies both validation and the missing bytes. UDM checks the date, returned interval and saved overlap before appending. If the server returns a usable full response, UDM can read that response from the beginning. A weak date, changed date or mismatched overlap requires a clean restart for an ordinary download. [HTTP partial response validation](https://www.rfc-editor.org/rfc/rfc9110.html#section-15.3.7.3)

In the controlled restart comparison, 6.55 retained 262,144 bytes and went straight to the remaining range. The 6.54 baseline first requested byte zero. Both verified outputs were 491,520 bytes. The native suites passed 96 checks and browser recognition passed 211. [Validation results](validation/saved-date-range-6.55.json)

### Deferring the single-connection range probe for strong dates in 6.54

When a server omits Accept-Ranges and supplies a Last-Modified date that meets UDM's existing strength check, a fresh one-connection download reads the first full response immediately. On a dropped connection, the resumed response must match the original modification date, byte range and saved overlap before UDM appends new bytes. A changed or weak date triggers a clean restart. [HTTP validator requirements](https://www.rfc-editor.org/rfc/rfc9110.html#section-8.8.2.2)

Against the same controlled origin, the staged 6.53 desktop made two GETs for a 491,520-byte file and 6.54 made one. Both files matched the origin byte for byte. The native fixture also verified an interrupted matching response and clean restarts for changed and weak dates. The direct-media recovery assertion now counts the 64-byte overlap as traffic and checks unique file coverage separately. [Validation results](validation/date-only-range-6.54.json)

### Applying one connection limit across direct media tracks in 6.53

Direct video and audio now use the connection budget saved on the parent job when the transfer starts. Previously, each track kept the number assigned at capture, so editing the scheduled connection count could leave the active workers unchanged. With a budget of one the unfinished tracks run in order. A larger budget can be divided across both tracks, and a lone unfinished track gets the full budget. The transfer also uses the parent's current queue for track retry settings. [IDM connection options](https://www.internetdownloadmanager.com/support/using_idm/options.html)

The controlled native fixture observed one stalled response at a budget of one and two overlapping stalled responses at a budget of two. Both runs cancelled without publishing a file. A separate serial run used one GET per track and the output fully decoded. All 18 media-prefetch network checks passed. The direct-refresh suite had 49 passes and one resumed-byte accounting failure, also reproduced with the retained 6.40 test binary. [Validation results](validation/direct-track-limits-6.53.json)

### Setting transfer limits from File Info Schedule in 6.52

File Info now opens the same connection and per-file speed controls as the schedule wizard opened from the download list. It seeds the draft from the current job and displays the edits on Review. Saving commits the transfer settings, destination and one-time queue together. During an active prefetch, the same transaction records the future limits before releasing the worker to stop. A completed prefetch remains eligible for verification and reuse.

The isolated Windows fixture passed 455 checks. With prefetch disabled and with a complete response already staged, the saved catalog reopened with one job, a future start, two connections and a 512 KB/s limit. Cancelling the wizard preserved the original options, queue list and verified prefetch. After checking that the future queue stayed idle, the fixture manually started each job and verified all 4,097 bytes. The prefetched case made one GET across confirmation, scheduling and manual start. The cold case made one metadata HEAD and one GET. The fixture did not measure throughput or wait for the real scheduled time. [Validation results](validation/file-info-schedule-limits-6.52.json)

### Closing File Info consistently in 6.51

Cancel, Escape and title-bar Close now use the same File Info dismissal handler. A fresh browser offer is removed after its prefetch stops. Existing records and captured original responses retain their pause and recovery behavior. Download Later remains the way to keep an unstarted offer in the list. [IDM Download File Info workflow](https://www.internetdownloadmanager.com/support/using_idm/starting.html)

The isolated Windows fixture passed 434 checks. The five dismissal cases each made one GET and published no file. Cancel, Escape and title-bar Close removed fresh entries and their staged files. The Close case interrupted a response with only part of the body received. Escape and Close on previously saved entries retained their paused records and all 4,097 staged bytes. The IDM window was present, but Windows Graphics Capture failed on the initial attempt and retry. The live side-by-side dismissal comparison remains open. [Validation results](validation/file-info-dismissal-6.51.json)

### Preserving capture shortcuts through redirects in 6.50

IDM documents Ctrl-click to force capture and Alt-click to leave the download in the browser. UDM 6.50 keeps its configured gesture when an HTTP redirect hides the clicked URL from the browser's download event. The extension verifies each hop using the browser's redirect event and existing request context. It requires one matching final GET, the same tab and frame, a matching source page, and matching document IDs when available. It accepts a gesture issued before the matched hop began and consumes it once. The lookup uses observed traffic and adds no HTTP requests. [IDM shortcuts](https://www.internetdownloadmanager.com/support/using_idm/starting.html) · [Chromium redirect API](https://developer.chrome.com/docs/extensions/reference/api/webRequest#event-onBeforeRedirect) · [Firefox redirect API](https://developer.mozilla.org/en-US/docs/Mozilla/Add-ons/WebExtensions/API/webRequest/onBeforeRedirect)

The worker regression reproduced the lost force gesture on the previous code. Both bundles passed the redirect cases for force and bypass, multihop redirects, missing or mismatched redirect observations, POST origins, ambiguous destinations, and mismatched tab, frame, and document identity. The isolated Edge redirect attempts did not complete. The real-DOM checks cover focus and shortcut suppression in editable controls. [Validation results](validation/redirect-gesture-6.50.json)

### Holding the visible batch through a pointer click in 6.48

Download all uses the same pointer guard as individual format rows. A catalog update received after pointer-down waits for the gesture to finish. A click submits the batch that was displayed at pointer-down. Releasing outside the button applies the pending update without starting a download.

The 6.47 Edge baseline submitted a different pair of format keys after a refresh arrived during the held click. The 6.48 fixture recorded the original pair on release over the button and zero submissions on release outside. Both extension bundles passed in Edge, and the Chromium bundle passed in Chrome. The interaction, panel and browser logic suites passed 409 checks. [Validation results](validation/panel-batch-pointer-6.48.json)

### Keeping keyboard selection safe during video refresh in 6.47

The panel keeps focus on the same format or action when its identity survives a catalog update. When the focused quality, track-options button or Download all action disappears, focus returns to the panel trigger and the menu asks for a new choice. Enter cannot select a replacement quality. Arrow keys let the user return to the available choices. IDM documents the video panel as the place to choose a format and quality. [IDM video workflow](https://www.internetdownloadmanager.com/register/new_faq/video5.html)

In the staged 6.46 baseline, removing the focused 720p row moved focus to 1080p and a trusted Enter issued a download handoff. In 6.47 the same sequence issued no handoff. The keyboard fixture passed with both bundles in Edge and the Chromium bundle in Chrome. The combined keyboard, panel control, source change and browser logic tests passed 403 checks. [Validation results](validation/panel-removed-focus-6.47.json)

### Keeping ZIP Preview on the current response in 6.46

When File Info already owns an HTTP background download for the selected archive, ZIP Preview opens immediately and waits for that response's saved receipt. It then reads the directory through the same bounded local parser. Closing the preview while waiting leaves the worker and the parent confirmation active. An ended prefetch with no complete archive leaves a message in Preview. When there is no active HTTP prefetch or the login draft describes a different request, Preview can still use the remote reader.

The slow fixture held back the ZIP directory for 2.2 seconds. It opened Preview after headers, cancelled while the archive was incomplete, reopened Preview, and listed the entry before Start. The slow and already-complete cases each made one GET and published identical archive bytes. All 407 native and 336 browser checks passed. [Validation results](validation/zip-preview-inflight-6.46.json)

### Reusing the completed prefetch for ZIP Preview in 6.45

The File Info ZIP Preview action uses the locally staged archive when the current HTTP prefetch has a completed receipt for the same address and request headers, and the consolidated file matches its recorded length. It keeps the existing preview dialog and leaves the worker waiting for confirmation. ZIP Preview uses the existing bounded local reader and does not extract entries. Before the receipt is available, the remote preview path still handles the request. IDM documents ZIP Preview in Download File Info. [IDM Starting Downloads](https://www.internetdownloadmanager.com/support/using_idm/starting.html)

The Windows fixture prefetched a one-use ZIP into a custom temporary folder. Preview listed `readme.txt` before Start with no destination file present. The worker and receipt stayed active after Preview closed. Start published the exact 142-byte archive, and the origin saw one GET across the whole flow. All 393 native and 336 browser checks passed. [Validation results](validation/zip-preview-prefetch-6.45.json)

### Resolving completion drags in 6.44

IDM exposes the downloaded file as a drag source in its completion dialog. UDM now resolves the current destination at the start of the drag from either completion icon. A committed Move or Rename can occur between dialog refreshes. The drop data now uses the new path even if a different file has appeared at the old path, and the dialog updates its displayed destination at the same time. [IDM completion dialog](https://www.internetdownloadmanager.com/support/using_idm/completeD.html)

The fixture performed two catalog moves and placed an unrelated file at the old path before each check. Windows decoded the current path from the drop data for each icon before any timer refresh, including the Unicode rename. All 149 completion checks and 336 browser checks passed. [Validation results](validation/completion-drag-resolve-6.44.json)

### Correcting schedule wizard fields in 6.43

The Schedule Download wizard returns focus to the field that needs correction after a validation warning. It selects rejected text in editable fields so typing replaces it. A stop time before the start returns focus to the stop time picker. The same behavior applies when Dates was opened from Review, so the corrected step returns directly to Review. The wizard commits the draft only on Schedule or Save changes.

The native fixture rejected and corrected an address, folder, filename, queue name, speed limit and stop time. It checked focus after each warning, checked text selection for all five text inputs, and confirmed that the path, start and stop times, and speed limit survived a catalog reopen. The Scheduler fixture passed 731 checks, and the two browser suites passed 336 checks. [Validation results](validation/wizard-field-correction-6.43.json)

### Signing in after a queue failure in 6.42

Scheduled queue jobs keep supported login challenges on the download record without opening a dialog. When the user selects Resume in the main list or starts the failed job from its progress window, UDM opens the saved same-origin challenge before sending another request. Cancel preserves the challenge. Signing in uses the existing per-download login and individual resume path. The global authentication-dialog preference still applies. IDM documents silent error handling for scheduled downloads. [IDM Scheduler](https://www.internetdownloadmanager.com/support/schedulerD.html)

The isolated Windows fixture received one HTTP Basic challenge from a real queue transfer. The main window stayed quiet, and Cancel from progress left the request count at one. Resume in the main list opened the login dialog before sending anything. Confirming credentials produced one authenticated request and saved all 1,048,647 bytes. The native fixture passed 179 checks after linking its media helpers. The two browser suites passed 336 checks. [Validation results](validation/queued-login-resume-6.42.json)

### Reusing a refreshed range with Last-Modified in 6.41

UDM records a date validator for new HTTP partials when the response has no ETag and its Date is at least 60 seconds after its Last-Modified time. The interval is a conservative allowance for clock differences. An explicit address refresh can reuse the first response for the remaining range when it proves the same modification time, range bounds and total size. The existing overlap check compares the saved bytes before any new bytes are written. A missing original proof keeps the existing identity probe and does not promote older saved bytes to the faster path. Refreshed date-only requests validate the returned date directly and omit If-Range. [HTTP validator comparison](https://www.rfc-editor.org/rfc/rfc9110.html#section-8.8.2.2) · [If-Range](https://www.rfc-editor.org/rfc/rfc9110.html#section-13.1.5)

The Windows fixture interrupted the original download after 262,144 bytes. File Info refreshed the address, reused one GET for bytes 262080–524287, showed 524,288 bytes before Start, and published the matching file at the selected destination after confirmation. A changed date and a date that was too close to the response time both left the saved bytes and range plan intact. An original response with a weak date still used the byte-zero probe and completed. All 392 native and 336 browser checks passed. [Validation results](validation/refreshed-date-range-6.41.json)

### Reusing the refreshed range in 6.39

When the saved single-file transfer has bytes, a strong ETag and a complete range plan, UDM asks the replacement URL for the remaining range with If-Range. The response must match the saved range bounds, total size and ETag. The transfer then compares the overlapping saved bytes before writing any new data. The accepted response supplies the remainder. The separate identity probe remains available for other saved layouts and validators.

The Windows fixture kept 262,144 saved bytes, then refreshed an expired link inside File Info. The replacement received one GET for bytes 262080 through 524287, including 64 saved bytes for comparison. File Info showed 524,288 bytes before Start, and confirmation published the exact payload. Separate replacement responses with a changed ETag or a changed overlap left the saved part and range plan intact. The rollback fixture also waits for its catalog lock before testing a failed save. All 371 native and 336 browser checks passed. [Validation results](validation/refreshed-range-reuse-6.39.json)

### Refreshing an expired address from File Info in 6.38

Download File Info offers Refresh after a direct download returns HTTP 403, 404 or 410. The existing address dialog accepts a captured link or a pasted URL. Saving returns to the same File Info confirmation with the selected destination and description still in the draft. The parent displays the new URL and reloads the login for it. Prefetch may prepare the new response while Start Download remains the publication gate.

The Windows fixture cancelled one refresh, then saved a replacement on another loopback origin. The old origin received one GET with its fixture authorization. The replacement received one GET without that authorization. The same File Info dialog showed the new URL, retained the typed destination, displayed 4,097 bytes before Start and published those bytes after confirmation. All 343 native and 336 browser checks passed on the completed run. An earlier native run stopped at the existing durable-confirmation rollback assertion. The unchanged rerun passed that assertion. [Validation results](validation/file-info-address-refresh-6.38.json)

### Permanent HTTP rejections in File Info in 6.37

File Info retains the server's rejection and waits for Start Download before retrying a permanent HTTP 4xx response. For HTTP 403, 404 and 410 it explains that an expired link may need an address refresh. IDM documents its Refresh download address action for recovering expired temporary links from the download list. [IDM temporary-link recovery](https://www.internetdownloadmanager.com/register/new_faq/sites2_3.html)

The native dialog fixture reopened the catalog after HTTP 403 and 410 responses and confirmed that the original status and guidance persisted. The existing HTTP 200 browser-verification case also passed. Each case made one initial GET and one explicit retry, with no HEAD or extra GET on reopen. The retry published the expected 4,097 bytes. All 325 native dialog and 336 browser checks passed. [Validation results](validation/file-info-http-rejection-6.37.json)

### Browser verification in File Info in 6.36

File Info reads the saved browser-verification requirement before admitting background prefetch or issuing an automatic metadata request. The compact dialog keeps the browser verification instruction on reopen. The same requirement also prevents the fallback probe after a challenge with HTTP 200. Start Download remains an explicit retry and can clear the requirement when the download starts.

The isolated dialog fixture received a challenge with HTTP 200, closed and reopened the catalog, and checked that the browser instruction remained visible. The loopback server received only the initial GET and a second GET after Start. The second response published a byte-identical 4,097-byte file. All 324 native dialog and 336 browser checks passed. [Validation results](validation/file-info-verification-retry-6.36.json)

### Unknown-length direct prefetch in 6.35

File Info displays a browser-provided byte count as reported while the native request is connecting. Once the native GET accepts response headers without a length, the compact dialog shows Final size pending and stops displaying the browser's earlier byte count. The native response establishes the size as its body finishes. File Info keeps the response's known length when the GET supplies one and continues using the existing prefetch for publication.

The isolated Windows fixture showed the browser's 4,000-byte hint before the headers, Final size pending while the response arrived, and 4,097 bytes after EOF while Start was still waiting. No destination existed before Start. Confirmation published exactly those bytes from the single GET. All 309 native dialog and 336 browser checks passed. [Validation results](validation/file-info-chunked-size-6.35.json)

### Numbered destinations before File Info in 6.34

The native Add path tests the requested filename and keeps the simple path when it is available. When the name is occupied, it reads the catalog's destination and previous-version paths once and uses those reservations for the whole numbered search. Each candidate still checks the disk and the active move reservation. Matching still uses the same case-insensitive path comparisons and the first available suffix.

With 2,000 history records and 128 occupied filenames, median native admission before the catalog write fell from 632.4 ms to 11.1 ms. Both builds selected suffix 128. The corresponding full-commit medians were 735.6 ms and 41.9 ms. With a free filename, median pre-write times were 9.9 ms and 7.6 ms. The fixture ran nine samples after a warm-up for each build and case. These results measure native admission in a private catalog.

The native transaction, rollback, presentation, and browser session fixtures passed 413 checks. The new reservation case combines a file on disk, a differently cased history name, a previous-version path, a move in progress, and an identical filename in another folder. Cancelling the move preserves its source and makes the lowest gap usable. Both browser fixture suites passed 336 checks. [Validation results](validation/numbered-destination-6.34.json)

An isolated Edge profile used a capture rule for the loopback fixture with File Info skipped. With 128 names reserved, UDM published a byte-identical 64 KiB file as the first free name, suffix 128. The browser transfer was cancelled and the native app retained ownership. The test removed its temporary native messaging registration. An initial version mismatch exposed a cached Bridge object, which was rebuilt. The final handshake and desktop diagnostics both report 6.34.

### Duplicate lookup before File Info in 6.33

Native browser admission uses one duplicate lookup for both proxy or session checks and the download offer. The lookup compares the method and canonical URL before reading protected form data. Matching requests still require the exact POST data and account headers. An unreadable form at an unrelated URL, or a saved POST when adding a GET, no longer prevents File Info from opening. If a saved form at the matching URL is unreadable, the prepared handoff stays recoverable.

On two paired runs with 2,000 synthetic history records, including 128 protected 16 KiB form bodies, median native admission before the catalog write fell from 330.5-333.0 ms to 9.9 ms. With 2,000 GET records it fell from 14.7-14.8 ms to 10.3 ms. Full-commit timing varied across runs, especially on the first candidate run. The results cover native admission and do not measure browser click-to-dialog time.

The native transaction, rollback, presentation and session fixtures passed 409 checks, including separate GET and POST jobs at the same URL, preservation of a prepared handoff when the matching form cannot be read, and distinction between POST bodies. The Chromium and Firefox fixtures passed 336 checks. [Validation results](validation/browser-duplicate-lookup-6.33.json)

### Browser admission with large history in 6.32

An ordinary browser handoff creates a new record or reuses an existing one. A captured address refresh changes only the record identified in its saved receipt. UDM limits the rollback copies accordingly and keeps the broader backup for automatic duplicate replacement, which can reset a paused job and its progress. Reading the preparation receipt uses the saved journal under the manager lock. The accepted receipt and File Info offer still become durable in the same catalog write as the download record.

With 2,000 synthetic history records and 24 saved segments per record, the median time from native commit entry to the pre-write checkpoint fell from 45.8 ms in 6.31 to 16.9 ms in 6.32. Full commit medians in those runs were 199.0 ms and 125.3 ms. With an empty history, the corresponding pre-write medians were 0.70 ms and 0.68 ms. The isolated fixture recorded nine samples per build and history size after one warm-up. These measurements cover native admission. Browser click-to-dialog and production workload latency require separate measurements. [Build receipt](validation/capture-catalog-rollback-6.32-build.json)

The capture transaction, rollback, and presentation persistence fixtures passed 382 checks. The native UI fixture passed 168 checks, including File Info reachability while the main window stays hidden. The Chromium and Firefox fixtures passed 336 checks across file recognition and the capture transaction. The crash matrix now expects two deferred saves for existing-download reuse, matching the observed path in both versions. [Validation results](validation/capture-catalog-rollback-6.32.json)

### Native preparation reply and File Info in 6.30

When the native host has just acknowledged a saved preparation, the active browser worker uses that reply for the cancellation transaction. It still verifies browser identity, policy, and the final URL before canceling. An interrupted handoff still queries the native journal because the new worker does not have the preparation reply.

Three isolated Edge and Windows dialog trials per build used the same 2,097,171-byte attachment and injected a 250 ms delay into each pre-commit status query. Median time from browser download creation to a visible, enabled Start Download button fell from 508 ms in 6.29 to 225 ms in 6.30. The median time to dispatch native commit fell from 321 ms to 50 ms. Pre-commit status calls fell from one to zero. Each run showed the exact size and made its native prefetch GET before Start, then published one byte-identical file. Each origin saw two GETs. These are controlled measurements with an injected delay. Public-site and installed timing remain open.

Eight focused suites passed 562 checks, including browser cancellation, eligibility changes, and native-journal recovery after interrupted preparation in both bundles. [Build receipt](validation/reuse-native-prepare-6.30-build.json)

### Automatic handoff preflight in 6.29

The extension reads desktop preferences once for an automatic handoff. It uses that reply for capture policy and for the proxy, session, and form capabilities needed to construct the request. It finishes this work before touching the browser response on the legacy pause path. The credential and capture checks at cancellation still read current browser state.

An isolated Edge fixture ran three direct-route attachments per build with a deliberate 250 ms delay per native preferences call. Version 6.28 made two such calls per download and reached native preparation in a median 566 ms after browser download creation. Version 6.29 made one call and reached preparation in 284 ms. Each run produced one byte-identical 2,097,171-byte native file and used two origin GETs. The test measures the interval before native preparation under the injected delay. It does not measure File Info paint or public-site timing.

Eight focused suites passed 562 checks. They included automatic proxy routing and captured POST body limits, and verified that a desktop type rejection or unsupported capability never pauses the legacy browser response. [Build receipt](validation/automatic-handoff-preflight-6.29-build.json)

### Prepared browser response address in 6.28

The automatic handoff now checks that its target matches the browser's final HTTP address before preparing the native request. It stores an address digest bound to the download identity and checks it again before cancellation, including after a worker restart. The digest includes the query, and the comparison ignores fragments. Earlier preparations without the digest release while the browser response is still available. A recorded cancellation still follows the existing recovery path.

An isolated Edge fixture issued real HTTP redirects for two attachments. Both builds handed the stable redirect to UDM and saved a byte-identical file. For the second attachment, the fixture simulated a change to the browser's reported final URL after the native host saved preparation. Version 6.27 canceled the browser and fetched the old target again. With 6.28 Edge kept the original 2,097,171-byte file, matched its SHA-256, and made one GET to the final resource with no native job for that case. The seven focused suites passed 536 checks, including changed and legacy preparation recovery in both browser bundles. [Build receipt](validation/prepared-response-address-6.28-build.json)

### Capture policy before browser cancellation in 6.27

Before canceling a browser download with a prepared native handoff, the extension reads the current browser capture switch and exclusions, including the desktop policy already synchronized to the browser. It checks the active download identity again and releases preparation if capture is disabled, the address is excluded, or the destination URL changes during that check. Recovery can apply the same checks after a worker restart using the browser download item. This adds no native round trip.

An isolated Edge test changed one browser setting after native preparation for each of two credential-free downloads. It turned off automatic capture in the first case and added the origin to site exclusions in the second. Version 6.26 canceled both browser responses and made a second GET for each. Version 6.27 kept the original responses in Edge. Each 2,097,171-byte file matched SHA-256 in one GET, and the native catalog stayed empty. The seven focused suites passed 526 checks, including both browser bundles and worker restart. [Build receipt](validation/prepared-browser-policy-6.27-build.json)

### Credential consent before browser cancellation in 6.26

If a prepared native handoff contains browser credentials, the extension checks consent again before canceling the browser download. The journal stores only a flag indicating that credentials were shared. A new worker can read that flag and check current consent during recovery. If consent is off while the original transfer remains available, UDM releases the native preparation and the browser retains its response.

An isolated Edge test granted the fixture extension cookie access and switched its sharing setting off after the native host acknowledged preparation. Version 6.25 canceled the browser and made a second GET using the saved cookie. Version 6.26 left the browser in control. Edge saved the original 2,097,171-byte file with a matching SHA-256 in one GET and the native catalog contained no job. The six focused suites passed 458 checks, including worker-restart recovery in both browser bundles. [Build receipt](validation/credential-revocation-6.26-build.json)

### Browser ownership when credential sharing is unavailable in 6.25

When a captured request used Authorization, or sent a cookie for a response that varies by Cookie, automatic handoff requires current credential-sharing consent. The browser download handler checks before pausing or opening a native admission. The request builder checks again before submitting, including for background capture rules. An incidental cookie alone does not trigger the guard. [HTTP Authorization](https://datatracker.ietf.org/doc/html/rfc9110#section-11.6.2) · [HTTP Vary](https://datatracker.ietf.org/doc/html/rfc9110#section-12.5.5)

An isolated Edge test used a 2,097,171-byte attachment that required a session cookie and sent `Vary: Accept-Encoding, Cookie`. With credential sharing off, 6.24 canceled the browser response and sent a second GET without the cookie, which returned HTTP 401. With 6.25 Edge saved the complete file with matching SHA-256 in one GET and no native job. Fixtures exercised both browser bundles, including permission denial and acceptance for Authorization and cookie variation. The seven focused suites passed 492 checks. [Build receipt](validation/browser-credential-ownership-6.25-build.json)

### Browser ownership of ranged requests in 6.24

Automatic capture keeps a browser request with a Range header, HTTP 206 status, or Content-Range header in the browser. This decision runs before a pause, cancellation, or native admission. The same rule applies to the background site-rule path when it would send a full native request. Media detection still receives partial responses. [HTTP range semantics](https://datatracker.ietf.org/doc/html/rfc9110#section-14) · [Browser fixture](../tests/file-recognition.test.cjs)

An isolated Edge fixture sent `Range: bytes=0-` to a one-use URL. The origin deliberately ignored Range, returned the complete body, and rejected a second GET with HTTP 410. Build 6.23 canceled Edge's download and its native retry received 410. With 6.24 the browser completed the original 2,097,171-byte file with matching SHA-256, one GET, and no UDM job. This verifies preserving a detectable ranged request. Native adoption of arbitrary already-issued single-use HTTPS responses remains open. The browser fixtures passed 381 focused checks across recognition, site rules, recovery, transport, forms, and sessions. [Build receipt](validation/browser-range-ownership-6.24-build.json)

### Iframe attachment routing in 6.23

An embedded-frame navigation with `Content-Disposition: attachment` goes through the normal browser Downloads handoff. The background site-rule path no longer starts an independent transfer for the same navigation. A matching rule can still capture the ordinary download, and it still applies to XHR and inline frame responses. Both browser fixtures exercise the routing distinction. [Transport fixture](../tests/transport-handoff.test.cjs) · [Build receipt](validation/iframe-attachment-ownership-6.23-build.json)

The same isolated Edge fixture sent an `add` and a `capture-prepare` to 6.22. It sent only `capture-prepare` to 6.23. Both saved one byte-identical 2,097,171-byte file and made two origin GETs, one browser request and one native request. This verifies the handoff decision for a repeatable response. Adopting an already-issued single-use HTTPS response is still open. Separate attempts with a ZIP attachment received HTTP 204 in Edge before download creation, even though the origin sent 200. The passing pair used a generic binary attachment.

### File Info after interrupted prefetch in 6.22

When the direct download response has supplied its headers, File Info continues using that response for size after the background transfer stops. The compact dialog shows the interruption beside the size, and the expanded details keep the specific error. If the GET never supplies headers, a prior HEAD value can still be shown as server-reported.

The isolated Windows fixture passed 304 checks. In the interruption case the HEAD lookup returned 4,001 bytes, the GET declared 4,097 bytes, and the origin closed after sending 1,024 bytes. The compact dialog displayed the GET size beside “Background download stopped” before Start. The test observed one HEAD, one GET, and no published file. [Dialog fixture](../native/FilenamePreviewUiTests.cpp) · [Build receipt](validation/file-info-prefetch-interrupted-6.22-build.json)

### File Info size while prefetch connects in 6.21

For direct downloads, File Info keeps an available HEAD length labeled as server-reported after a transfer slot opens. The GET headers take precedence as soon as the transfer accepts them. If the GET has no length, File Info can wait for the actual body size. The dialog uses the existing metadata request and prefetch response.

The isolated Windows fixture passed 298 checks. With the only transfer slot held, File Info displayed the HEAD value of 4,001 bytes. It kept that hint during a 1.5-second GET header delay, then displayed 4,097 bytes from the actual response before Start. The full file matched those bytes after confirmation. The origin saw one HEAD and one GET for this case. [Dialog fixture](../native/FilenamePreviewUiTests.cpp) · [Build receipt](validation/file-info-retain-reported-size-6.21-build.json)

### Single-connection range verification in 6.20

For a fresh known-length response with an ETag, one selected worker and no Accept-Ranges header, the worker reads the existing full response immediately. A reconnect uses the ETag, exact returned range and boundary bytes to validate continuation. The progress dialog reports range support as unknown until a valid resumed response proves it.

Five isolated trials per build alternated launch order and downloaded the same 480 KiB file. The loopback origin deliberately delayed Range response headers by 700 ms. Median native HTTP entry to first recorded body progress fell from 765 ms in 6.19 to 63 ms in 6.20. Requests fell from two to one. These timings cover the transfer engine on the controlled origin. File Info first paint and public origins need their own measurements.

| Recovery case in 6.20 | Observed requests and result |
| --- | --- |
| Origin supports ranges but omits the header | Full GET and one validated resume with a 64-byte overlap |
| Origin returns HTTP 403 to Range | Full GET, refused range, then one full restart |
| Origin ignores Range | Full GET and one range request whose full response supplies the restart body |
| Origin returns a mismatching overlap | Full GET, range with bad overlap, then one full restart |
| Origin advertises Accept-Ranges | One full GET |

All 15 outputs matched the fixture bytes. A separate HTTP 403 with Retry-After recorded the deadline, retained the partial, and made no additional GET. [Build receipt](validation/deferred-single-range-probe-6.20-build.json)

### Isolated browser qualification

The current Chromium bundle connected to the staged 6.13 native host and a private app catalog in Edge. The existing HLS fixture passed all 25 checks. It exercised actual panel selection, nested, flat and cyclic playlist graphs, MP4 with selected audio, TS with alternate audio, M4A without fetching video, and Unicode subtitles with timing and language preserved. The fixture decoded the outputs and removed its temporary native messaging registration. It used skip-File-Info mode. Pre-Start size, active personal Chrome, Firefox, and equivalent IDM video results remain open. [Fixture](../tests/hls-catalog.native-live.cjs)

### Completion dialog qualification

The current native completion fixture passed 144 checks against retained IDM control metadata and UDM-specific behavior. It checked labels and control geometry, tab traversal and wrap, destination and file actions, duplicate notification prevention, and scanner-expanded layouts at 96, 120, 144 and 192 DPI. The 5.45 scanner-bounds failure did not recur. A current installed side-by-side pass still needs to measure foreground and focus, display scaling, and actions with actual shell handlers. [Reference metadata](reference/completion-dialog.json) · [Fixture](../native/CompletionDialogUiTests.cpp)

### Installation checkpoint

Program Files still reported desktop 5.45 and browser 0.69.9 at the last check. The requested local setup was waiting for Windows elevation and had produced no install log. The pre-upgrade catalog was backed up locally. Record the installer exit status, the four persisted executable hashes, both manifests, registered native hosts, catalog identity, and running process path before treating the upgrade as verified. Setup may extract its fifth executable, SetupHelper, temporarily. Browser activation requires its own comparison.

Windows Computer Use returned an accessibility window-open handler error for IDM and later returned no targetable IDM window. The command tool also blocked the separate interactive IDM launch with a policy rejection. The supported silent command-line transfer ran successfully in the paired cases above. Interactive comparison remains open for the main window, dialogs, Scheduler, and browser video panel. [IDM command-line reference](https://www.internetdownloadmanager.com/support/using_idm/starting.html)

## Target architecture

| Owner | Boundary and invariant | Evidence to collect |
| --- | --- | --- |
| Browser observation and player selection | Bind each request to tab, frame, document, visit, visible player and representation. Admit only current, playable evidence and release stale work immediately. | Ad transitions, SPA navigation, two players, stale and out-of-order observations. |
| Capture admission and presentation | One selection creates one receipt, one File Info window, and at most one durable job. Dispatch and first actionable paint precede optional media inspection. | Correlated timestamps, foreground owner, duplicates and host replay. |
| Prefetch and size proof | Bind proof and retained data to the same entity, tracks, format, container and output revision. Transfer ownership exactly once on Start or Later. | Verified exact bytes before Start, monotonic retained bytes, exact final byte count and complete decode. |
| Transfer and publication | Every counted byte maps to one verified output interval. Pending writes and worker changes preserve recovery, throttle and quota accounting. | Per-range events, hashes, retransmitted bytes, first/last byte and publication. |
| Desktop dialogs and commands | Centralize command enablement and use the current job state for focus, menu entries, tab order and actions. | Matched screenshots and accessibility snapshots at common DPI and themes. |
| Scheduler and catalog | Validate one immutable draft at Save. Persist job edits, schedule membership, wake intent and local-time resolution transactionally. | Reopen and reload, forced write failure, DST boundaries, source/destination collisions and stale drafts. |
| Grabber | Preserve provenance for static and rendered discovery. Keep scope, archive mapping and resume identity in the project record. | Controlled page assets, parent/child navigation, selected resources and offline replay. |
| Windows deployment | Identify build, package, installed files, running image, registrations and active browser version independently. | Hashes, no catalog loss, live app launch and tested shell/native-host routes. |

## Work order and acceptance

### P0. Completion dialog, File Info and exact size

1. **COMP-01 · Completion layout.** Start with [App.cpp](../native/App.cpp), [CompletionDialogUiTests.cpp](../native/CompletionDialogUiTests.cpp) and the [reference metadata](reference/completion-dialog.json). Pair ordinary, scanner-expanded, long-path and multi-completion cases. Measure dialog client bounds, title text, icon position, byte unit and grouping, URL and path alignment, row heights, action width, checkbox placement, and drag affordance. Check Minimize, Close, disabled Maximize, default focus, Alt mnemonics, Tab and Shift+Tab, Enter, Escape, copy, and cross-monitor DPI. File actions need to reference the verified output even if it moved after completion. Check Open, Open with, Open folder and file drag individually. Preserve completion suppression scope and keep publication out of the UI layer. [IDM completion](https://www.internetdownloadmanager.com/support/using_idm/completeD.html)
2. **INFO-01 · Fast actionable paint.** Start at selection time in [browser background](../browser/chromium/background.js), [Bridge.cpp](../native/Bridge.cpp), [CaptureAdmission.cpp](../native/CaptureAdmission.cpp) and [DownloadInfoUi.hpp](../native/DownloadInfoUi.hpp). Split timings into catalog resolution, extension dispatch, host attach, admission, HWND creation and visible actionable paint. Cover cold and warm hosts, main window minimized, app in tray, Chrome foreground, and an existing File Info dialog. Require one visible confirmation and zero main-list foreground changes for browser handoff. Late title, icon, size and category details must never overwrite a user-edited path. Compare button order and labels, path selection, Browse, category mnemonic, description, Login, Advanced, ZIP preview, control focus, and return focus to browser.
3. **SIZE-01 · Exact output proof.** Use [MediaSize.hpp](../native/MediaSize.hpp), [Adaptive.cpp](../native/Adaptive.cpp), [Streaming.cpp](../native/Streaming.cpp) and the browser offer receipt. For a direct response, require entity-bound length and content encoding. For split tracks, require complete layout for the selected output or verified staged output from that mux path. For HLS TS, cover maps, ranges, key changes, gaps and discontinuities. For MP4 and M4A, include mux overhead, alternate audio and captions. Show an honest intermediate state until exact output is proved. Verify 360p, 1080p and 1440p AV1 YouTube, M4A, X original MP4, HLS TS and MP4, recorded HLS subtitles, and indexed DASH. Require byte-for-byte size agreement and full decode with the selected tracks. Treat selection changes as invalidation of proof.
4. **PRE-01 · Real prefetch and reuse.** Trace admission, first byte, bytes before Start, adopted bytes, first progress paint and recent measured rate. Try Start, Later to a named queue, Schedule, Cancel, close, folder change across volumes, duplicate target, restart, quota and unavailable destination. The normal first progress sample should account for real bytes and time accrued during prefetch. Time spent verifying, scanning or moving a file belongs to its own phase. Add separate counters for duplicate origin bytes and retry traffic. [IDM Options](https://www.internetdownloadmanager.com/support/using_idm/options.html) · [IDM Starting Downloads](https://www.internetdownloadmanager.com/support/using_idm/starting.html)

### P1. Connection efficiency and recovery

5. **NET-01 · Reproduce sequential variability.** Extend [origin.py](../benchmarks/speed/origin.py) and [bench.py](../benchmarks/speed/bench.py) to report dispatch to headers, first byte, range negotiation, first full-response byte, last byte, verify, publish, and visibility. Compare no Accept-Ranges, ignored Range, malformed Content-Range, and compressed response. Record origin response, socket reuse, bytes cut from the discarded response, and publication latency. Follow the earliest repeatable slow phase. First gate is exact SHA-256 in every case. Use ten cold and ten warm runs before making timing claims.
6. **NET-02 · Reuse successful connections.** Instrument [Transfer.cpp](../native/Transfer.cpp) at every worker start, split, retry, refusal, pool reuse and boundary drain. Capture the active worker rate and bytes left when each split was issued. Build heterogeneous workers, a slow last range, and every second connection failing after a partial body. Keep healthy request counts and cancellation budgets bounded. Do not count a canceled overlapping range as progress.
7. **NET-03 · Optional host admission evidence.** Prototype only after concurrent and back-to-back cases show repeated refusals. Retain temporary in-memory evidence by normalized origin, transport route and relevant connection policy. Distinguish simultaneous successful streams from outstanding starts, and endpoint concurrency refusals from outages, policy 403s, 429s and expired signatures. Keep origin isolation across proxy routes and accounts, clear evidence on route change, and probe recovery conservatively after the evidence expires. Do not override an explicit lower connection limit. The two-connection test, cap removal, isolated refusal, alternate proxy, and many-job fairness are required before enabling a learned cap.
8. **NET-04 · Recovery and rate controls.** Pair abrupt close, scheduled pause, explicit Stop, crash after checkpoint, validator change, 416, 429 with Retry-After, H2 failure, FTP resume, signed URL refresh, quota, and global/per-download limiter changes. Verify unique bytes, response validators, final hash, exact local cap and retry timing. Split a reusable connection lease from per-job checkpoint state. Avoid replaying a captured POST or browser credentials on a changed origin. [IDM segmentation](https://www.internetdownloadmanager.com/support/segmentation.html)

### P2. Browser panel and integration

9. **BR-01 · Readiness and correct target.** Compare real pre-roll ending naturally and with Skip, an ad-free visit, X feed with two videos, embeds, Shorts, fullscreen and theater mode. Match an observation to the current main content before publishing. Trace time from first usable player evidence to selectable row and handoff. A canceled visit leaves no discovery work or stale row. Measure request count separately from time.
10. **BR-02 · Panel details.** Capture size, position, icon baseline, margins, border, opening direction, row spacing, height, width, labels, format grouping, keyboard focus, hidden/disabled state, scroll clipping and animation under dark/light, 80–175% zoom, and a small iframe. Test drag anchoring, reload and persistence, Escape/outside click, arrows, Home/End, Enter, and preserving selection across catalog refresh. Main format rows should be actionable on the first open. [IDM video workflow](https://www.internetdownloadmanager.com/register/new_faq/video5.html)
11. **BR-03 · Browser takeover and fallback.** Inspect [capture.js](../browser/chromium/capture.js), [native messaging](../browser/chromium/native-bridge.js) and [CaptureAdmission.cpp](../native/CaptureAdmission.cpp). Cover Chrome, Edge and Firefox one at a time with attached files, redirects, force/bypass keys, site and extension exceptions, auth, form submissions, blob, incomplete host ack and disconnected hosts. Require one owner of any accepted browser response. Declined or unreachable handoff leaves the browser transfer usable. Do not compare a fixture temporary host with active personal-profile activation.
12. **BR-04 · Session refresh.** Track same video, quality, language, container and output revision through media refresh. Record transport-level errors apart from stale player evidence and expired signed URLs. Resume exactly the verified portions or issue a clear refresh action, with one job and no silent quality downgrade.

### P3. Main window, progress and properties

13. **UI-01 · Desktop details.** Compare the Tasks, File, Downloads, View, Help and tray menus, split queue buttons, disabled states, keyboard shortcuts, toolbar spacing, icon sizes, category tree and counts, list headings, width/order, sorting state, selection, column tooltips, horizontal scrolling and status bar. Run first launch, resize, high contrast, dark/light, 100/125/150/200% DPI, second monitor, and right-to-left layout separately. Identify deliberate UDM-specific categories in the fixture. [IDM Main Dialog](https://www.internetdownloadmanager.com/support/using_idm/using_idm.html)
14. **UI-02 · Progress and graph.** Check progress text, transferred size, ETA, resume capability, status transitions, segment start marker, completed range fill, compact/expanded graph, actual sampled rates, tooltip placement on narrow view, minimize, restore, taskbar and cancel. Keep speed, ETA and axes consistent through pause and resume. Use a deterministic stepped-rate origin to validate the graph at its source. Capture verified transition to assembly, hash, scan and final publication.
15. **UI-03 · Properties and save rules.** Compare display and editability of source, destination, description, type and details for waiting, in-progress and completed entries. Verify cancellation, long Unicode paths, UNC, collisions, locked files, category-folder inheritance, remembering the last location, completed move recovery, missing output, Open, and re-download. Keep metadata and physical file movement under the transaction in [FileWorkflows.cpp](../native/FileWorkflows.cpp). [IDM Properties](https://www.internetdownloadmanager.com/support/using_idm/properties.html)
16. **UI-04 · Options and accessibility.** Inventory tabs, section headers, radio/checkbox defaults, file type and site-rule precedence, connection defaults and per-server overrides, save and temp folders, proxy, sound, login, download dialog startup choices, and scanner presentation. Check accessible label association, keyboard focus and tree order for native and browser surfaces. Compare persisted state after Cancel, Apply and Close. [IDM Options](https://www.internetdownloadmanager.com/support/using_idm/options.html)

### P4. Dates, queues and the scheduling wizard

17. **SCH-01 · Qualify the wizard.** Run Tasks, File Info, waiting-job menu and Edit against [ScheduleDownloadUi.hpp](../native/ScheduleDownloadUi.hpp) and [Queue.cpp](../native/Queue.cpp). Record source validation and login scope, folder dropdown vs Browse, category default vs manual path, filename, description, full-path preview and tab order. Walk forward, backward and from each Review edit link. Cancel during each step and verify no catalog write. Check Enter and Escape and in-page validation focus. Reopen a saved schedule and change location, date, retry, wake, speed and connections. Confirm the exact durable job and queue values after app restart. Keep File Info prefetch and the scheduler's original confirmation semantics intact.
18. **SCH-02 · Calendar and local-time edges.** Exercise dates entered from the calendar, time to the second, Today, Tomorrow, In 5 minutes, In 1 hour, and both stop shortcuts. Verify month/year rollovers, leap day, gaps and repeated hours from DST, past starts that expire during review, stop before start, and optional stop. Define missed starts, manual start of scheduled jobs, and waking with explicit hardware qualification. Keep each one-time job's chosen UTC instant stable after time-zone or locale change. Inspect multi-date requirements separately if individual runs are later needed.
19. **SCH-03 · Queue parity.** Compare Scheduler tabs, queue order, limits, per-file retries, queue concurrency, manual vs scheduled run, synchronization, retry-to-tail, optional stop and completion actions. Cover multiple queues at the same origin, sleep/resume, independent stops and completion suppression. Exercise safe inert completion commands before any real power action. Check names, the Q indicator, menu wording, button locations, startup and invalid Apply behavior. [IDM Scheduler](https://www.internetdownloadmanager.com/support/schedulerD.html)

### P5. Grabber and Windows delivery

20. **SITE-01 · Grabber.** Pair all four configuration stages and the results view with the same owned site. Cover presets, auth and logout exclusions, URL/path selection, internal/external depth, redirects, duplicates, MIME filters, nested archives, selected download, save/reopen project, project scheduling and queue handoff. For rendered capture, check lazy images, shadow DOM, responsive images, selected media/captions, and offline links after stopping the origin. Compare control labels and default navigation at matched DPI. [IDM Grabber](https://www.internetdownloadmanager.com/support/idm-grabber/grabber_wizard.html)
21. **WIN-01 · Install, repair and shell.** Hash source, setup and installed files. Verify effective native host registration in Chrome/Edge/Chromium/Firefox, file protocol and Explorer shortcut verb, COM, installed shell icon, running image path and preserved personal catalog. Compare fresh install, upgrade with locked host, repair, uninstall and rollback in a disposable VM. Reopen a harmless Internet Shortcut from Explorer before marking the shell path verified.
22. **REL-01 · Acceptance and recording.** Publish an evidence ledger with app identity, theme, DPI, browser and profile, run order, trace version, observations and limitations. Capture matched clips for File Info paint, exact size, prefetch, completion and schedule navigation. Keep source implementation, focused fixtures, installed behavior and cross-app evidence in separate columns. Re-run the affected gate for each desktop/browser version increment.

## Mister Download Manager review

Reviewed the repository at `5d30a7c36b2e963fc0245be58615b64fd6516d67`. Its workspace declares GPL-3.0-or-later. UDM's original code and documentation are Apache 2.0. No upstream code or assets were imported. A separately permissively licensed contribution would need its own provenance and notices. Apache's compatibility explanation applies to Apache code included in GPLv3 works, with GPLv3 terms applying to the combined work. [MDM license](https://github.com/anefzaoui/misterdownloadmanager/blob/5d30a7c36b2e963fc0245be58615b64fd6516d67/Cargo.toml) · [Apache compatibility explanation](https://www.apache.org/licenses/GPL-compatibility.html)

| Upstream design or evidence | UDM decision |
| --- | --- |
| Learn a cap across jobs from extra connection refusals while other connections are productive. The upstream code expires observations and raises the estimate when it sees more productive connections. [Host admission source](https://github.com/anefzaoui/misterdownloadmanager/blob/5d30a7c36b2e963fc0245be58615b64fd6516d67/crates/engine/src/hosts.rs) | Useful independent design for NET-03. UDM already paces failed opens within one transfer and only saw one refusal per capped trial. Demonstrate a multi-job benefit and define proxy/account isolation before implementation. |
| Separate startup, body transfer, finalization, CPU and peak memory. Include a slow minority of connections and partial failures on alternating connections. [Benchmark report](https://github.com/anefzaoui/misterdownloadmanager/blob/5d30a7c36b2e963fc0245be58615b64fd6516d67/docs/bench/run5/REPORT.md) | Adopt the workloads and measurements independently in UDM's existing harness. The report tested MDM against a different IDM build and did not include UDM. Its reported speed ratios cannot rank UDM. |
| Let the browser retain ordinary downloads when the desktop cannot accept them. [Extension admission source](https://github.com/anefzaoui/misterdownloadmanager/blob/5d30a7c36b2e963fc0245be58615b64fd6516d67/extension/background.js) | Compare timeout and ambiguous acknowledgement cases in BR-03. UDM already has admission receipts and response ownership. A rewrite would need to preserve them, along with POST handling and browser security context. |
| Broad locale support, mirrored layouts, remote control, torrents and debrid. [Project scope](https://github.com/anefzaoui/misterdownloadmanager/tree/5d30a7c36b2e963fc0245be58615b64fd6516d67) | Locale and keyboard coverage are relevant to UI-01 and UI-04. Remote control, torrents and debrid are outside the present IDM-parity scope. |

### Follow-up on 2026-11-09

Reviewed MDM at `aefa9381886e4d3860392d2ae3d81694cc665c32`. Its transfer worker can reconnect a slow response using the best speed observed earlier in the same download. UDM currently splits remaining work for a faster worker or helps a stalled worker. The reconnect policy is a candidate for NET-03 once a paired run shows whether it reduces the transfer tail and how many additional requests it costs. [MDM engine](https://github.com/anefzaoui/misterdownloadmanager/blob/aefa9381886e4d3860392d2ae3d81694cc665c32/crates/engine/src/download.rs)

UDM's Apache-licensed [benchmark harness](../benchmarks/speed/README.md) now supports the three workloads below and records the first and last body writes separately from observed completion. Each case ran three times with UDM 6.89, eight requested connections, and a private catalog. All nine files matched the expected SHA-256.

| Workload | Median wall time | Median to first body write | Median body window | Median from last body write to observed completion |
| --- | ---: | ---: | ---: | ---: |
| `small-256k` | 0.515 s | 0.404 s | <0.001 s | 0.111 s |
| `mixed-speed-32m` | 6.467 s | 0.699 s | 5.409 s | 0.360 s |
| `alternating-drop-50m` | 2.443 s | 0.490 s | 1.810 s | 0.125 s |

UDM reassigned ranges in every mixed-speed run and recovered from six or seven forced drops in each alternating-drop run. The completion interval includes polling and delivery. These measurements cover UDM only. MDM's published results use a different server, machine and IDM build. [MDM benchmark report](https://github.com/anefzaoui/misterdownloadmanager/blob/aefa9381886e4d3860392d2ae3d81694cc665c32/docs/bench/run5/REPORT.md)

## Immediate next sequence

1. Complete the installation identity check. Start the installed build in a private data directory and confirm its main window and active extension separately.
2. Run COMP-01 and INFO-01 together on the controlled 1 MiB file, then SIZE-01 and PRE-01 for one 1080p selection and one recorded HLS TS selection. Save screenshots, focus transitions, and exact byte comparisons. Expand quality and format coverage after these first traces establish a usable measurement path.
3. Reproduce the no-range delay with idle-machine alternating runs. Run the new mixed-speed and alternating-drop workloads against IDM on the same origin before tuning split thresholds or cross-job caps.
4. Run the complete scheduling wizard matrix and close any destination preview, keyboard, persistence or layout gaps before extending its schedule model.
5. Continue the remaining desktop, browser, Grabber and deployment rows with paired evidence. Advance the desktop and both browser versions for each functional delivery and preserve the prior build records.
