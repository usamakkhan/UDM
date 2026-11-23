# Release history

## 10.9 / browser 0.73.73 — consolidated 9.68–10.8 delivery

This release consolidates the documented work from 9.68 through 10.8 into one installable build. It includes staged-transfer cleanup and publication ownership hardening; completion, countdown and dialog-ownership workflow refinements; request admission, range-accounting and HTTP identity handling; and the DASH recording path improvements for discovery, freshness, single-pass resource capture, resumable pending aliases, caption handling and exact AAC completion. The browser manifests advance to 0.73.73. The detailed version-by-version behavior, constraints, rejected candidates and validation evidence remain in the entries below rather than being represented as unverified performance or parity claims.

## 10.8 / browser 0.73.72 — canonical HTTP identity encoding; built, uninstalled

Emit one byte-preserving identity-encoding field for both Windows HTTP and curl, removing internal duplicates before request admission while retaining injection rejection. Frozen/current wire controls cover GET, POST, HEAD, ranges, lower/mixed-case and conflicting fields. All five native programs build; current parallel-origin, POST recovery, admission/decoder/read, metadata, crash, recording and complete-media regressions pass. C: has sufficient build reserve; D: still fails its workspace reserve, so combined delivery acceptance remains false. No installer or installation. [Implementation, complete counts, evidence and next work](docs/http-wire-identity-10.8.md).


## 10.7 / browser 0.73.71 — resume pending resource aliases; built, uninstalled

Fix retries where another saved segment already pins the exact source list of an unfinished segment or initialization file. Only explicit unaccepted pending intents may adopt that known version; accepted data, coordinates, source lists and legacy rules retain their protection. Frozen 10.6 fails both controls; current code resumes without refetching accepted resources. All five native programs build; 107 focused assertions, 17 HTTP cases, three hard-stop cases, seven continuous recordings and nine direct/caption cases pass. Build outputs use C: with sufficient reserve; D: remains below its workspace reserve, so combined delivery acceptance remains false. No installation or installer. [Correction, evidence and ordered next work](docs/dash-pending-alias-10.7.md).


## 10.6 / browser 0.73.70 — single-pass full-resource capture; built, storage reserve not met

Use the same full GET to pin and store a fresh independent DASH SegmentList resource, with a durable version checkpoint before body writes. Preserve ranged-object preflight, compatibility fallback, legacy receipts and strict changed-version rejection. All five native programs build; 107 focused assertions, 17 HTTP workflow cases, three recovery controls, eight freshness controls, seven continuous-recording cases and nine direct/caption media cases pass in the final runs. Initial failures are retained. The final free-space audit is below the unchanged 1 GiB reserve, so combined acceptance remains false and further candidate builds are held. No installer or installation. [Ownership, exact results, failed attempts and ordered follow-up](docs/dash-single-pass-10.6.md).


## 10.5 / browser 0.73.69 — purpose-scoped resource discovery; built, uninstalled

Reuse recording-owned transport for resource-version discovery while isolating manifest, clock and discovery roles even with identical settings. Preserve HEAD/range fallback, strong version checks, cancellation and ownership bounds. All five native programs build; 85 focused assertions, 14 HTTP isolation/discovery requests, eight freshness controls and the production seven-case recording suite pass. Version probes remain; their single-pass response/checkpoint replacement is explicitly planned. No installer or installation. [Implementation, qualification and required next steps](docs/dash-discovery-roles-10.5.md).


## 10.4 / browser 0.73.68 — recording-owned DASH metadata transport; built, uninstalled

Replace per-request metadata session setup with a bounded recording-owned cache, separating origins and complete preferences. Release transports on normal exit and exceptions; keep clock requests anonymous and headers request-local. The 82 lifetime/AAC/boundary assertions, real HTTP isolation checks, held-response cancellation, eight freshness controls and production seven-case continuous recording suite pass. All five native programs build as 10.4; no installer or installation. [Ownership rules, exact scope, build attempts and remaining work](docs/dash-metadata-transport-10.4.md).


## 10.3 / browser 0.73.67 — bounded live-manifest freshness recovery; built, full acceptance incomplete

Recover from a transiently expired manifest or clock response within at most three attempts, preserving cached timing anchors, parent cancellation, identity checks and strict rejection. All five native programs build; eight targeted controls and 62 structural/boundary assertions pass. Full recording still fails when all three HTTP header stages exceed validity. A separate phase trace identifies 208–590 ms in HTTP construction/headers versus sub-millisecond parsing/accounting; a private reuse prototype passes the full suite, while a transfer-owned product implementation remains pending. No installer or installation. [Controls, retained failures and ordered follow-up](docs/dash-freshness-10.3.md).


## 10.2 / browser 0.73.66 — one final mux for a captioned DASH Period; built, uninstalled

Assemble a single captioned recording directly in the final mux, preserving exact AAC ending samples, selected track names/languages and restart receipts. Changing those metadata choices invalidates reuse. All five native programs build; 62 focused assertions, four caption-session publication cases, five direct regression cases and three negative selection controls pass. A fresh seven-case continuous run passes, while its earlier manifest-clock expiry remains an explicit open issue with the failed attempt retained. No installer, installation or IDM timing claim. [Implementation, exact results, failed attempts and remaining work](docs/single-caption-session-10.2.md).


## 10.1 / browser 0.73.65 — exact final AAC samples in qualified DASH recordings; built, uninstalled

Correct the partial final AAC LC packet for a qualified zero-origin single-Period MP4/M4A output, using bounded in-place padding metadata before the existing durable receipt. Legacy receipts are invalidated; corrected files remain reusable after restart. All five native programs build, 62 focused structural/boundary checks pass, five real capture/publication cases match complete source PCM, and seven continuous-recording regression cases pass. Multi-Period interiors, caption-session remuxing, leading edits, other players and installed/browser/IDM acceptance remain open. No installer, installation or release. [Implementation, exact results and limits](docs/gapless-aac-tail-10.1.md).


## 10.0 / browser 0.73.64 — compact native object retention; built, uninstalled

Add an opt-in, exclusively locked target-object scratch slot for builds without debug symbols, recording consumed object and executable digests before reuse. All five native programs link as 10.0; 31 distinct backend assertions pass in each of the old normal, current compact and current normal loops. Guard, lock and deliberately failed compilation controls pass. The driver executes the actual target statements with cached setup; the full packaging entrypoint is not exercised. This is a build-workflow change; installed programs and runtime parity gates remain open. [Measured retention, provenance and remaining work](docs/compact-native-build-10.0.md).

## 9.99 / browser 0.73.63 — folder handoff leaves the completion UI responsive; built, uninstalled

Move automatic completion-folder opening and the two Open folder commands to an owned worker. Retain errors behind modal ownership and join accepted handoffs before shutdown. All five programs built; 127 backend and 1,105 UI assertions passed, plus two separate frozen 9.98 source controls. These controlled launchers do not measure real Explorer or IDM paint. [Qualification records](docs/validation/shell-folder-9.99/verification.json).


## 9.98 / browser 0.73.62 — automatic presentation respects dialog ownership; built, uninstalled

Retain browser/prepared File Info, authentication, capture/review, update, clipboard and integration presentation while another owned dialog is open. Resume through the existing modal-release wake; retain newer prepared views across nested dispatch and bound presentation-failure retry to the existing timer. All five native programs build; all 116 backend and 1077 current UI assertions pass, plus 20 separate frozen 9.97 App-source assertions. Both source manifests advance to 0.73.62; no browser script changes or installed acceptance are implied. No installer, installation or release is included. [Implementation, qualification and remaining gates](docs/automatic-presentation-9.98.md).


## 9.97 / browser 0.73.61 — manual Exit honors dialog ownership; built, uninstalled

Guard Window Close, Tasks → Exit and prepared tray decisions through modal creation and teardown. Reserve dialog invocations before HWND creation and release canceled layout ownership after modal return, including layout failure. All five native programs build as 9.97; both source manifests advance to 0.73.61. All 85 backend and 721 current UI assertions pass, plus 23 separate frozen-source counterexample assertions. No installer or installation is included. [Implementation and remaining gates](docs/manual-exit-modal-9.97.md).


## 9.96 / browser 0.73.60 — completion waits for secondary modals; built, uninstalled

Defer completion presentation, action review, automatic file/queue actions and CLI completion exit while a secondary UDM window owns a modal. Recheck live ownership after a release wake and retain observation across an aborted shutdown. All five normal native programs build as 9.96; both source manifests advance to 0.73.60. All 76 backend and 428 current UI checks pass, plus six separate frozen-source counterexample assertions. No installer or installation is included. [Implementation, evidence and remaining gates](docs/completion-secondary-modal-9.96.md).


## 9.95 / browser 0.73.59 — countdown wake; built, uninstalled

Wake file and queue countdowns directly when owned maintenance completes, with scoped generation-checked dialog routing. Passive completion wakes do not schedule another tick; final acknowledgement, current eligibility and safe cancellation remain required. All five normal native programs build as 9.95 and both source manifests advance to 0.73.59. All 54 backend and 130 current UI checks pass, plus five separate frozen-control assertions. No installer, installation or release is included. [Implementation and remaining gates](docs/countdown-wake-9.95.md).

## 9.94 / browser 0.73.58 — queue countdown; built, uninstalled

Move queue-finished countdown ticks to the existing background maintenance owner and use nonblocking current cycle/epoch admission. Keep Cancel/Enter responsive, acknowledge fresh scheduler work at expiry, and retain an ordered step cursor through model contention without repeating performed actions. All five normal native programs build as 9.94; both source manifests advance to 0.73.58. All 36 backend and 100 focused UI checks pass. The build is uninstalled, with no installer or release. [Implementation and remaining gates](docs/queue-countdown-9.94.md).

## 9.93 / browser 0.73.57 — file countdown; built, uninstalled

Move file-completion countdown ticks to the existing model worker, use nonblocking current eligibility reads, and acknowledge a fresh scheduler tick at expiry before allowing an action. Cancel remains usable during model contention; stopped/failed final maintenance cannot authorize an action. All five normal native products build as 9.93 and both source manifests advance to 0.73.57. All 20 backend and 66 current UI checks pass. The build is uninstalled, with no installer or release. [Implementation and remaining gates](docs/completion-countdown-9.93.md).

## 9.92 / extension 0.73.56 — uninstalled

Move armed completion-action catalog saves off the UI thread and start them after completion presentation drains. Accepted claims survive dialog Close, retain durable rollback and explicit Retry/Cancel, defer errors behind a disabled modal owner, and gate CLI Exit until joined acknowledgement. All five native products build as 9.92; both source manifests advance to 0.73.56. All 147 backend and 210 current UI assertions pass, plus the two-assertion frozen counterexample. Controlled dispatch is 31 ms versus 593 ms in frozen App source, not IDM click-to-paint timing. A normal C: 4 MiB smoke hashes both files and joins process exit/flush, but UDM loses first file availability by 15.214 ms in that sample. The build is uninstalled and general parity remains open. [Details](docs/completion-claim-9.92.md).

## 9.91 / extension 0.73.55 — uninstalled

Queue bounded initial HTTP pieces after verified range admission, keeping the full first response and the existing read-ownership and recovery checks. All five native programs build as 9.91 and both source manifests advance to 0.73.55. All 244 focused native assertions and six accounting tests pass. The engine controls improve over frozen 9.89; one normal C: pair per 4 MiB/64 MiB sequential and 64 MiB split case beats IDM at total, availability, exit and post-exit-flush endpoints. Rejected 9.90 and all losing samples remain recorded. D: repetition, installed browser/UI and broad parity remain open; this build is uninstalled. [Details](docs/initial-range-queue-9.91.md).

## 9.90 / extension 0.73.54 — intermediate candidate rejected

Bound initial helpers to one range per worker. All 244 native assertions pass, but both slow-origin controls take longer than frozen 9.89; this candidate is not installed. [Full rejection and source evidence](docs/initial-range-plan-9.90.md).

## 9.89 / extension 0.73.53 — uninstalled

Reserve pending HTTP body reads under the split-map lock, bound each request to live ownership, and retain that fence while committing the preceding write. Five native products build as 9.89; both source extension manifests advance to 0.73.53. All 213 focused assertions pass, including real 64 MiB split and Pause/Resume checks. The frozen-transfer ABBA control is mixed; speed and extra-byte parity remain open. The build is uninstalled. [Details](docs/http-read-reservation-9.89.md).

## 9.88 / extension 0.73.52 — uninstalled

Defer completion while File Properties or another modal disables the main owner, retaining both inbox events and ready intents. Re-enable posts the existing transfer-event wake so retained work resumes before CLI Exit without a main-timer tick. Five native programs build as 9.88; both source extension manifests advance to 0.73.52. All 246 backend and 612 UI assertions pass, plus a normal 4 MiB hash/catalog/shutdown smoke. A fresh one-pair-per-case IDM comparison wins total process time and exit tail in three cases, but loses first file availability in all three and sends more extra split bytes. The candidate is uninstalled and the overall parity gate remains open. [Details](docs/completion-modal-9.88.md).

## 9.87 / extension 0.73.51 — uninstalled

Give pending close and exit-layout decisions priority over deferred completion presentation, including direct admission retries. Hide to tray before opening the retained completion, and resume queued work on an owned wake after cancellation or failure. Five native programs build as 9.87 and both source manifests advance to 0.73.51. Frozen 9.86 creates a dialog before applying either ready close route; current timer/wake routes create none at that decision. All 246 backend and 476 UI checks pass, plus a normal 4 MiB hash/catalog/graceful-exit smoke. Temporary fixtures are removed after sanitized export. The build is uninstalled and IDM speed/visual parity remains open. [Details](docs/completion-close-9.87.md).

## 9.86 / extension 0.73.50 — uninstalled

Keep explicit completion requests and publication/finished handoffs responsive while the catalog is busy. Retain ordered intents, coalesce pending duplicates and read current eligibility/path/size before display. A cancellable worker wakes the UI when model access is available; CLI Exit retries on that wake instead of waiting for the main timer. Five native programs build as 9.86 and both source manifests advance to 0.73.50. The frozen routes block for 547–563 ms; current dispatches return before writer release. All 227 backend and 368 final UI assertions pass, and a normal 4 MiB download verifies its hash, saved catalog and graceful exit. The candidate is uninstalled; IDM speed and visual parity remain open. [Details](docs/completion-admission-9.86.md).

## 9.85 / extension 0.73.49 — uninstalled

Save the completion dialog’s “Don’t show this dialog again” preference off the UI thread. Own the accepted save after dialog Close, merge only its field into current settings, retain rollback/error reporting, and defer main-window shutdown until acknowledgement. Five native programs build as 9.85 and both source manifests advance to 0.73.49. The frozen checkbox blocks for 563 ms; current handlers return in 0–16 ms while the writer remains held. All 214 focused backend and 340 UI checks pass. This does not establish first-paint or whole-download speed parity; the candidate is uninstalled. [Details](docs/completion-suppression-9.85.md).

## 9.84 / extension 0.73.48 — uninstalled

Keep completion Open/Open with/Open folder responsive while the catalog is busy. Queue one validated action, preserve Close and cancel pending actions on destruction; retain current Unicode destinations and scanner eligibility. Five native products build as 9.84 and both source manifests advance to 0.73.48. The frozen command blocks for 579 ms; current commands return in 0–16 ms while the writer remains held, with all 42 contention checks passing. The focused backend run passes 197 checks; all 274 retained-reference UI checks also pass. This is handler responsiveness, not whole-download speed parity. Desktop control captures IDM after activation, but Windows reports it unresponsive and its menu does not react; fresh paired timing and installed acceptance remain open. [Details](docs/completion-actions-9.84.md).

## 9.83 / extension 0.73.47 — uninstalled

Fix uninterrupted live DASH recording when a refreshed manifest retires the current chapter. Check verified adjacent transitions before rematching expired IDs; retain durable boundary/session ownership. Five native programs build as 9.83; both manifests advance to 0.73.47. All 1,601 backend assertions pass. Eight HTTP workflows qualify three-Period AV/video/audio, old-job Resume, held-refresh Save and three correct rejections. Frames, audio packets, decoded samples and timestamps match their sources. Remove 1,187 disposable files after sanitized export. Installed files and the IDM speed/UI gates remain unchanged; non-aligned chapter ends remain open. [Details](docs/continuous-dash-9.83.md).

## 9.82 / extension 0.73.46 — uninstalled

Resume fully retained live DASH chapters whose duration was originally unknown after they disappear from the manifest. Require contiguous verified coverage in every track and a fresh explicit adjacent successor; checkpoint the boundary safely across restart. Five native programs build as 9.82; both manifests advance to 0.73.46. All 1,595 backend assertions pass. Nine HTTP workflows verify four complete 20-frame outputs and five preserved rejection cases, including interrupted-revision recovery. Remove 1,118 disposable files after sanitized export. Installed files and the open IDM speed/UI gates remain unchanged. [Details](docs/retired-dash-9.82.md).

## 9.81 / extension 0.73.45 — uninstalled

Remove a duplicate full-body disk read when publishing a response after approved prefix removal. Compute the raw server SHA-256 during the existing copy while separately hashing the final output; keep checksum rejection, source protection, flush and publication barriers. Five native programs build as 9.81; both manifests advance to 0.73.45. All 1,559 backend assertions, 274 current HTTP workflow assertions and 28 frozen 9.80 control assertions pass. Exact pre-Start and final sizes match for 395-byte and 4,560,015-byte outputs. Remove 1,059 temporary files after sanitized export; retain current products, resource and 42 core objects. Installed files remain unchanged. Desktop control still fails to initialize and IDM remains running in the current session, so no fresh IDM timing or overall speed-parity claim follows. [Details](docs/prefix-checksum-9.81.md).

## 9.80 / extension 0.73.44 — uninstalled

Enforce the selected server SHA-256 on plain form responses and retain hashing/checksum collection through form-to-GET redirects. Preserve cached-response validation without resubmitting forms, and distinguish no-content representation metadata from body checksums. Five native programs build as 9.80; both manifests advance to 0.73.44. All 1,559 backend assertions and 278 form-workflow checks pass. Frozen controls reproduce seven fresh plain mismatches, a redirected gzip mismatch and a cached-response checksum bypass; current legacy and encoded-byte controls pass. Remove 1,222 disposable files after sanitized export. Installed files remain unchanged, and the overall speed/UI/IDM gates stay open. [Details](docs/post-checksum-9.80.md).

## 9.79 / extension 0.73.43 — uninstalled

Preserve the original volume lookup for UNC and mapped remote routes without the preliminary attribute open introduced by 9.78. Keep guarded local GUID queries, handle closure and all publication/durability barriers. Five native programs build as 9.79; both manifests advance to 0.73.43. All 1,559 assertions in 23 suites, 27 additional C: checks, twenty read-only API comparisons and two normal C:/D: 4 MiB completion/hash/post-exit-flush smokes pass. Remove both payloads and 908 disposable test/build/catalog files, including six superseded 9.78 artifacts. Live SMB and the earlier full-lifetime speed gate remain open; installed files stay unchanged. [Details](docs/network-volume-route-9.79.md).

## 9.78 / extension 0.73.42 — uninstalled; speed gate failed

Use a guarded opened-file volume query instead of repeating a full-path walk when a valid GUID root is available; retain legacy fallback and every durability/publication barrier. Five native products build as 9.78; both manifests advance to 0.73.42. All 1,551 assertions in 23 suites and nineteen additional C: checks pass. Twenty read-only comparisons show a faster helper, but the six normal-product pilot downloads retain total/finishing losses, so the speed gate fails and the candidate stays uninstalled. All payloads and 278 disposable fixture/build files are removed; current products/core objects and protected installed data remain. [Details](docs/fixed-volume-9.78.md).

## 9.77 / extension 0.73.41 — uninstalled

Fix fresh-process live DASH Resume after a completed retained Period has disappeared from the server MPD. Refresh and plan its adjacent compatible continuation before rematching old IDs; retain rejection of incomplete coverage and gaps, preserve ordinary waiting, and honor Save during a canceled refresh. Five native products build as 9.77; both manifests advance to 0.73.41. All 1,532 assertions in 22 affected suites pass. Frozen 9.76 controls reproduce the failure; 9.77 reuses the same retained bytes and completes both simple and moving-pattern video. All twenty distinct moving frames match the original, with 211,911 exact assembled/final bytes. Still-visible continuation and held-refresh Save also pass; both rejection controls preserve their cached files. Installed files and the unresolved IDM speed gate remain unchanged. [Details](docs/live-dash-continuation-9.77.md).

## 9.76 / extension 0.73.40 — uninstalled

Carry the actual hashed DASH assembly into publication through a held source owner, including cached output and the confirmation wait. Replace the redundant session Period hash with a check against the continuously protected object and pinned name. Rewind shared file position before cross-volume copying. Five native products build as 9.76; both manifests advance to 0.73.40. All 1,532 assertions in 22 affected suites, sixteen DASH ownership/copy checks and 88 media-workflow checks pass, including nine complete EOF decodes. A frozen 9.75 control reproduces four protection/publication failures. Keep the installed app unchanged; full live multi-Period acceptance and the finishing-speed gate remain open. [Details](docs/dash-output-ownership-9.76.md).

## 9.75 / extension 0.73.39 — uninstalled

Keep the actual verified video protected through publication by acquiring an identity-matched reader before releasing its verification handle. Pass that ownership through YouTube/adaptive assembly and retained-media reuse. Frozen 9.74 controls reproduce modified/replaced-file acceptance; corrected 9.75 passes all 1,532 assertions in 22 affected suites, six retained-source checks and 84 media-workflow checks including eight complete EOF decodes. Five native products carry 9.75; both source manifests advance to 0.73.39. No new request, remux, full-file hash or flush barrier is added to the ordinary handoff. Installed files remain unchanged, the finishing-speed gate remains open, and no whole-app IDM parity claim follows. [Details](docs/verified-media-handoff-9.75.md).

## 9.74 / extension 0.73.38 — unpromoted

Release completed advisory hashing storage inside its owned worker and include deallocation in its recorded task time. The real split owner median wait falls from 3.0809 ms to 0.0237 ms; buffer release remains counted at about 2.83 ms. Five native products build; 1,522 affected assertions, 119 cache checks and 326 standalone lifetime checks pass. All eighteen normal-product downloads verify, but the complete no-regression gate fails, including slower sequential exit medians. Keep the candidate uninstalled and preserve every losing sample. Both source manifests advance to 0.73.38; there is no new IDM comparison or whole-app parity claim. [Cause, all measurements and cleanup](docs/advisory-buffer-release-9.74.md).

## 9.73 / extension 0.73.37 — unpromoted

Built unpromoted candidate: transfer the existing budgeted buffer and hash state into an owned advisory task, overlap it with an identity-bound durable intent, and join it before final catalog acceptance. Reject required/known-digest misuse and prevent a stale recorded checksum from supplying the pending intent digest. Five native products build; 1,522 affected assertions, 119 cache checks and seventeen HTTP checksum checks pass. All eighteen normal-product transfers verify, but the matched no-regression gate fails: median 4 MiB and split finishing regress against 9.71. Keep the installed app unchanged; no new IDM comparison or parity claim follows. Source manifests advance to 0.73.37. [Implementation, failures and evidence](docs/advisory-intent-9.73.md).

### Intermediate 9.72 / browser 0.73.36

Introduced the value-owned advisory handoff and passed 1,506 affected assertions. Superseded without installation by 9.73 after review found stale-checksum and required/known-digest policy gaps. Seven exact source snapshots and raw build/test records remain.

## 9.71 / extension 0.73.35

Bind relocated publication to the actual opened source and retain source/stage objects through cleanup. Guard the writer-to-reader handoff and require a live reference for legacy identity deletion. Build five native products; pass 1,458 assertions in twenty affected suites, 66 cross-volume relocation checks, five scoped-handoff checks, twenty fallback identity checks and 43 gzip workflow checks. The separate checks overlap registered cases. Seven response-reuse routes each make one request. Source manifests advance to 0.73.35. No installation or new IDM speed claim follows. [Changes, qualifications and evidence](docs/relocation-ownership-9.71.md).

### Intermediate 9.70 / browser 0.73.34

Introduced scoped source adoption and identity-aware relocation cleanup. Built and validated, then superseded without installation by 9.71 to keep referenced objects alive through cleanup.

## 9.69 / extension 0.73.33

Create decoded and prefix-stripped gzip outputs exclusively, and bind each cleanup to the identity captured from its actual new-file handle. Preserve pre-existing items, replacement files/directories and unused stripping paths while retaining encoded recovery bytes. Build five native products and pass 1,425 assertions in nineteen affected suites, plus 57 C-volume ownership checks and 43 gzip workflow checks. Validate exact decoded size before Start, saved completion, destination changes, Download Later reuse and encoded-offset resume; all seven one-response routes still use one request. Source extension manifests advance to 0.73.33. No installation, live-browser claim or new IDM timing claim follows.

See [gzip ownership, workflow and request-count validation](docs/gzip-stage-ownership-9.69.md).

## 9.68 / extension 0.73.32

Bind ordinary transfer cleanup to the created file object, preserving an equal-byte replacement or empty folder at its old staging name. Capture copy/assembly identity on the exclusive new-file handle; use verified partial-file identity for failed hard-link cleanup, and join/release unhanded writeback before deletion. Build five native products and pass 1,351 assertions in seventeen affected suites, plus 36 cross-volume interruption checks and four successful copied-download checks. Source extension manifests advance to 0.73.32. Installed files and the unresolved IDM performance gates are unchanged.

See [created-stage ownership and validation](docs/transfer-stage-cleanup-9.68.md).

## 9.67 / extension 0.73.31

Refuse unclaimed staging files/directories with exclusive output creation, and preserve them on early checksum failure. Reuse a leftover hard link only after proving it names the current partial file and deleting through the checked handle. The same real HTTP controls reproduce foreign-item loss on 9.65 and the owned-link refusal in the incomplete 9.66 candidate; corrected 9.67 controls pass. Build five native products and pass 1,333 assertions in sixteen affected suites, plus eighteen C-volume checks. Source manifests advance to 0.73.31. No installation or new IDM speed claim follows.

See [staging ownership and recovery validation](docs/staging-admission-9.67.md).

## 9.66 / extension 0.73.30 (intermediate, not released)

Replace destructive staging-name reuse with exclusive creation, and stop cleanup of a name the transfer never created. Twelve foreign-item checks passed, but owned hard-link recovery needed completion. Its full build was stopped; 9.67 supersedes this uninstalled candidate.

## 9.65 / extension 0.73.29

Bind failed prepared-stage cleanup to the original stable file identity and delete through its checked handle, preserving a replacement file or directory at the old name. Reuse the pre-flush snapshot on the async path; retain recovery journals and joined-worker ordering. The same component and fully frozen real HTTP controls reproduce data loss on 9.64 and pass on 9.65. Five native products build; all 1,315 assertions in fourteen affected suites pass. Source extension manifests advance to 0.73.29. No new speed claim or installation follows; the existing IDM performance gaps remain open.

See [failure cleanup ownership and retained gaps](docs/prepared-cleanup-owner-9.65.md).

## 9.64 / extension 0.73.28

Keep the prepared owner alive through failed publication cleanup: reset its in-place scope and join owned body work before failure status or active ownership retires. A same-fixture real HTTP control fails on frozen 9.63 and passes twelve lifetime assertions on 9.64. Build five native products and pass 1,289 affected checks. Successful I/O policy is unchanged; the prior speed gaps remain open and no new speed claim or installation follows. Source manifests advance to 0.73.28; installed 8.68 is unchanged.

See [failure lifetime and retained performance gaps](docs/publication-failure-owner-9.64.md).

## 9.63 / extension 0.73.27

Capture source metadata and stable identity on the writer-excluding file object before asynchronous flush, avoiding the blocking query found in 9.62. Bind known-hash recovery to that captured identity and retain post-join validation. Build five 9.63 native products; pass 1,277 affected checks. Three clock traces prove actual intent/body overlap, and all 27 normal comparison files verify. Sequential/split finishing regressions and IDM first availability remain open; the candidate is not promoted for installation. Source manifests advance to 0.73.27; installed 8.68 is unchanged.

See [diagnosis, implementation and every remaining performance gap](docs/publication-source-snapshot-9.63.md).

## 9.62 / extension 0.73.26

Keep one writer-excluding file object through owned asynchronous final body writeback and durable publication, scheduling owned asynchronous body writeback without the rejected 9.61 full-file rehash; later traces show a source query still serialized intent preparation. Build five native 9.62 products; pass 1,275 affected assertions. All 27 downloads verify. Median split finishing improves by 26.9 ms against 9.60, but total-time and sequential regressions remain; IDM first availability still wins all nine pairs. Neither the no-regression gate nor full parity passes. Source manifests advance to 0.73.26; installed 8.68 is unchanged.

See [implementation, measurements and open gates](docs/publication-owned-pin-9.62.md).

## 9.61 / extension 0.73.25 — rejected candidate

An owned early-intent prototype uses a metadata snapshot, strict promotion and full rehash. Five products and 1,273 assertions pass, but all nine finishing pairs lose against 9.60. Do not install/promote this approach as a speed fix. All observations and the initial compile failure are preserved. See [rejected candidate](docs/publication-overlap-9.61.md).

## 9.60 / extension 0.73.24

Release ordinary publication file pins after the exact completed catalog proof is accepted, before journal cleanup, while retaining publication ownership and directory protection through cleanup. Keep pre-acceptance write/rename exclusion and all existing flush/checkpoint work. Build five native 9.60 products and pass 994 affected assertions. All 27 matched download outputs verify correctly. The IDM first-availability gate and the all-pairs no-regression gate against 9.59 still fail, despite faster process-exit medians. Source browser manifests advance to 0.73.24; installed 8.68 remains unchanged.

See [qualification and failed performance gates](docs/publication-availability-9.60.md).

## 9.59 / extension 0.73.23

Fix caught startup failures for silent command-line requests: report to inherited error output when available and exit with status 1 instead of waiting on an error dialog. Validate instance tags before opening a catalog, while preserving quiet-looking option values and successful/recovery exit behavior. Build five native 9.59 products; pass 199 backend and 40 real App launch assertions. Source browser manifests advance to 0.73.23. New 9.58-versus-IDM observations win all nine whole-process and output-durability pairs but lose six first-availability pairs; full parity remains open. Installed 8.68 is unchanged.

See [qualification and measured parity gaps](docs/silent-launch-9.59.md).

## 9.58 / extension 0.73.22

Open standalone Browser Integration before waiting for shared settings, with cancellable background preparation, save its owned draft on a joined worker, and merge only edited browser fields into current settings. Nested menu and panel edits preserve live sibling values; unchanged effective defaults do not write the catalog. The held-model opening observation improves from 2535.8 ms on frozen 9.57 to 48.5 ms on 9.58. Build five native products and pass 292 backend plus 80 native UI assertions. Source browser manifests advance to 0.73.22. Installed 8.68 remains unchanged; installed IDM parity and the earlier total-time gate stay open.

See [qualification and scope](docs/browser-integration-9.58.md).

## 9.57 / extension 0.73.21

Move toolbar size, captions, looks, visibility, skin selection, layout persistence and Reload onto the owned appearance worker. Merge only the changed fields, prepare immutable skin pixels outside the catalog lock, render without reopening skin files, and restore the current layout on failure without a second model read. The held-model toolbar command returns in about 22 ms versus 2.56 seconds on frozen 9.56. Build five native 9.57 products and pass 251 backend plus 1,293 optimized UI assertions. Browser source manifests advance to 0.73.21. Installed 8.68 remains unchanged; installed IDM parity and the prior total-time gate remain open.

See [qualification and scope](docs/toolbar-settings-9.57.md).

## 9.56 / extension 0.73.20

Move dark mode, category visibility, font selection/reset and tray-style saves onto an accepted worker with narrow drafts and fresh-field merges. Render acknowledged values without reading the shared catalog again; preserve live unrelated settings, rollback/retry, guarded controls and deferred Close/Exit. The held-model dark command returns in about 5 ms versus 2.54 seconds on frozen 9.55. Build five native 9.56 products and pass 218 backend plus 1,420 optimized UI assertions. Source browser manifests advance to 0.73.20. Installed 8.68 remains unchanged; installed IDM parity and the prior total-time gate remain open.

See [qualification and scope](docs/appearance-command-9.56.md).

## 9.55 / extension 0.73.19

Reuse the main font and skip redundant native font-change messages when its face, pixel height and weight are unchanged. Repair detached control bindings, prepare replacements before releasing the old font, and retain the previous valid font on creation failure. Build all five native 9.55 products; pass 187 backend and 910 optimized UI assertions. Source browser manifests advance to 0.73.19. Installed 8.68 and extension files 0.72.32 remain unchanged; installed IDM parity and the prior total-time gate remain open.

See [qualification and scope](docs/font-reuse-9.55.md).

## 9.54 / extension 0.73.18

Cache built-in toolbar sources and bounded size/style/DPI rasters; reuse the active native normal, hot and disabled image lists on unchanged repaints. Keep custom skins fresh, make Reload invalidate built-in assets explicitly, and preserve current image handles if regeneration fails. Build five native 9.54 products and pass 187 backend plus 814 optimized UI assertions. Source browser manifests advance to 0.73.18. Installed 8.68 and extension files 0.72.32 remain unchanged; installed IDM parity and the prior total-time gate remain open.

See [qualification and scope](docs/toolbar-cache-9.54.md).

## 9.53 / extension 0.73.17

Use owned appearance snapshots for Windows system-theme and DPI repaints, with a nonblocking settings read and fallback to the last rendered state. Keep font, toolbar and layout reads coherent while leaving ordinary preference commands on their existing settings path. Correct the busy-resize fallback to inspect the real toolbar window style. Controlled system-color handling improves from 2,503.01 ms on frozen 9.52 to 15.48 ms. Build five native 9.53 products; pass 154 backend and 1,336 UI assertions on child-attested desktops. Source browser manifests advance to 0.73.17. Installed 8.68 remains unchanged; installed IDM parity and the previous total-time gate remain open.

See [qualification and timing scope](docs/appearance-9.53.md).

## 9.52 / extension 0.73.16

Move Speed Limiter On/Off persistence off the UI thread. Preserve the live remembered rate, startup choice and limiter mode, update radio state only after a committed result, and keep local selection/sort/paint responsive. Reserve Close and explicit Exit until the accepted save is acknowledged; cancel an ordinary close through Show even after acknowledgement, and cancel deferred Exit on save failure for explicit retry. Controlled held-model command return improves from 2,612.99 ms on frozen 9.51 to 9.49 ms. Build five native 9.52 products; pass 154 backend and 864 native UI assertions on child-attested desktops. Source browser manifests advance to 0.73.16. Installed 8.68 remains unchanged; installed IDM parity and the earlier total-time gate remain open.

See [qualification and timing scope](docs/global-toggle-9.52.md).

## 9.51 / extension 0.73.15

Open Speed Limiter settings before waiting for the catalog, prepare its values on a cancellable worker, and save an immutable draft under an acceptance guard. Merge only edited limiter fields into live settings, preserving the current on/off state and mode. Controlled held-model opening improves from 2,542.2 ms on frozen 9.50 to 52.4 ms on 9.51. Build five native products; pass 134 backend and 581 UI assertions on child-attested desktops. Source browser manifests advance to 0.73.15. Installed 8.68 and the installed extension remain unchanged; installed IDM parity and the older total-time gate remain open.

See [qualification and timing scope](docs/global-limit-9.51.md).

## 9.50 / extension 0.73.14

Columns now opens before waiting for a catalog writer, loads saved hidden widths asynchronously, and saves an immutable draft without blocking the UI. Failed reads offer Retry; failed saves retain edits. Accepted saves cannot be abandoned by Cancel, and forced closure still applies a committed layout. Five native 9.50 products built; 102 backend and 1,346 UI assertions passed. Both source browser manifests advance to 0.73.14. Installed 8.68 remains unchanged. Historical desktop-isolation claims are corrected; fresh tests use child-attested native desktop assignment. Installed IDM parity and the prior total-time gate remain open.

See [qualification and timing scope](docs/columns-9.50.md).

## 9.49 / extension 0.73.13

- Remove blocking acquisition from list-layout bookkeeping for every caller. A held-model sort command takes 1,997.3 ms on frozen 9.48 and 0.31 ms on 9.49; all 11 sort choices return in 0.25–0.40 ms in the controlled fixture. Preserve latest widths, order and sort through timer catch-up or normal Exit before catch-up. Build five native 9.49 products and pass 77 backend plus 1,390 native UI assertions. Both source browser manifests advance to 0.73.13; installed 8.68 remains unchanged. The prior matched total-time gate and installed IDM parity remain open. [Evidence and remaining work](docs/layout-write-9.49.md).

## 9.48 / extension 0.73.12

- Avoid validating an unrelated selected download before dispatching Exit or local view commands. Preserve live row-action validation and selection-aware export. The same native Select All fixture blocks for 2,002.6 ms on frozen 9.47 and returns in 0.2 ms on 9.48 while its model writer is still held. Quick Filter returns in 27.5 ms and selected/deferred Exit in 12.3 ms. Build five native 9.48 products and pass 77 backend plus 526 native UI assertions, including all three real export entries and byte-exact resumed downloads. Both source browser manifests advance to 0.73.12; installed 8.68 remains unchanged. The prior 9.47 total-time gate and installed IDM parity remain open. [Evidence and remaining work](docs/command-dispatch-9.48.md).

## 9.47 / extension 0.73.11

- Replace timed Exit model-lock retries with direct acquisition on a Manager-owned worker. Cancel a destroyed window without waiting for that acquisition; join all retained workers before Manager's final checkpoint. Build five native 9.47 products and pass 867 backend plus 1,360 native UI assertions. In three rotating rounds, pre-unlock-marker-to-acquisition median changes from 15.82 ms in 9.46 to 0.03 ms in 9.47. Controlled total-cleanup medians are 839.9 / 864.4 / 843.1 ms for 9.45 / 9.46 / 9.47. The matched median total gate remains open; universal and installed IDM parity remain unproven. Both source extension manifests advance to 0.73.11; installed 8.68 remains unchanged. [Evidence and remaining work](docs/managed-exit-9.47.md).

## 9.46 / extension 0.73.10

- Prepare a contended Exit layout merge on an owned worker while preserving the immediate fast path and the existing final checkpoint. Pass 49 backend and 1,360 native UI checks, including retry, direct destruction and concurrent settings. Matched command-return median improves from 842.8 to 14.2 ms, but matched total teardown median regresses by 21.1 ms; the total-time performance gate remains open. Build five native 9.46 products and advance both source extension manifests to 0.73.10. Installed 8.68 remains unchanged. [Evidence and remaining work](docs/exit-layout-9.46.md).

## 9.45 / extension 0.73.9

- Keep a pending context menu from consuming a child dialog's Escape key, and retire pending requests when another command or row activation supersedes them. The same real-modal fixture fails on 9.44 and passes on 9.45. Build five native products; pass 30 focused backend and 509 native UI checks. A separate controlled Exit trace locates a 761.9 ms layout lock wait under a deliberate 750 ms catalog hold; that wait remains to be fixed. Both source extension manifests advance to 0.73.9. Installed 8.68 remains unchanged. [Evidence and remaining work](docs/menu-focus-9.45.md).

## 9.44 / extension 0.73.8

- Prepare the download context menu on one owned cancellable reader, retain native popup advice and queue choices, reject obsolete cohorts, and join before window destruction. Under a held writer the request returns in 0 ms at timer resolution instead of the 9.43 baseline's 2,016 ms; popup visibility still waits for a coherent read. Build five native 9.44 products; pass 84 focused backend and 480 native UI checks. Both source extension manifests advance to 0.73.8. Installed 8.68 remains unchanged; installed IDM timing is not claimed. [Evidence and remaining work](docs/download-menu-9.44.md).

## 9.43 / extension 0.73.7

- Correct the legacy menu fixture to acknowledge completed Resume before rendering the next batch, and require the intended active/completed native selection before Stop. All 458 menu checks pass, including isolated Stop, deferred Stop All, scheduler non-restart and three byte-exact resumed files. Build five native 9.43 products; advance both source extension manifests to 0.73.7. This is a validation/maintenance increment with unchanged download-engine behavior. Installed 8.68 remains unchanged. [Evidence and remaining work](docs/menu-acknowledgment-9.43.md).

## 9.42 / extension 0.73.6

- Keep selection-dependent menu advice consistent with the displayed row when Manager is busy. Capture runtime activity/prefetch flags with immutable rows; refresh supported selection actions from that cohort instead of retaining another selection's menu state. Keep live validation on invocation. Build five 9.42 products from 636 frozen product-build inputs; pass 90 focused backend and 867 current UI assertions. The legacy menu suite passes the former active-selection Stop failure, then stops after 435 assertions at: Selected Stop pauses transfer and closes only its progress window. Both source extension manifests advance to 0.73.6; installed 8.68 remains unchanged. [Evidence and remaining work](docs/selection-9.42.md).

## 9.41 / extension 0.73.5

- Keep valid requested history filters when queues/projects/categories are added, renamed or reordered. Fix the first-refresh mismatch where the tree selected a new queue but the list showed All Downloads. Retain deleted-node ancestor fallback and nonblocking busy reads. Build five 9.41 products from 634 frozen inputs; pass 68 focused backend and 35 current UI assertions. The legacy menu suite passes the previously failing queue-view assertion, then stops after 413 assertions at: Active selection enables Stop. Source extension manifests advance to 0.73.5; installed 8.68 remains unchanged. [Evidence and remaining work](docs/navigation-9.41.md).

## 9.40 / extension 0.73.4

- Keep the main window responsive while Close waits for a fresh close-to-tray policy. Coalesce repeated Close clicks; let Show cancel a pending hide; retain deferred automation review. Remove blocking settings reads from toolbar/category resizing and waiting-status layout. Build five 9.40 products from 632 frozen inputs; pass 452 targeted backend and 1,582 native UI assertions. Source extension manifests advance to 0.73.4; installed 8.68 remains unchanged. One initial GUI watchdog expiry remains unexplained despite three unchanged-binary passing repeats; an additional legacy menu suite fails a queue-view check. [Evidence and remaining work](docs/main-close-9.40.md).

## 9.39 / extension 0.73.3

- Refresh only the changed imported-file metadata slot instead of probing the whole batch. Reuse an existing preview's policy and preserve other rows; recreate a missing owner from fresh settings for just the selected index. Keep explicit Retry available after partial recovery. Build five 9.39 products from 629 frozen inputs; pass 876 targeted backend and 1,559 current UI assertions. Source extension manifests advance to 0.73.3; installed 8.68 is preserved. [Evidence and remaining work](docs/import-single-9.39.md).

## 9.38 / extension 0.73.2

- Reject registered clicks from disabled dialogs/controls, mismatched notification sources and destroyed children. Freeze batch command/change/list notifications during refresh and admission, while preserving Cancel and paint. Validate every refresh source identity before publishing any metadata, reject superseded results and retain inline Retry. Build five 9.38 products from 627 frozen inputs; pass 838 targeted backend and 1,480 current UI assertions. Source extension manifests advance to 0.73.2; installed 8.68 remains unchanged. [Evidence and remaining work](docs/batch-command-9.38.md).

## 9.37 / extension 0.73.1

- Prepare bulk and per-file batch preview requests on an owned cancellable worker. Preserve edited fields and explicit choices, invalidate stale metadata before waiting, and keep a per-file edit to one preview slot without reacquiring Manager. Build five 9.37 products from 625 frozen inputs; pass 796 targeted backend and 1,394 actual UI assertions. Source extension manifests advance to 0.73.1; installed 8.68 is preserved. [Evidence and remaining work](docs/batch-refresh-9.37.md).

## 9.36 / extension 0.73.0

- Prepare batch rows, request context and initial metadata on the cancellable worker, then show a virtual native list that renders visible cells. Preserve source identities through checks, sorting, filtering and cell updates. Build five 9.36 products from 623 frozen inputs; pass 752 targeted backend assertions across 30 C:/D: runs and 1,339 actual UI assertions. Source extension manifests roll to 0.73.0. Installed 8.68 remains unchanged. [Measurements, evidence and remaining work](docs/batch-virtual-9.36.md).

## 9.35 / extension 0.72.99

- Complete the batch loading-to-full-view transition by reapplying current anchors and palette and routing keyboard focus to the new file list. Anchor loading status/Retry/Cancel during browser-dialog resizing; retain window geometry, logical column widths and monotonic command IDs. Build five 9.35 products from 619 frozen inputs; pass 714 targeted backend assertions across 28 C:/D: runs and 1290 current UI assertions (261 transition, 90 opening/late metadata, 108 batch admission, 831 Main/Options). Ten authenticated selected files still complete with one transfer request each. Remove 36 owned fixture/compiler roots and 38 temporary build files (370.1 MiB logical bytes). Installed 8.68 and extension files 0.72.32 remain unchanged. [Evidence and remaining work](docs/batch-transition-9.35.md).

## 9.34 / extension 0.72.98

- Open batch loading controls before acquiring Manager, prepare settings/categories/queue labels on an owned cancellable reader, and rebuild the full view from one cohort. Refresh preview policy off the window thread, invalidate old request metadata before waiting, retain inline Retry and preserve destination/checkbox edits. Treat a submitted checkbox cohort as explicit intent so late HTML metadata cannot undo it after queue cancellation or save failure. Build five 9.34 products from 618 frozen inputs; pass 714 targeted backend assertions across 28 C:/D: runs, 90 opening/late-metadata, 108 batch-admission and 831 Main/Options UI assertions. Ten selected files still download completely with one authenticated transfer request each. Remove 35 owned fixture/compiler roots and 37 temporary build files (388.7 MiB logical bytes). Installed 8.68 and extension files 0.72.32 remain unchanged. [Evidence and remaining work](docs/batch-opening-9.34.md).

## 9.33 / extension 0.72.97

- Move batch queue-policy preparation and canonical admission/save/start off the window thread. Freeze one owned selection and complete browser request context, allow cancellation before acceptance, retain accepted-save ownership through acknowledgement, preserve inline errors/retry and reuse the prepared queue cohort. Build five 9.33 products from 615 frozen inputs; pass 682 targeted backend assertions across 26 C:/D: runs, 108 actual batch-control assertions and 831 Main/Options assertions. Ten selected authenticated sample files download completely with one transfer request each. Remove 32 owned fixture/compiler roots and 34 temporary build files (332.5 MiB logical bytes). Installed 8.68 and extension files 0.72.32 remain unchanged. [Evidence and remaining work](docs/batch-admission-9.33.md).

## 9.32 / extension 0.72.96

- Open the generic queue selector before its catalog read completes, with cancellable background preparation, one names/labels cohort, inline failure and Retry. Keep editing disabled until ready, retain locally created queue choices and Schedule, and give both prepared and generic selectors an explicit Cancel button. File Info keeps its existing prepared path and canonical command validation. Build five 9.32 products from 612 frozen repository inputs; pass 602 targeted backend assertions across 24 C:/D: runs, 81 actual selector-control assertions and 831 Main/Options assertions. Remove 55 owned fixture/compiler roots and 69 temporary build files (462.4 MiB logical bytes). Installed 8.68 and extension files 0.72.32 remain unchanged. [Evidence and remaining work](docs/queue-choices-9.32.md).

## 9.31 / extension 0.72.95 - superseded native build

- Builds five products and passes 602 targeted backend assertions. The standalone selector test fails compilation with MFC/Winsock2 header-order error C1189 before a UI fixture starts. Correct test header order in 9.32, retain the failure and exclude its later mixed-version preflight from qualification. Not installed or qualified.

## 9.30 / extension 0.72.94

- Prepare File Info's Start session check and Download Later queue-policy lookup on an owned cancellable reader. Keep fields frozen and Cancel available, preserve inline errors and retry, and open the queue selector from one prepared names/labels cohort. Start skips queue preparation; a suppressed prompt stays suppressed. Canonical save workers retain authority. Build five products from 609 frozen inputs; pass 570 targeted backend assertions across twenty-two C:/D: runs and 1700 UI assertions (831 Main/Options, 257 pending, 327 presentation, 46 first-paint and 239 wizard). Remove 79 owned fixture/compiler roots and 102 temporary build files (833.7 MiB logical bytes). Installed 8.68, catalogs and shared helpers remain unchanged. [Evidence and remaining work](docs/file-info-actions-9.30.md).

## 9.29 / extension 0.72.93 - intermediate native build

- Builds five products and passes 570 backend checks. Stop the UI compile before execution after source inspection shows its expected page omits the required session-binding fragment. Correct marker validation in 9.30; not installed or qualified.

## 9.28 / extension 0.72.92 - superseded candidate

- Builds five products and passes 570 backend checks. Its Main run stops after 520 assertions when session recovery is cancelled before requiring the asynchronous page event; its URI expectation also omits the binding fragment. Preserve that failure; wait for the real callback and validate its binding marker before cancelling in 9.30. Not installed or qualified.

## 9.27 / extension 0.72.91

- Move listed/revised scheduling eligibility, original download/queue snapshots and initial fields onto the wizard's owned preparation task. Main dispatch uses a strong cached selection without waiting for command-state or deferred-selection validation; the worker and final save revalidate current state. Retain original UTC windows, draft comparisons, inline errors, Cancel and shutdown ownership. Build five products from 607 frozen inputs; pass 536 targeted backend assertions across twenty C:/D: runs and 1621 UI assertions (752 Main/Options, 257 pending, 327 presentation, 46 first-paint and 239 wizard). Remove 52 owned fixture/compiler roots and 65 temporary build files (666.0 MiB logical bytes). Installed 8.68, catalogs and shared helpers remain unchanged. [Evidence and remaining work](docs/listed-schedule-9.27.md).

## 9.26 / extension 0.72.90 - superseded candidate

- Builds five products and passes 536 backend plus 239 wizard assertions. Main stops at assertion 569 before the new commands because the fixture attempts row selection while its frame is hidden. Preserve that failure; show the owned frame and wait for row convergence in 9.27. Not installed or fully qualified.

## 9.25 / extension 0.72.89

- Open the Schedule wizard before catalog acquisition. Prepare categories, filtered folder history, category policy, queue-name advice and File Info's connection/speed defaults on an owned cancellable reader. Keep navigation disabled until ready, retain explicit Retry and shutdown ownership, and use the snapshot for editor actions while final saves revalidate current state. Build five products from 607 frozen inputs. Pass 512 targeted backend assertions across twenty C:/D: runs and 1539 UI assertions (738 Main/Options, 257 pending, 327 presentation, 46 first-paint and 171 wizard). Remove 31 owned fixture/compiler roots and 46 temporary build files (423.2 MiB logical bytes). Installed 8.68, catalogs and shared media helpers remain unchanged. [Evidence and remaining work](docs/schedule-preparation-9.25.md).

## 9.24 / extension 0.72.88 - incomplete candidate

- Introduces asynchronous schedule preparation. A malformed saved-login fixture fails before its first assertion; stop the owned partial build before product compilation. Preserve the failed preflight and frozen inventory; no complete build or installation is claimed. Correct the fixture through the normal saved-login constructor in 9.25.

## 9.23 / extension 0.72.87

- Show a matching prepared snapshot's read-only address, destination, category, size provenance and server filename on File Info's first paint while an owned worker revalidates current state. Keep Start/Later/Schedule/name adoption disabled until revalidation and preserve Cancel. Pass 470 targeted backend assertions over eighteen C:/D: runs and 738 Main/Options, 257 pending, 327 presentation and 46 first-paint assertions. Build five products from 605 frozen inputs; source extension manifests are 0.72.87. Remove 26 owned fixture/compiler roots and 28 temporary build files (335.1 MiB logical bytes), preserving current products/cores and 9.22 products. Installed 8.68, catalogs and media helpers remain unchanged. The first-paint fixture verifies advisory hints under a held model and name adoption only after revalidation. Keep the earlier missing-tools failures and 9.22 diagnostic separate. [Evidence and remaining work](docs/first-paint-hints-9.23.md).

## 9.22 / extension 0.72.86

- Move File Info's Use server name preparation to an owned advisory worker; release Manager before filesystem checks, reject obsolete display/draft results and preserve manual folders and custom remember rules. Defer Start/Later and drain Cancel safely. Build five products; pass 388 targeted backend assertions across fourteen C:/D: runs and 738 Main/Options, 257 pending, 327 presentation assertions. Retain the superseded 9.21 fixture failure separately. Remove 22 owned fixture roots and 50 temporary build files (466.6 MiB logical bytes). Installed 8.68, catalogs and helpers remain unchanged. The optional first-paint size-label diagnostic, other synchronous actions and live parity comparisons remain open. [Evidence](docs/server-filename-9.22.md).

## 9.21 / extension 0.72.85 — superseded candidate

- Introduces asynchronous server-name adoption. The redirect-category test uses an empty extension list and stops at assertion 9. Five products build, but the candidate is not qualified or installed; the explicit ZIP fixture is corrected in 9.22.

## 9.20 / extension 0.72.84

- Keep Save To's immediate writes responsive: category Browse, remember-path, server date, temporary Browse and Use default capture owned commands and save on the worker. Acknowledge only affected preferences/baselines and refresh only their controls; preserve other unfinished drafts and uncollected text. Retain accepted-write ownership through errors, with rollback and explicit retry.
- Build five products; pass 308 targeted backend and 1216 optimized UI assertions (632 Main/Options, 257 pending, 327 presentation). Keep the older all-static-label /Od failure separate from qualification. Remove 35 owned fixture roots and 80 temporary build files (972.4 MiB logical bytes); preserve installed 8.68 and personal data. [Evidence and remaining work](docs/immediate-options-9.20.md).

## 9.18–9.19 / extensions 0.72.82–0.72.83 — superseded candidates

- 9.18 introduces owned immediate Save To commits and passes 308 backend checks. Preserve its older /Od label failure, single-refresh presentation failure and intervened 622-assertion Main failure. The synthetic warning's actual OK button has ID 2; its driver incorrectly sent hard-coded IDOK.
- 9.19 corrects warning acknowledgement and drives the asynchronous receipt wave. Main/Options passes all 632 assertions and pending File Info passes 257, but presentation still requires IndividualStart after the authenticated file has already completed. Its exact terminal payload and cleared flag are recorded. Parallel native entry compilation records C1060 heap failures; only a verified partial core/output cache is retained. Neither candidate is an installed or qualified release.


## 9.17 / extension 0.72.81

- Open category properties from Main and Options without a caller settings wait. Return the owned accepted snapshot to File Info/Options; keep category draft/baseline fields coherent without rereading Manager and preserve unrelated unfinished Options changes. Category commands skip unrelated download-selection validation; model-backed menu advice preserves its displayed state during a busy snapshot. Download commands keep their existing authoritative checks.
- Build five products; pass 260 targeted backend and 1099 UI assertions (515 Main/Options, 257 pending, 327 presentation). Remove 29 owned test roots and 59 temporary build files (922.7 MiB logical bytes); preserve installed 8.68 and personal data. [Evidence and remaining work](docs/category-callers-9.17.md).

## 9.14–9.16 / extensions 0.72.78–0.72.80 — superseded candidates

- 9.14 returns owned category settings through File Info and Options and removes caller pre-reads. Its native/backend, pending and presentation checks pass, but Main stops at an unrestored hidden-window row setup. Preserve the original failure; this candidate is not a qualified release.
- 9.15 restores visible Video navigation in an isolated /Od driver but fails entry checks without enough control-readiness diagnostics. 9.16 adds nonblocking model-backed menu advice and two held-writer menu checks; its diagnostic identifies a timer observing the second category window before its controls exist. Neither has a normal product build. The corrected 9.17 driver waits for visible, constructed controls; all diagnostic results are excluded from optimized qualification.


## 9.13 / extension 0.72.77

- Open File Info category properties immediately, load settings off the window thread and permit cancellation before the catalog is readable. Save owned drafts asynchronously, reject duplicate OK/Close during accepted work and return the fresh category list without a parent model wait.
- Preserve correction drafts after failed validation and retain concurrent unrelated preferences. Build five products; pass 260 targeted backend and 1044 UI assertions (460 Main, 257 pending, 327 presentation). Remove 13 owned test roots and 22 temporary build files (303.9 MiB logical bytes); preserve installed 8.68 and personal data. [Evidence and remaining work](docs/category-properties-9.13.md).

## 9.12 / extension 0.72.76

- Keep File Info category changes responsive while settings are busy. Coalesce rapid choices, preserve late filename/folder/rule edits and defer Start/Download Later until the selected folder is ready. Cancel drains owned work before releasing Manager.
- Keep advisory selection and menu preference updates from waiting on Manager; actual command execution retains authoritative validation. Correct the native CCmdUI test index bounds.
- Build five native products; pass 222 targeted backend and 1,026 UI assertions (442 Main, 257 pending, 327 presentation). Remove 27 owned test roots and 58 temporary build files. Installed 8.68, personal catalogs and media helpers remain unchanged. [Evidence and remaining work](docs/file-info-category-9.12.md).

## 9.10–9.11 / extensions 0.72.74–0.72.75 — superseded candidates

- 9.10 introduces the owned category reader and draft merge; its Main workflow fails at the earlier held-content warning despite passing targeted backend and the other UI suites. 9.11 changes advisory reads; its diagnostic menu probe omits CCmdUI index bounds and exits without assertion results. A separate corrected diagnostic probe passes. Preserve both candidates separately from optimized 9.12 qualification; neither is an installed release.


## 9.9 / extension 0.72.73

- Restore qualification of the older default native browser-presentation workflow with owned asynchronous Save, receipt acknowledgement, Resume/login and network-session interfaces. Retain stale/undisplayed capture checks, partial bytes, exact-size states and a complete authenticated loopback download.
- Build five products; pass 327 optimized presentation checks and 122 focused backend checks. Keep the 9.2 broad baseline separate. Clean 11 owned fixture roots and 32 temporary build files; preserve installed 8.68 and personal catalogs. [Evidence and remaining work](docs/presentation-workflow-9.9.md).

## 9.3–9.8 / extensions 0.72.67–0.72.72 — superseded diagnostic drafts

- Preserve the compile failure and all original assertion failures while migrating the driver: removed network-session members, the old Queued-only Start expectation, synthetic size fixture state and missing asynchronous command acknowledgement. The 9.8 diagnostic identifies a held decision guard with an unlocked model; 9.9 pumps and acknowledges that owned command. These /Od fixture drafts are not native-product releases or optimized qualification.

## 9.2 / extension 0.72.66

- Keep parent File Info responsive after address/media recovery. Prepare stop, saved URL/page and login values on an owned worker, retain draft destinations, and guard subsequent admission with the exact saved request selection. Cancel drains accepted work before owner release.
- Build five products; pass 3,334 backend and 698 actual UI assertions. Clean 78 fixture roots and 87 temporary build files; preserve installed 8.68 and personal catalogs. [Evidence and remaining work](docs/file-info-refresh-9.2.md).

## 9.1 / extension 0.72.65

- Open ordinary/FTP/media address recovery before Manager waits. Serialize preparation, browser-offer reads, reviewed saves and displayed receipt cleanup on one owned worker. Keep Cancel and repainting responsive, preserve accepted saves and reject undisplayed replacement changes.
- Build five native products; pass 3,270 backend and 678 actual UI assertions. Remove 78 owned fixture roots and 119 temporary build files. Installed UDM remains 8.68. [Evidence and remaining work](docs/download-refresh-9.1.md).

## 9.0 / extension 0.72.64 — superseded development draft

- The focused shared-helper preflight passes 33 checks, but UI compilation rejects protected Form caption access. Retain that failure, correct title-setting through SetWindowText and rebuild fresh as 9.1. No installation or qualification claim is made for 9.0.

## 8.99 / extension 0.72.63

- Deliver owned ZIP completion messages after local parsing, remote reads and prefetched-file readiness. Publish results before notification, isolate callback exceptions and keep callbacks safe after window teardown.
- Verify the real ZIP list updates with its polling timer removed. Build five products; pass 3,204 backend and 657 actual UI assertions.
- Clean 148 owned fixture directories and 176 temporary build files, including six displaced cache objects. Preserve previous native products, installed 8.68 and personal catalogs. [Evidence and remaining work](docs/zip-completion-8.99.md).

## 8.98 / extension 0.72.62 — superseded development candidate

- Build all native products and pass 3,204 backend assertions. Preserve the failed Main UI visibility assertion: a child HWND can be observed before modal initialization has shown it. Retain the held-writer and deadline requirements, wait for actual visibility and qualify 8.99 from fresh inputs.

## 8.97 / extension 0.72.61

- Restore ordinary ZIP Preview after admission settles, using the current eligibility snapshot. Open ZIP preview before model reads or prefetch stopping. Prepare selected requests and private credentials off the UI thread; retain a responsive closing window until accepted preparation settles.
- Preserve restoration selection across capacity/decision retries; reject changed routing and membership before launch, stop or restoration. Keep active HTTP confirmation on its existing file reader.
- Build five products; pass 3,170 backend and 651 actual UI assertions. Remove 146 owned fixture directories and 216 temporary build files; preserve installed 8.68 and personal catalogs. [Evidence and remaining work](docs/zip-preparation-8.97.md).

## 8.96 / extension 0.72.60 — superseded development candidate

- Build five products and pass 3,170 backend assertions. Preserve the original and fresh same-binary Main UI failures: a one-refresh duplicate fixture expectation and an ordinary ZIP button left disabled after admission. Correct the visible-history fixture precondition, restore ZIP eligibility-driven enabling, add diagnostics and qualify 8.97 instead.

## 8.95 / extension 0.72.59 — superseded development candidate

- Real HTTP preflight reaches 32 successful assertions, then a selected stop rejects an empty no-redirect response address newly added by the transfer. Preserve the failed result, normalize only empty resolved-address metadata, retain nonempty address guards and qualify fresh 8.97 inputs.

## 8.94 / extension 0.72.58 — superseded development candidate

- Introduce asynchronous ZIP entry/preparation and selected stop authority. Source audit finds restoration retries could drop their guard; interrupt the incomplete build, preserve the audit, strengthen the retry ownership and positive workflow tests, and qualify 8.97 instead.

## 8.93 / extension 0.72.57

- Keep pending ZIP previews responsive while waiting for a prefetched archive. Read the receipt and file length on an owned cancellable worker, and revalidate the selected request, ownership and parts path before exposing the file.
- Build five products; pass 3,088 backend and 634 actual UI assertions. Clean 70 owned fixture directories and 80 temporary build files. Installed UDM remains 8.68. [Evidence and remaining work](docs/prefetched-zip-8.93.md).

## 8.92 / extension 0.72.56 — superseded development candidate

- Restore version/manifest UTF-8 encoding. Compilation then rejects private activity-map access in the new fixtures; use the existing public reserved-response fixture state and qualify the fresh 8.93 build instead.

## 8.91 / extension 0.72.55 — superseded development candidate

- Introduce the prefetched ZIP readiness reader. An encoding error in version files prevents qualification; diagnostics are retained with the final 8.93 evidence.

## 8.90 / extension 0.72.54

- Prepare File Info metadata asynchronously and wake it on actual HTTP completion. Cancel obsolete credential/selection checks; reject changed progress, completion, capture requirements and protected proxy routing before launch.
- Build five products; pass 3,046 backend and 628 actual UI assertions. Clean 68 owned fixture directories and 147 temporary build files. Installed UDM remains 8.68. [Evidence and remaining work](docs/file-info-preview-8.90.md).

## 8.89 / extension 0.72.53 — superseded development stage

- Add direct HTTP-completion notification. Preserve the 25-check preflight; source audit identifies missing final eligibility checks and the protected proxy field mismatch. Stop incomplete builds and qualify the corrections as 8.90.

## 8.88 / extension 0.72.52 — development stage

- Introduce owned cancellable preview preparation. Included in the final 8.90 implementation; no separate build qualification or release claimed.

## 8.87 / extension 0.72.51

- Keep More available during background admission now that its queue inputs are owned. Carry cold File Info preparation, responsive cancellation and a transition within the same window; ignore stale bootstrap commands and show known exact bytes before Start.
- Build five products; pass 2,984 backend and 617 actual UI assertions. Clean 129 owned fixture directories and 214 temporary build files, preserving current core cache, installed UDM 8.68 and personal catalogs. [Evidence and remaining work](docs/file-info-initial-8.87.md).

## 8.86 / extension 0.72.50 — superseded development candidate

- Introduce owned initial-view preparation, private address/login resolution, coherent queue snapshots and the deferred control transition. Backend validation passes, but UI qualification exposes the existing More admission restriction. Preserve the initial MFC compiler correction and failed UI evidence; use 8.87 for the qualified candidate. [Investigation](docs/file-info-initial-8.87.md).

## 8.85 / extension 0.72.49

- Queue browser presentation acknowledgements off Main’s UI thread; preserve fresh mixed-group captures and exact failed-attempt recovery through Retry/Close. Drain accepted work before shutdown.
- Build five products; pass 2,944 backend and 580 actual UI assertions. Preserve superseded fixture evidence, clean owned payloads and 178 temporary build files, and retain installed UDM 8.68 and personal catalogs. [Evidence and next work](docs/browser-acknowledgement-8.85.md).

## 8.84 / extension 0.72.48

- Keep accepted File Info attached through benign metadata/preference refreshes while rejecting changed selected requests and tracks. Preserve edited destination and description.
- Build five products; pass 2,886 backend and 566 actual UI assertions. Remove 61 fixture roots and 69 temporary build files; preserve installed 8.68 and personal catalogs. [Evidence and remaining work](docs/file-info-selection-8.84.md).

## 8.83 / extension 0.72.47

- Open already-accepted prepared media through the early File Info route; move job/receipt/current-view binding to an owned reader that can cancel its model-lock wait. Preserve drafts, retry identity and grouped acknowledgement ownership.
- Build five products; pass 2,852 backend and 556 actual UI assertions, including Main's real accepted-notification handler and pending/retry/cancellation workflows. Preserve initial fixture failures and the token-format correction proof; use existing media helpers without binary copies.
- Remove 63 fixture roots, 64 temporary build files and a source copy. The zero-copy helper junction remains because automatic approval review blocked removal. Installed UDM remains 8.68. [Evidence and remaining work](docs/file-info-binding-8.83.md).

## 8.82 / extension 0.72.46

- Fix a scheduler race that paused a manually requested download between prepared handoff and publication after its persistent IndividualStart flag was retired. Keep generation-bound runtime ownership through finishing, without another catalog write; explicit Pause remains effective.
- Carry 8.81 asynchronous File Info admission, queued early Start/Later and joined Cancel. Build five products; pass 2,794 backend assertions and 302 actual UI assertions. Correct BOM-aware dependency scanning and retain initial compiler/UI failures and ownership trace evidence.
- Remove 77 owned fixture roots, 127 temporary build files and five source copies; preserve installed files and personal catalogs. Candidate only; installed UDM remains 8.68. [Detailed evidence](docs/independent-finishing-8.82.md).

## 8.81 / extension 0.72.45 — superseded, unqualified development build

- Move File Info prefetch admission/capacity attempts off the window thread; retain early Start/Later intent and join admission before asynchronous Cancel. The corrected new C: prefetch suite passes 42 assertions.
- Broader qualification exposes an ordinary publication race; preserve its failed runs and controlled diagnosis instead of claiming release readiness. 8.82 contains the fix and complete requalification. [Investigation](docs/file-info-prefetch-8.81.md).

## 8.80 / extension 0.72.44

- Read repeated File Info display metadata on an owned joined worker; retain coherent size and filename controls while the catalog is busy. Coalesce requests and prevent completion-driven refresh loops. Keep explicit decisions authoritative and guard main shutdown while a File Info dialog owns reads.
- Build all five native products; pass 2,648 backend assertions, 183 main/File Info/network/login/progress UI assertions and 88 progress-menu assertions. Remove 54 fixture roots, 56 temporary test build files and the owned source copy. Candidate only; installed UDM remains 8.68. [Evidence and remaining work](docs/file-info-display-8.80.md).

Milestone headings use the reconciled two-digit sequence. Parenthetical recorded numbers identify the original build or changelog values. The [archived tag ledger](#archived-tag-version-ledger) covers tagged milestones that lack a paired heading.

## 8.79 / extension 0.72.43

- Run network polling, endpoint enumeration and monitor controls on one joined operation worker. Render immutable cached network views, coalesce busy polls, preserve mutation acknowledgement semantics and settle captured-response shutdown before Manager exit.
- Build five products and pass 2,578 backend assertions, 170 actual main/network/login/progress assertions and 88 progress-menu assertions. Retain the corrected monitor fixture's failure evidence; remove 53 fixture roots and 54 temporary test build files. Candidate only; installed UDM remains 8.68. [Evidence and remaining work](docs/network-session-8.79.md).

## 8.78 / extension 0.72.42

- Prepare main-list duplicate prompts on the Resume worker and run duplicate choice, replacement verification and existing-download retry on an owned joined decision. Validate displayed request identity, preserve failed-save recovery, and defer close/repeated submit until acknowledgement.
- Build five products and pass 2,376 backend assertions, 159 actual main/login/progress window assertions and 88 progress-menu assertions. Remove 48 fixture roots and 50 temporary test build files. Candidate only; installed UDM remains 8.68. [Evidence and remaining work](docs/duplicate-decision-8.78.md).

## 8.77 / extension 0.72.41

- Run main-list Resume classification, recovery, durable queueing and dispatch on a joined worker. Retain checked follow-up views and apply complete content decisions on a worker, with stale-request validation and saved-choice recovery.
- Build five products and pass 2,314 backend assertions, 139 actual main/login/progress window assertions and 88 progress-menu assertions. Remove 48 fixture roots and 48 temporary test build files. Candidate only; installed UDM remains 8.68. [Evidence and remaining work](docs/resume-selection-8.77.md).

## 8.76 / extension 0.72.40

- Prepare requested queue-retry login views on owned workers, including credential resolution and current challenge/policy/source checks. Present only a ready view, retain pending requests on Cancel, and protect login presentations from main-window shutdown.
- Build five products and pass 2,246 backend assertions, 125 actual main/login window assertions and 88 progress-menu assertions. Remove 44 fixture roots and 46 temporary test build files. Candidate only; installed UDM remains 8.68. [Evidence and remaining work](docs/requested-authentication-8.76.md).

## 8.75 / extension 0.72.39

- Run Sign in and retry persistence, guarded resume and immediate dispatch on an owned decision task. Keep login controls responsive with a busy state, defer close until command settlement, and preserve a retryable saved identity when credential saving succeeds but retry preparation fails.
- Build five products and pass 2,170 backend assertions plus 104 isolated window assertions, including the actual held-save dialog and complete authenticated retry. Remove 41 fixture roots and 42 temporary test build files. Candidate only; installed UDM remains 8.68. [Evidence and remaining work](docs/authentication-login-8.75.md).

## 8.74 / extension 0.72.38

- Prepare and durably claim automatic login prompts on an owned joined task, including credential resolution before UI construction. Revalidate the challenge before presenting/applying credentials; asynchronously rearm unpresented or failed dialogs and restore unpresented claims before shutdown.
- Build five products and pass 2,082 backend assertions plus 97 isolated window assertions, including real prepared login rendering under a held model and complete owned Basic/Digest transfers on both drives. Remove 37 fixture roots and 38 temporary test build files. Candidate only; installed UDM remains 8.68. [Evidence, ownership and remaining work](docs/authentication-task-8.74.md).

## 8.73 / extension 0.72.37

- Snapshot automatic confirmation, review and authentication discovery with a try-lock. Retain unpresented confirmation and browser-batch reservations across busy reads and presentation failures; roll back seen markers after failed dialog creation. Restore the authentication pending flag if its durable claim fails.
- Build five products and pass 1,996 backend assertions plus 79 isolated main-window assertions, with owned presenter callbacks for the new discovery cases. Remove 35 owned fixture roots and 36 temporary test build files. Candidate only; installed UDM remains 8.68. [Evidence, ownership and remaining work](docs/automatic-prompts-8.73.md).

## 8.72 / extension 0.72.36

- Run desktop model ticks and queue dispatch on one joined, coalescing worker. Wake the live window with an owned integer identity after work completes; stop and join maintenance before Manager shutdown, with restart after failed durable shutdown. Skip busy timer layout, quota, hangup and queue-completion discovery reads.
- Preserve the File Info decision acknowledgement guard and synchronous authoritative countdown checks. Build five products and pass 1,934 backend assertions plus 54 actual main-window assertions; remove 34 owned fixture roots and 34 temporary test build files. Candidate only; installed UDM remains 8.68. [Evidence, ownership and remaining work](docs/model-maintenance-8.72.md).

## 8.71 / extension 0.72.35

- Capture navigation, filtered history rows, settings, status observations and selection identities in one nonblocking model read, then apply controls after release. Keep the prior view intact while the model is busy and retry on a later timer tick. Read pending browser offers without waiting for a held catalog.
- Build five products and pass 1,780 backend assertions plus 37 actual main-window assertions on an isolated owned desktop. Remove 29 owned fixture roots and 28 temporary test build files. Candidate only; installed UDM remains 8.68. [Evidence and remaining work](docs/history-read-8.71.md).

## 8.70 / extension 0.72.34

- Build list tooltips from immutable row/navigation snapshots and skip busy-model toolbar/selection-file advisory refreshes. Preserve previous button states while busy; actual commands retain current-model validation. Normal full-history refresh and other synchronous owners remain separate work.
- Build five products and pass 1,624 backend assertions plus twenty actual MFC main-window assertions on a separate owned desktop. Remove 23 owned fixture roots and 24 temporary test build files. Candidate only; installed UDM remains 8.68. [Evidence and remaining work](docs/main-advisory-8.70.md).

## 8.69 / extension 0.72.33

- Keep modal-dialog size refresh from waiting on a busy catalog: retain existing cells for that tick, read only the visible range when the model becomes available, and update controls after releasing the snapshot lock. This targets one remaining nested main-window timer stall; broader synchronous UI work remains separate.
- Build all five products and pass 1,606 backend assertions plus ten actual MFC main-window assertions on a separate owned desktop. Remove owned fixtures and temporary test build files. Tested development candidate; installed UDM remains 8.68. [Evidence and remaining UI work](docs/modal-size-refresh-8.69.md).

## 8.68 / extension 0.72.32

- Hash bounded independent copies of successful cached write prefixes during reception, then reuse the joined partial hash at completion. Detect cache rewrites/resets, preserve gap and ownership checks, and retain conservative full-hash fallback and mandatory durability/publication barriers. Build, installation and actual complete-time comparisons are recorded separately.
- Build and install five native products and both manifests; pass 1,580 assertions and actual both-drive pause/reload/resume controls. The matched split comparison improves median finishing by 56.9 ms and total elapsed by 109.6 ms against 8.67; post-flush hash wait falls from 44.58 to 0.042 ms. Preserve all individual losses, including one development tail 3.35 ms behind IDM.
- The installed executable wins all nine paired IDM cases at exit, durable and full-elapsed endpoints. Remove generated payloads, fixture trees, twenty temporary test build artifacts and the private comparator copy. [Detailed evidence and scope](docs/reception-hash-8.68.md).

## 8.67 / extension 0.72.31

- Remove redundant progress-side writeback locking and notifications below the next dirty-byte trigger and while IO is active. Preserve the 8 MiB trigger, credited-byte accounting, joined shutdown and final durable publication barrier. Verify concurrent completion/progress handoff before performance and installation.
- Build and install all five native products with both manifest updates. Pass 1,342 assertions and real pause/reload/resume controls on both drives. The installed app wins all nine paired IDM cases at exit, durable and full-elapsed endpoints; split medians are 348 ms finishing and 2.597 s total versus IDM 397 ms / 3.604 s.
- Retain all four-way development observations: the original 155 ms regression does not reproduce, and 8.67's finishing median remains 17.4 ms longer than 8.66 there. Exact no-loss causality is unproven. Reject the queued-writer probe, remove generated payloads/fixtures/test binaries, and preserve rollback/evidence. [Details](docs/data-writeback-8.67.md).

## 8.66 / extension 0.72.30

- Drain split-download cached data on a single joined background worker using data-only writeback, triggered by newly credited written bytes. Avoid repeated full metadata/storage-cache synchronization during reception; retain the mandatory final durable flush and all publication/recovery ordering. A failed optional request disables further drainage and never supplies disk/resume credit.
- Build five matched native products and pass 1,336 assertions plus actual pause/reload/resume checks on both drives. Three D: 64 MiB split trials beat IDM at process-exit, post-exit-flush and full-elapsed endpoints. Retain the roughly 155 ms median full-elapsed regression versus 8.65; no universal speed-parity claim. Remove generated payloads, fixtures and temporary test binaries. [Results and limitations](docs/data-writeback-8.66.md). No installer or deployment.

## 8.65 / extension 0.72.29

- Bind the accepted serialization to its durable snapshot/sequence and compare actual pinned catalog text during completion, shutdown and destruction. Remove repeated disk-proof JSON parsing and the remaining destructor pathname reopen. Preserve explicit saves, pending-operation guards, real reads and conservative fallback.
- Rebuild all 24 C++ core units and matched products. Pass 1,992 selected assertions, 120 exact disk proofs, forty actual Manager stop measurements and two normal App integrity/exit/post-exit-flush samples. Warm 7,500-record Manager stop improves from 123.0 to 42.8 ms on C: and 119.6 to 44.6 ms on D:; no overall download/IDM parity claim.
- Remove 794 fixture files, both sample payloads and seventeen obsolete 8.63 build files while preserving current and 8.64 baseline dependencies. No installer, deployment, administrator prompt, push or release. [Evidence and memory/provenance scope](docs/accepted-catalog-text-8.65.md).

## 8.64 / extension 0.72.28

- Read completion/shutdown proof catalogs through their existing pinned handles, removing redundant pathname size/open operations while preserving full content comparison, exclusion, size limits and fallback durability. Add a registered pinned-reader fixture.
- Build ten products and five fixtures; pass 502 selected assertions, 120 exact cached-catalog proofs and two real App integrity/exit/post-exit-flush samples. All six proof medians improve, with every slower pair retained. This is not a whole-app speed or IDM parity claim.
- Remove 178 generated files and both App payloads. Installed app stays unchanged; no installer, deployment, administrator prompt, push or release. Ordered writes stay private. [Evidence](docs/pinned-catalog-read-8.64.md).

## 8.63 / extension 0.72.27

- Separate native HTTP receiver/ordered/prefinish/hasher waits and record longest optional flush, in-flight-at-finish and discarded pending work. Preserve original teardown measurements across repeated finish. Clear generation-specific timings before reuse.
- Retain existing IO scheduling, mandatory final flush and publication ordering; this adds attribution rather than claiming a latency fix. Keep ordered writes private after the prior no-regression result.
- Build ten products and five fixtures with all resources reporting 8.63. Pass 696 selected assertions in ten C:/D: runs and four actual 64 MiB App integrity/exit/post-exit-flush samples. Remove 176 generated files and all sample payloads. Installed app stays unchanged; no installer, deployment, push or release. [Evidence](docs/join-waits-8.63.md).

## 8.62 / extension 0.72.26

- Integrate a gated native ordered-body HTTP candidate using the existing one-buffer budget, accepted-byte retry validation, asynchronous storage-failure notification, written-credit checkpoints and joined same-allocation hash handoff. Normal builds remain synchronous pending promotion gates.
- Preserve accepted queued data on pause, separate already-accounted reception from legacy progress accounting, remove unused ordered-worker file opens and clear stale per-generation usage diagnostics.
- Pass 1,780 selected checks in twenty final-binary-matched runs. Build five private qualification and five normal products. Verify a real 64 MiB ordered App transfer, an 8 MiB normal App transfer, and actual 16 MiB pause/reload/resume plus 22 dropped-connection recovery with independent bytes/hashes, normal exit and post-exit flush.
- Remove 953 generated files and all smoke payloads; retain matched inputs/products and sanitized evidence, including the rejected no-tick driver. Installed app stays 8.32; no installer, deployment, push or release. No balanced speed or IDM parity claim. [Details](docs/ordered-body-8.62.md).

- Further test-only qualification: 126 assertions in fourteen actual Manager storage-fault cases on C:/D:, with corrupt failed-byte exclusion, held-IO pause/rollback and full recovery hashes. Production source/objects and versions stay unchanged; remove all owned fixture data. [Evidence](docs/ordered-storage-fault-8.62.md).

- Further test-only HTTP qualification: 24 actual Manager cases / 126 assertions on C:/D:, covering original-body retention, one-use URLs, helper denial/malformed identity, truncated response recovery and replacement blocked by a held old-generation save. Independent final bytes/hashes pass; 128 generated files / 219,930,803 logical bytes removed. No production version or default activation change. [Evidence](docs/ordered-responses-8.62.md).

- Further test-only shutdown qualification: 20 assertions in four actual Manager processes on C:/D:. A held old save blocks stop; joined shutdown preserves independently verified partial credit, and fresh-process resumes complete exact files. Remove 21 generated files / 34,841,252 logical bytes. No production or version change. [Evidence](docs/ordered-shutdown-8.62.md).

- Further test-only lifecycle qualification: 62 assertions in ten actual Manager processes on C:/D:. Active/retiring folder, refresh-address and removal mutations are refused; failed operation saves roll back, and inactive edits retain verified resume credit. Fresh-process folder/refresh downloads complete exact hashes. Remove 40 generated files / 60,226,000 logical bytes. No production or version change. [Evidence](docs/ordered-lifecycle-8.62.md).

- Further test-only performance qualification: 36 matched normal/private App transfers across C:/D:, including process exit and separate output flush, pass exact correctness checks. Split wall medians improve, but C: finishing median regresses and one split pair is slower overall; keep ordered writes private. Retain every outlier and remove all payloads/owned metadata. Versions unchanged. [Results and decision](docs/ordered-comparison-8.62.md).

## 8.61 / extension 0.72.25

- Integrate generation-bound disk-credit checkpoint leases into the existing synchronous cache-eligible fresh root HTTP path. Persist acknowledged segment prefixes while preserving live UI and network/quota counters.
- Keep detached save leases through catalog publication and result handling; reconcile and retire outside pending saves before reset/publication. Pin the actual expected parts file with a real read handle. Retain the rejected attribute-only pin attempt as evidence.
- Pass 1,644 selected checks across eighteen final-binary-matched runs, including held successful/failed actual Manager saves, pending browser-admission overlays, native checkpoint concurrency and physical pin/reload checks. Build five products and twelve fixtures from 522 inputs.
- Verify a real 64 MiB split App download uses the binding, reconciles every byte and passes exact length/hash/catalog, normal exit and separate output flush checks. Remove generated fixtures and payloads. Ordered network writes remain inactive; no speed parity claim. Installed app stays 8.32; no installer, deployment, push or release. [Details](docs/native-disk-credit-8.61.md).

## 8.60 / extension 0.72.24

- Share one 64 MiB storage owner and atomic budget between the production body cache and standalone ordered writer. Transfer the same allocation through cache invalidation, uniquely owned writer admission and joined successful completion, retaining the budget until the last reader releases it.
- Refuse completed-body export after pause, abort, failure or unsettled work. Ordered network writes remain inactive pending disk-credit checkpoint and cancellation integration; final flush and publication policy remain unchanged.
- Pass 1,303 native checks plus 94 standalone checks. Build five products and eight fixtures from 518 inputs; verify a real 64 MiB split App download with write-buffer hashing, exact length/hash/catalog, normal exit and a separate output flush.
- Remove generated fixtures and payloads while retaining qualified products/core objects. Installed desktop stays 8.32; no installer, deployment, push or release. No finishing-speed or IDM parity claim. [Details](docs/shared-body-storage-8.60.md).

## 8.59 / extension 0.72.23

- Fix the full-body cache's process-budget lease: a download worker could release a mutex acquired by the parent thread when optional caching was disabled. Use a thread-independent atomic owner token, release storage before handing off the budget, and explicitly prevent ownership copies/moves.
- Keep the one-buffer 64 MiB cap, serialized cache-data access, exact byte coverage and disk fallback. No flush, allocation or HTTP request policy change.
- Pass 1,263 selected checks in twelve final-binary-matched runs, including 79 cache checks on each drive with cross-thread invalidation/destruction and 24 eight-contender rounds. Build five native products and eight fixtures from 516 inputs; recompile Transfer and Bridge with forty qualified core objects retained.
- Verify a real 64 MiB split App download uses the write-buffer cache, matches its hash/catalog, exits normally and passes a separate output flush. Remove 407 generated files / 126,169,357 logical bytes plus its payload. Installed desktop stays 8.32; no installer, deployment, push or release. No finishing-speed or IDM parity claim. [Details](docs/cache-lease-8.59.md).

## 8.58 / extension 0.72.22

- Fix valid Unicode filenames rejected when stem truncation left half a surrogate pair before the extension. Clip the name prefix and extension at complete UTF-16 boundaries while preserving existing budgets, ASCII output, device-name rules and regex caching.
- Reproduce the actual admission failure against qualified 8.57. Pass 2,286 selected checks across thirteen final-binary-matched runs, including 290 filename and 84 Unicode workflow checks on each drive.
- Cover manual/native browser-message admission, reported server names, Properties, exact Windows targets, refused-save rollback, nested audio/video catalog loading, stable reload and original backup preservation.
- Build five products and nine fixtures from 516 inputs; rebuild Core and Bridge, retaining forty qualified core objects. Complete a real one-request 4 MiB download with the original split-emoji CLI name, exact normalized target, hash/catalog/normal-exit/post-exit-flush checks.
- Remove 768 generated files / 133,914,094 logical bytes plus its payload. Installed desktop stays 8.32; no installer, deployment, push or release. No finishing-speed or installed/browser parity claim. [Details](docs/unicode-filename-8.58.md).

## 8.57 / extension 0.72.21

- Reuse the unchanged reserved-filename pattern across calls instead of reconstructing its regex for each loaded/admitted name.
- Pass 1,926 selected checks across eleven final-binary-matched runs, including 194 filename rules per drive and twelve simultaneous first-use workers. The qualified 8.56 engine also passes the new filename fixture.
- Build five products and eight fixtures from 515 inputs; rebuild Core and Bridge, retaining forty qualified core objects.
- Compare eighteen production-core controller processes. Filename-batch thread cycles fall roughly 70%; constructor wall times vary widely, with slower candidate runs retained. No consistent full-startup, GUI, installed or IDM parity claim.
- Record a separate valid-Unicode truncation failure in 8.56 for the next correctness increment. Pass a real one-request 4 MiB lifecycle/hash smoke; remove 670 generated files / 267,417,915 logical bytes plus its payload. Installed desktop remains 8.32; no installer, deployment, push or release. [Details](docs/filename-startup-8.57.md).

## 8.56 / extension 0.72.20

- Transfer parsed catalog records into Jobs, removing the redundant whole-record deep copy while retaining existing validation, normalization, backup and recovery behavior.
- Add 112 load/failure-preservation checks on each drive: nested audio/video, long and unknown metadata, interrupted states, malformed late/nested records and exact original/backup preservation.
- Pass 1,863 selected checks in ten final-binary-matched runs. Build five products and seven fixtures from 513 inputs; rebuild Core and Bridge and retain forty qualified core references.
- Measure eighteen hidden startup launches. With 7,500 records, median manager-ready falls 447.2012 → 406.3427 ms and initial-refresh-returned 651.7352 → 609.4440 ms. Empty-history medians are slightly slower; no universal, visible-paint, installed or IDM parity claim.
- Pass a real one-request 4 MiB lifecycle/hash/normal-exit/post-exit-flush smoke. Remove 793 generated files / 333,714,036 logical bytes plus its payload. Installed desktop stays 8.32; no installer, deployment, push or release. [Details](docs/catalog-load-8.56.md).

## 8.55 / extension 0.72.19

- Remove unnecessary whole-history startup snapshots and restrict Start/Stop rollback copies to potentially modified queue rows, including same-queue completed nonmembers and Start's review-field exceptions.
- Restore transient schedule pause hints on failed startup persistence. Preserve wake timers, retry, synchronization and persist-before-cancel ordering.
- Pass 593 selected checks across thirteen final-binary-matched runs; build five native products and ten fixtures from 512 inputs with Queue and Bridge rebuilt.
- Measure hidden startup with 7,500 records: median queue phase 46.3258 → 0.0468 ms; post-pipe-construction marker 693.7648 → 627.8318 ms. These are three samples per engine, without visible-paint, installed or IDM parity claims.
- Pass a real one-request 4 MiB integrity/lifecycle smoke. Remove 677 generated files / 352,091,647 logical bytes plus its payload. Installed desktop remains 8.32; no installer, deployment, push or release. [Details](docs/queue-startup-8.55.md).

## 8.54 / extension 0.72.18

- Move receipt-only recovery journal checks and catalog persistence outside recovery's own model lease. Retain immutable descriptors, scope revalidation, shutdown enrollment and exclusion of affected destinations and journal names.
- Make ordinary/replacement publishers and journal recovery honor journal-name reservations. Refuse a conflicting reused identifier before moving bytes, while allowing unrelated real publication during a held inventory check.
- Bind retirement to actual serialized receipt absence. Accept a later containing snapshot after an earlier save fails; reject noncontaining, deferred and unknown outcomes. Roll back only missing exact keys, preserving concurrent receipts and current metadata.
- Pass 22,769 selected checks in 21 final-binary-matched runs, including 267 pruning checks on each drive, all sixteen selected backend suites and retained publication/recovery fault coverage. Preserve the four-case 8.53 blocked-model control and the corrected fixture assertion separately from final qualification.
- Build five products and sixteen fixtures from 510 inputs using five rebuilt native core units and nineteen audited retained native units. Verify 8.54 on all 21 outputs and 0.72.18 in both source extension manifests. Pass an actual one-request 4 MiB app lifecycle/hash/post-exit-flush smoke.
- Remove 14,422 owned generated files / 243,500,410 logical bytes, its separate 4 MiB payload and 15,147 empty test directories. Keep a compact cleanup receipt and compressed private inventory. No installation, installer creation, push or release; installed desktop stays 8.32. Finishing-speed parity and remaining caller/namespace gates stay open. [Details](docs/recovery-pruning-8.54.md).

## 8.53 / extension 0.72.17

- Dispatch completed and unfinished Properties saves to an owned worker. Keep the actual message pump active while real catalog IO is held; close only after acknowledgement, or restore the controls/draft and report the error for correction/retry.
- Guard the full preparation/acknowledgement lifecycle against reentrant Close and duplicate OK, including control-freezing notifications. Worker actions capture owned edits/header deltas and manager/job references, without control or dialog captures.
- Recheck completed-job membership, closing, batches and current session under the saving lease. Preserve atomic logical header merging, guarded unfinished recovery, fast input validation and completed-file relocation. Remember a successful relocation before retrying a failed metadata save.
- Pass 2,896 selected checks across 17 runs matched to final binary hashes, including 55 new backend checks per drive, four actual-window probes (62 checks) and the existing 522-check Properties suite. Keep the qualified paused 8.52 blocked-UI control separate from invalid completed-fixture attempts and repaired warning-dialog driving.
- Build five products, nine backend fixtures and two UI fixtures from 509 source inputs with all 24 native core units rebuilt. Pass a real 4 MiB lifecycle/hash/normal-exit/post-exit-flush smoke; remove 1,071 owned files / 321,629,212 logical bytes plus the payload. Installed desktop stays 8.32; backend/pruning locks, zone metadata and original IDM finishing-speed gaps remain open. [Details](docs/properties-save-8.53.md).

## 8.52 / extension 0.72.16

- Release recovery's own model lease during accepted verified-handle closure, zone/retained-output metadata work and exact-handle journal retirement. Close still-owned verified handles outside that lease during exceptional unwind too.
- Reserve the separate immutable retained source alongside publication destinations. Retain full operation ownership until cleanup ends; validate accepted rows and the exact private receipt around each cleanup boundary.
- Preserve durable completion when cleanup fails. Read-only journals retain their exact intent for explicit retry; failures after retirement preserve accepted history and do not erase a successor receipt.
- Pass 25,218 selected checks in 28 runs, including 4,471 cleanup checks and 306 real workflow cases per drive. Cover independent saves, mutated scope/graph/membership, batches, shutdown, private receipts, read-only retirement, refused retained deletion and hard-link cleanup.
- Build five products and twenty-one fixtures from 507 frozen inputs, two rebuilt core units and twenty-two retained native units. Pass a one-request 4 MiB lifecycle/hash/normal-exit/post-exit-flush smoke; remove 13,971 owned files / 293,728,138 logical bytes plus its payload. Installed desktop remains 8.32, without installer/deployment/push/release. Receipt-only pruning and original IDM finishing-speed losses remain unresolved. [Details](docs/recovery-cleanup-8.52.md).

## 8.51 / extension 0.72.15

- Release recovery's own model lease during journal enumeration, bounded reading/parsing and exclusive same-object claim. Retain shutdown enrollment, scoped input revalidation and full operation reservations before reconciliation.
- Reuse exact held content without a second read. Refuse changed file identity, identical-content clones, missing scoped intents, changed graphs/destinations and shutdown while preserving bytes/history.
- Preserve active synchronization replacement workers during predecessor discovery. Retain the real retry regression that exposed the initial guard error and the corrected fixture failures.
- Pass 16,276 selected checks across 26 runs, including 2,795 discovery checks and 238 workflow cases on each drive. Build five products and twenty fixtures from 506 frozen inputs, five rebuilt core units and nineteen retained native units.
- Pass a complete one-request 4 MiB lifecycle/hash/normal-exit/post-exit-flush smoke. Remove 9,495 owned files / 273,029,223 logical bytes plus the payload. Installed desktop stays 8.32; no installer, deployment, push or release. Original IDM finishing-speed losses remain unresolved. [Details and qualification](docs/recovery-discovery-8.51.md).

## 8.50 / extension 0.72.14

- Fix four reproduced false-completion cases in replacement recovery: completed new/old metadata could record Size 100 for actual 14/9-byte files, and rollback could resurrect stale original accounting while consuming the journal.
- Require completed-pair counts, digest and destination to match each verified held file. Reconcile retained/restored originals from measured size, received bytes, digest and target; preserve descriptions, finish history and unpublished replacement bytes.
- Apply one bounded integer completed-counter schema to normal and recovery proof preparation. Preserve zero-byte files, exact restoration length, private receipts, failure rollback and explicit retry.
- All 10,686 checks pass across 24 runs, including 2,490 metadata checks on each drive. The separate five-case 8.49 probe and its exact source remain. Five products and nineteen fixtures built from three rebuilt core units and twenty-one retained native units.
- Complete a one-request 4 MiB lifecycle/hash/normal-exit/post-exit-flush smoke. Remove 4,478 owned files / 241,807,467 logical bytes plus the payload. Installation stays deferred at 8.32; no installer, deployment, push or release. Discovery/cleanup work and IDM finishing-speed parity remain unresolved. [Details and qualification](docs/replacement-metadata-8.50.md).

## 8.49 / extension 0.72.13

- Run ordinary publication, paired replacement and legacy-completion byte IO outside recovery's model lease using immutable inputs and held ownership. Revalidate current identity, membership, graph, destination, verification fields and shutdown before applying history.
- Classify the replacement target from one held digest instead of reading/hashing the old original twice. Reuse the legacy repair's measured handle/length, preserving independent current metadata for both current and legacy journal application.
- All 5,706 production-code checks pass across 22 runs, including 1,166 byte-IO checks on each of C: and D:. The separate instrumented 8.48 control passes 90 checks and verifies blocked-model behavior and the redundant old-target hash pass.
- Build five products and eighteen fixtures using two rebuilt core units and twenty-two retained native units with checked source closure. Complete a one-request 4 MiB app lifecycle/hash/normal-exit/post-exit-flush smoke. No IDM timing or universal speedup claim.
- Remove 2,377 owned files / 240,384,792 logical bytes plus the smoke payload, retaining source/control reconstruction and evidence. Installed app remains 8.32, with installation deferred. Journal discovery/read, post-acceptance cleanup/pruning, further caller audit and original finishing-delay parity remain unresolved. [Details and qualification](docs/recovery-byte-io-8.49.md).

## 8.48 / extension 0.72.12

- Release the five identified UI recovery scopes through guarded backend operations. Recheck progress actions after recovery; preserve current authentication/session headers in Properties and roll back failed edits to the refreshed current row.
- Keep cancellation eligibility atomic through recovery completion and removal, including Awaiting offers recovered to Paused. Preserve completed and newly accepted downloads; recheck browser receipt identity and reject overlapping cancellation of a provisional recovery state.
- All 3,374 focused checks pass across 20 runs, including the new 206-check actual command-adapter matrix on C: and D:, existing cancellation and Properties request-draft regressions. Five products and seventeen fixtures built from a rebuilt native core.
- Complete and hash-check a one-request 4 MiB app download through normal exit and a separate output flush; retain initial fixture failures and exact reconstructible source deltas. Retire owned temporary payloads and redundant targets.
- Installed app remains 8.32, with installation deferred. Verification/discovery IO, other mutation saves, installed visual/browser checks and original finishing-delay parity remain unresolved. [Details and evidence](docs/recovery-ui-commands-8.48.md).

## 8.47 / extension 0.72.11

- Reserve recovery job trees and destinations before verification; release recovery's own model lease during catalog serialization, durable save and acknowledgement.
- Bind acceptance to exact captured recovery changes, removed fields, row presence/absence and private completion receipts. Support actual later containing writers; preserve journals and independent edits on failed/unknown outcomes.
- Move backend recovery preparation outside command mutation leases and revalidate current inputs. Preserve recovered File Info warnings, project batch ordering and active internal-child Resume behavior.
- All 2,829 focused checks pass across 16 runs, including 558 new recovery checks on each of C: and D:. Build five products and fourteen fixtures from the rebuilt native core; hash-check a real 4 MiB app download through normal exit and post-exit flush.
- Remove 1,787 owned files / 187,721,662 logical bytes plus the smoke payload, retaining initial failures and reconstructible source deltas. Installed desktop remains 8.32, with installation deferred. Verification IO, four outer UI scopes and original finishing-delay parity remain open. [Design and qualification](docs/recovery-catalog-checkpoint-8.47.md).

## 8.46 / extension 0.72.10

- Bind ordinary and replacement recovery to an immutable opened journal and retire that exact file by handle. Prevent concurrent pathname replacement and byte writes through hard links from consuming a newer journal.
- Reject refused journal ownership before changing the selected row; preserve journals on failure and support explicit retry. Keep unrelated locked scoped journals untouched.
- Retire the 8.45 metadata experiment after its performance gate failed; restore the qualified ordinary transfer path and preserve the source delta and all measured losses.
- All 1,661 focused checks pass, including journal cases on C: and D:. Five production programs and eleven fixtures built; a real 4 MiB app lifecycle/hash smoke passed. Generated payloads and redundant/retired artifacts removed.
- Installed desktop remains 8.32; research-first installation deferral, recovery mutex migration and finishing-delay parity remain open. [Implementation and evidence](docs/recovery-journal-ownership-8.46.md).

## 8.45 / extension 0.72.9 — experimental candidate — retired by 8.46

- Prepare Internet-zone metadata before fresh bounded local HTTP body transfers; final verification preserves the exact correct marker and repairs a changed or missing one. Other paths retain their existing metadata call.
- Build five native products and five fixtures; all 489 focused checks and both 4 MiB early-marker/tamper downloads pass against the corrected link. Source extension versions advance; browser JavaScript is unchanged.
- Full-lifetime performance gate **not accepted**: alternating D: 64 MiB trials have essentially unchanged exit medians and a worse candidate post-exit durability median. Retain all results; no claim of being faster than 8.44 or IDM.
- Remove generated payloads, fixtures and redundant target objects. Installed desktop remains 8.32; no installation, installer, release or push. [Details and evidence](docs/early-zone-metadata-8.45.md).

## 8.44 / extension 0.72.8

- Retire the 8.43 body/intent overlap after its timing gate failed to demonstrate a speedup. Restore ordinary serial write-only body writeback, with no extra per-file flush task or newly generated early intents.
- Retain verified recovery support for saved early intents and final-name protection through normal catalog acceptance.
- Built five native products and ten fixtures; all 1,491 focused checks passed. One actual 64 MiB lifecycle/hash smoke completed and its payload was removed; concurrent REA analysis excludes a controlled timing claim.
- Preserve the exact experiment source delta and measured losses; retire obsolete prototype binaries/core objects. Installation remains deferred at 8.32. Fresh actual IDM comparison and finishing parity remain unverified.

## 8.43 / extension 0.72.7 — experimental candidate

- Overlap eligible ordinary-transfer body writeback with durable publication-intent preparation. Both barriers precede rename and completed-catalog acceptance; recovery explicitly verifies and flushes early-intent bytes before accepting them.
- Preserve early intents after denied write access, changed/truncated bytes or unrecognized staging paths. Keep body and destination-name pins through catalog acceptance, plus automatic checkpoint coalescing until publication resolves.
- Built five native products and ten fixtures; all 1,473 focused checks passed, including owned process interruption before the body-flush syscall.
- Withhold installation: two alternating D: 64 MiB trials did not demonstrate a speedup (median application-exit tail 266.595 ms for 8.42 versus 269.816 ms for 8.43). Both wins and losses remain in the evidence. Installed desktop remains 8.32.

## 8.42 / extension 0.72.6

- Retain both verified replacement versions, or the restored original, until recovery catalog persistence succeeds. Recheck the measured identity after restoring the original name; reject reparse-point files during verification.
- Reproduce a concurrent-writer failure against 8.41 and add paired/restore recovery ownership checks. All 1,249 focused checks passed; five native products and seven fixtures are built. Installation remains deferred.

## 8.41 / extension 0.72.5

- Retain verified publication-recovery file ownership through catalog acceptance, rejecting concurrent writes and renames. Recheck the measured file identity after staging rename.
- Add real interrupted-publication tests for staged and already-renamed recovery. The concurrent-writer regression failed against 8.40; all 817 focused checks now pass. Five native products and five fixtures are built; installation remains deferred.

## 8.40 / extension 0.72.4

- Move pinned publication-directory checks outside the shared model mutex while retaining destination revalidation and durable publication barriers.
- Cover model responsiveness and folder changes at the final directory-validation boundary.
- Built five native products and five focused fixtures; all 803 publication/recovery checks passed. Installation remains deferred.

## Unreleased

- Developer diagnostics: the isolated HLS fixture can capture a private NetLog and export sanitized owned-loopback parser errors, response framing and body-byte counts. Added privacy checks and a guarded intentionally malformed preview to verify actual error capture without admitting a download. Product versions/behavior remain unchanged; the intermittent natural preview failure remains unresolved.

- Transfer finish after review: the advisory SHA-256 computed after publication now stops with the download (Stop or exit no longer waits for a full-file hash) and, for a file copied to another volume, reads the published copy after the parts are cleared; a copy onto a network or removable destination is flushed before completion is committed, and a fixed-disk copy whose writeback cannot be confirmed keeps its parts and records a warning; the stale keep-alive retry honours Retries=0; speed limits are clamped the same way everywhere.
## 8.39 / browser 0.72.3

- Retain failed pending cancellations in a bounded recovery queue. A separate cancellation-error dialog explains the failure and offers explicit background Retry without showing the main list. Generation-bound retry/dismiss actions exclude stale decisions and duplicate retries; repeated failure remains visible rather than reporting success. Main-window notification has a timer fallback.
- Failed/active cancellation offers retain their presentation claim until recovery is handled, preventing refresh from reopening File Info prematurely. The recovery status says “could not be confirmed” to account for partly persisted cleanup; Close acknowledges an unresolved notice without asserting success.
- All 1,849 focused checks pass, including 91 cancellation/controller checks and 247 real File Info checks per theme. Twenty binaries qualify from 497 frozen inputs and 42 core objects; Bridge rebuilt and forty-one objects retained. All sixteen focused runs were repeated after final claim integration.
- Removed 1,276 generated files and fifteen obsolete 8.38 fixture executables (213,380,863 logical bytes total), preserving results, rollback products, pinned references, tools and personal data. Installation stays deferred at 8.32. Production main-frame/browser/shutdown acceptance and finishing-delay parity remain open. [Implementation and evidence](docs/file-info-cancellation-recovery-8.39.md)

## 8.38 / browser 0.72.2

- Retain a pending File Info session's claim through its cancellation callback. Late matching producers inherit Cancel without posting another notification; terminal cleanup retires all queued keys and successor claims before reporting completion. Stale cancellation of user-confirmed work clears its obsolete presentation receipt without deleting the download.
- All 1,764 focused checks pass, including 72 cancellation/ownership checks and 214 real File Info checks in each theme. Twenty binaries qualify from 496 frozen inputs and 42 core objects; Bridge/CaptureAdmission rebuilt and forty objects retained.
- Removed 593 generated files and fifteen obsolete 8.37 fixture executables (209,141,548 logical bytes total), preserving results, rollback products, pinned references and personal data. Installation remains deferred at 8.32. Visible persistence-failure recovery, installed-browser/session-end acceptance and finishing-delay parity remain open. [Implementation and evidence](docs/file-info-cancellation-8.38.md)

## 8.37 / browser 0.72.1

- Retain a private request fingerprint in pending File Info sessions. Cancel follows exactly matching in-flight ownership outside the Manager lock, safely removes only an unconfirmed accepted job, or durably releases a missing receipt before the session retires. Coalescing rejects a changed request fingerprint. Preserve accepted history, user-confirmed work and existing receipt compatibility.
- All 1,706 focused checks pass, including 46 cancellation/restart/failure checks, 142 race/ownership checks and 198 real File Info checks per theme. Twenty binaries qualify from 496 frozen inputs and 42 core objects. All 24 native core units rebuilt; eighteen unchanged compression objects were retained. The first private rebuild lacked the canonical Brotli include path; its failure is preserved and the complete core rebuilt after correction.
- Removed 585 generated files (163,257,917 logical bytes) and 28 obsolete 8.35/8.36 fixture executables (86,178,816 logical bytes), retaining historical results, product rollback binaries, pinned references, canonical tools and personal catalogs. Installation remains deferred at 8.32. Cancellation callback presentation ownership and user-visible failure recovery remain open, alongside installed-browser acceptance and finishing-delay parity. [Implementation and evidence](docs/file-info-cancellation-8.37.md)

## 8.36 / browser 0.72.0

- Coalesce a validated same-receipt browser File Info successor into its retained presentation, carrying the draft and cancellation without posting a second window notification. Retry activity belongs to the session; stale generations cannot submit another Retry. A joined worker reuses a matching producer-owned successor without taking over that producer's completion lifetime. Preserve an undispatched original notification by retargeting its inbox entry.
- All 1,660 focused checks pass, including 142 race/ownership checks and 198 real File Info checks in each light/dark mode. Nineteen native binaries qualify from 495 frozen inputs and 42 core objects; Bridge rebuilt and 41 objects retained. The initial race-fixture wait assertion failure is preserved; only that fixture was rebuilt after correcting its session-completion condition and extending boundary checks.
- Removed 578 generated files (156,586,062 logical bytes), preserving current/prior binaries, canonical tools/core objects and personal catalogs. Installation remains deferred; installed desktop is 8.32. Browser version carries after component 99 under the two-digit policy. Installed-browser, full session-end and post-retirement cancellation gates remain open; finishing-delay parity is unresolved. [Implementation, evidence and limits](docs/file-info-coalescing-8.36.md)

## 8.35 / browser 0.71.99

- File Info's explicit admission Retry now follows an exactly matching in-flight browser receipt outside the Manager lock. It rechecks durable acceptance and the live download before binding the same presentation; a definitely failed competing admission may yield to the already requested Retry. Ordinary browser and legacy callers retain their non-waiting busy behavior. Preserve the session's draft and cancellation through the joined attempt; no generic uncertain outcome authorizes another submission.
- All 1,486 checks pass across thirteen focused backend fixtures and light/dark real File Info runs. All nineteen native binaries (five production, fourteen fixtures) qualify from 495 frozen inputs and 42 core objects. Preserved the first UI fixture failure and fixed its post-modal weak-reference assertion; product/core bytes were unchanged by that fixture correction.
- Removed 567 generated fixture/compiler files (155,983,501 logical bytes), preserving current/prior binaries, canonical core/media tools and personal catalogs. Installation remains deferred; installed desktop is 8.32. Two independently notified browser producers, installed-browser acceptance and finishing-delay parity remain open. [Implementation, evidence and limits](docs/retry-join-8.35.md)

## 8.34 / browser 0.71.98

- Hash fresh, known-length non-range HTTP responses from their successful write buffers, including large sequential downloads. This removes the trailing prefix-reader thread on that proven single-writer path. Preserve pending-confirmation, resumed and range-probe exclusions, required-digest checks and fallback behavior. Keep existing full-body retention limits; flush pacing and publication barriers are unchanged.
- Add a live loopback regression for large sequential bodies, correct and incorrect required digests, and multiworker range exclusion. All 1,930 checks in 21 selected suites passed; 26 binaries built and verified from 494 frozen inputs and 42 core objects. Preserved the initial missing-helper fixture failure and corrected it using a shared-tools junction.
- Portable comparison verified all eighteen files and confirmed inline hashing in all three large sequential runs, but the finishing target remains unmet: exit tails won 4/9 pairs and full elapsed runs 8/9; the sequential median tail was 568.490 versus IDM's 341.011 ms. Retain every loss and the separate post-exit flush endpoints. Removed 253 generated files (1,070,640,722 logical bytes), preserving personal data, current/prior binaries, core objects and canonical tools. Installation remains deferred; installed desktop stays 8.32. [Results and limits](docs/finishing-performance-8.34.md)

- Follow-up controlled D: comparison verified eighteen files; exit tails won 5/9 and full elapsed runs 9/9, while all split tails lost. Approved tracing stopped/exported safely but failed owned-process coverage, so kernel attribution remains unverified. Corrected recorder deadlines, added private D: workload placement and prepared a larger ring with trace-only UDM-last ordering; 21 tooling checks pass. Total cleanup now covers 282 files / 2,563,368,802 logical bytes. No additional app behavior, installation or version change is claimed for these research/tooling updates.

- Add a standalone read-only ETL consumer to avoid large XML exports and reject incomplete target lifetimes/flush pairs. Twenty normalized fault checks, five CLI checks and a real five-event process-local ETL smoke match Windows reference counts; kernel mapping remains unverified. Removed seven fixture/compiler files (4,414,461 logical bytes); total 8.34 cleanup is 289 files / 2,567,783,263 logical bytes. This is investigation tooling, with no further app/version/install change or new UAC request.

## 8.33 / browser 0.71.97

- Add owned diagnostic-only finish timestamps separating file/journal/catalog IO, acknowledgement, cleanup and teardown. A loaded 1.201-second split sample spent 447 ms in final file flushing, 140 ms in intent publication and 300 ms in catalog publication (256 ms in FlushFileBuffers); acknowledgement/Manager stop were fast. Diagnostic load is explicit, and these results do not replace shipping qualification.
- Coalesce redundant automatic checkpoint IO only while every active owner is finalizing eligible full-body ordinary downloads, with no pending admission/publication capture or storage error. Preserve explicit saves, retries, other active work, scoped marker release and all mandatory durability barriers. File-flush pacing is unchanged.
- All 1,913 maintained checks passed and all 25 binaries qualified after a fixture precondition correction. Windows canceled elevation before any installed file changed; 8.32 remains installed and the 8.33 comparison is pending. Deleted 287 generated files (1,980,442,579 logical bytes), preserving ready binaries/core, personal data and evidence. [Detailed findings and status](docs/finishing-performance-8.33.md)
- The exact qualified executable subsequently passed all eighteen portable download size/hash checks from its build folder. Original and separate output-flush tails each won 8/9 pairs; full elapsed runs won 9/9. All sequential/split tail pairs won, but one 4 MiB control pair lost, so the all-pairs target remains false and installed-path performance is pending. Added and parser-validated a small Windows IO profile with 16 MiB configured system buffers; no live system recording started.
- Added normal-token trace controller and separate named administrator recorder with deadline/owner-loss cleanup, failed-export/unknown-state handling and bounded recovery logs. Nineteen supplemental lifecycle tests, a read-only plan and a two-transfer case-filter smoke passed; no live privileged collection occurred. The optional benchmark filter is a documented measurement-input amendment, with original source hash reconstructed and full default protocol preserved. Native app binaries remain unchanged.

## 8.32 / browser 0.71.96

- Admit one optional early flush after one-eighth credited progress for fixed-volume 32–64 MiB split transfers with two through eight workers. Later progress cannot request another; other split sizes and higher concurrency retain previous eligibility.
- Reduce the three-operation sequential schedule's minimum final runway from eight MiB to four MiB while retaining measured slow-IO advancement, joined teardown and the mandatory final publication flush. Installed and hash-verified after 1,901 maintained checks and all 25 native binaries built. All eighteen final sizes/hashes and saved digests matched, settings restored and personal data/tools preserved. The experiment does not establish a speed gain: original tails won 4/9, full elapsed times 8/9, and separate output-flush tails 9/9 but full durable times 8/9. Sequential/split median original tails still lost and one split full download lost. The target remains unmet. Removed 266 generated files (1,096,280,687 logical bytes). [Design and results](docs/finishing-performance-8.32.md)

## 8.31 / browser 0.71.95

- Bound sequential local-file early writeback for known 32–64 MiB bodies to quarter-body, half-body and eight-MiB-before-EOF milestones. Keep one joined worker, coalesced requests, adaptive runway and the mandatory final publication flush. Other transfer sizes and multiple connections retain their prior schedule.
- Fourteen added scheduling/teardown checks cover both boundary sizes, held IO, operation limits, nonblocking progress and advisory errors. Installed and hash-verified after all 1,887 maintained checks passed and all 25 binaries built. Sequential 64 MiB median exit tails were 246.421 versus IDM 451.068 ms; median tails through symmetric output flush were 261.464 versus 636.667 ms. Original paired tails won 7/9 and full elapsed runs 9/9; the stricter original target remains unmet. Separate output-durability tails/full runs won 9/9 each. Every final size/hash and saved digest matched; settings/catalogs/tools preserved. Removed 266 generated files (1,096,001,246 logical bytes). [Design and results](docs/finishing-performance-8.31.md)

## 8.30 / browser 0.71.94

- Prepare final transfer metadata before the owning worker's existing journaled publication, avoiding a duplicate acknowledged complete-catalog write for bounded ordinary HTTP downloads. Keep downloaded parts until acknowledgement and require matching durable/model proof before skipping the worker's redundant save.
- Add real-worker cancellation, retained-parts, first-checkpoint failure/recovery and one-completion-write checks. All 1,873 maintained checks passed and all 25 binaries built. Installed and hash-verified on the third attempt with unchanged personal catalogs/tools. All eighteen comparison files/hashes matched and IDM settings were restored. The no-slower target remains unmet: original finishing tails won 6/9, separate output-durability tails 7/9, and full elapsed runs 9/9 at both endpoints. Sequential 64 MiB median exit tail was 419.426 versus 370.395 ms; observed output and recovery-intent flush waits remain. Removed 246 generated fixture/compiler/comparison files (1,032,827,287 logical bytes), preserving qualified binaries/core and personal data. [Design and validation](docs/finishing-performance-8.30.md)

## 8.29 / browser 0.71.93

- Final published-file writeback requests write access only and measures open, sparse normalization, FlushFileBuffers and close separately, with individual Windows errors. Failed confirmation retains parts; the sparse branch no longer ignores flush failures. Sixteen added checks cover real bytes, operation attribution/order/access, failure handling and actual HTTP-worker integration.
- The comparison runner optionally adds a symmetric explicit completed-output flush after process exit, preserving original exit metrics and separately recording durability endpoints before hashing. [Design and validation](docs/finishing-performance-8.29.md)
- Installed after 1,848 maintained checks passed; a supplemental four-check actual NTFS sparse normalization probe also preserved the independently verified bytes. All eighteen comparison outputs and final UDM hashes matched. Nine measured opens were 0.138–0.263 ms; the slower split flush was 266.728 ms, identifying current flush waiting rather than opening. The old 2.7-second combined sample cannot be separated retroactively.
- The no-slower target remains unmet: original exit timing won five paired tails and all nine full elapsed runs; the separately retained symmetric output-durability endpoint won eight tails and all nine full elapsed runs. Its remaining sequential 64 MiB loss was 720.012 versus 671.327 ms. New post-exit flushes change cache conditions, so no code-only speedup is claimed. All failures remain. Corrected stale launcher labels in 8.26–8.29 after executable-hash/resource verification, with raw records and timing fingerprints preserved. Individually removed 270 generated files / 1,099,818,205 logical bytes, retaining current artifacts and personal catalogs. [Complete evidence](docs/finishing-performance-8.29.md)

## 8.28 / browser 0.71.92

- Prepare publication-journal storage on the worker before the network body, outside the model lock. Create the actual durable intent only after verification, preserving journal ordering and final writeback. Five real paused-origin checks verify early directory preparation without a premature intent.
- Split atomic intent observation into directory readiness, temporary-file opening, bytes written, flush/close and final publication while preserving aggregate/full-exit timing. [Design and validation](docs/finishing-performance-8.28.md)
- Installed after all 1,832 checks passed; 25 binaries, 490 inputs and 42 core objects qualified, with unchanged personal catalogs. Every comparison output/final-record hash matched. The original comparison still fails the target: seven of nine paired finishing tails and eight full elapsed times favored UDM. A 2.715-second split tail was traced to 1.831 seconds in published-file opening/flushing, 295 ms intent flush/close and 129 ms catalog writing; settlement was fast. The historical 16.7-second event did not recur and its exact cause remains unknown.
- A separate symmetric post-exit flush diagnostic retained original metrics and found another 526 ms flushing IDM's output versus 0.436 ms for UDM in one pair. It does not replace the failed primary result or prove IDM's internal API choices. Individually removed 789 generated files, 3,311,914,806 logical bytes, preserving current binaries/core, source, catalogs, shared tools, rollback/evidence and required historical fixtures. [All outcomes and limitations](docs/finishing-performance-8.28.md)

## 8.27 / browser 0.71.91

- Reuse the prepared, writer-excluding source handle for unknown-digest publication identity and fallback SHA-256 instead of opening the staging pathname again. Named-source and destination revalidation, required checksums and journal/catalog durability remain. Five source-pin tests cover identity/size, writer denial, fallback digest and rename safety.
- Export per-operation publication timings from actual Manager-owned workers before their final checkpoint; standalone publication keeps its existing durable proof. The comparison metric and thresholds are unchanged. [Design and validation](docs/finishing-performance-8.27.md)
- Installed after all 1,827 checks passed. All eighteen output/final-record hashes matched. UDM won nine full elapsed comparisons and eight finishing tails; sequential trial 2 still lost at 522.237 versus 376.288 ms, including 115 ms intent writing and 65 ms catalog writing. Source identity and rename were negligible. The no-slower target remains unmet; all split tails were faster and the historical 16.7-second stall did not recur. All eighteen benchmark payloads were rechecked and removed afterward. [Complete results](docs/validation/finishing-8.27/comparison.json)

## 8.26 / browser 0.71.90

- Move preliminary finish-path destination filesystem validation outside the shared model lock, using an immutable owned job snapshot. Publication-time source/destination pins and current-state revalidation remain. A slow inspection no longer monopolizes the model lock.
- Split settlement attribution into lock acquisition, snapshotting, folder inspection, destination-directory work, recovery-journal checks and actual-length query/correction. Preserve the aggregate total and all background work in timing comparisons. Five held-validator checks cover lock access, snapshot ownership, state transition and actual delay attribution. [Design and validation](docs/finishing-performance-8.26.md)
- Installed after all 1,822 checks passed, with 25 verified binaries and unchanged personal catalogs. All eighteen output/final-record hashes matched. UDM won nine paired full elapsed times and eight finishing tails; sequential trial 2 remained slower at 470.056 versus 340.412 ms, including 236 ms publication work. All three split tails were faster and the old 16.7-second stall did not recur, without proving its historic cause. The target remains unmet. Removed all eighteen verified benchmark payloads afterward. [Complete results](docs/validation/finishing-8.26/comparison.json)

## 8.25 / browser 0.71.89

- After workers, periodic checkpoints, relocation and publication ownership settle, reuse the exact acknowledged final checkpoint only when there is no pending admission/publication outcome and the pinned actual catalog matches the current snapshot. Dirty, missing, malformed, externally changed, unreadable or failed state still triggers a real shutdown save. Explicit saves remain forced. This removes the redundant post-completion catalog flush without moving background work past process exit.
- Added fifteen shutdown fallback/persistence/repeated-stop checks and updated lifecycle write counts. The publication failure/retry fixture now includes a genuinely unsaved setting before locking the catalog, so it continues to test a required failed save. All 1,817 checks passed; 25 binaries, 489 inputs and 42 core objects qualified. Installed seven hash-verified files and updated version registration, preserving personal catalogs and shared tools. Initial fixture/environment failures remain separately recorded.
- All eighteen full-process outputs and final UDM hashes matched, but the performance target remains unmet: six of nine paired finishing tails and six full elapsed times favored UDM. Median tails UDM/IDM: 110.888/343.908 ms (4 MiB), 109.311/372.474 ms (sequential 64 MiB), 307.246/461.824 ms (split 64 MiB). A 16.748-second split outlier includes 14.345 seconds in Settle; split full elapsed median is slower. Preserve all outliers; further tracing must identify the individual blocking call. Individually removed 1,032,470,602 logical bytes of benchmark/fixture payloads and unique compiler outputs. [Design, complete results and remaining work](docs/finishing-performance-8.25.md)

## 8.24 / browser 0.71.88

- Retain eligible credited parallel HTTP body bytes in one optional 64 MiB slot. Track actual interval coverage, bound fragmentation and fall back to file hashing on invalid input, contention or allocation failure. Advisory hashing can overlap publication without rereading the file; required checksum and final durability gates remain.
- All 1,802 native checks passed; 25 binaries, 489 frozen inputs and 42 core objects verified. Installed seven changed files with matching hashes and unchanged personal catalogs. The original comparison still failed the no-slower target; its results remain separate from the single comparison after storage cleanup. Removed 1,294 owned generated/test files, recovering approximately 2.75 GB observed free space. Shared media-helper hard links, current core, rollback records and source were preserved. [Implementation and measurements](docs/finishing-performance-8.24.md) · [Qualification](docs/validation/finishing-8.24.json)

## 8.23 / browser 0.71.87

- Use measured first-flush duration and credited body throughput to advance the second opportunistic writeback when a fixed final-2-MiB point would leave too little runway. The request remains bounded to half-body and two total operations; required final flush is unchanged. Added held-worker regression checks for adaptation, bounds and no extra late operation.
- Overlap optional same-file advisory hashing with publication and final writeback. All body/trailing-hash writers are already joined; the hash task exclusively owns the hasher, supports the hard-link rename and writeback share modes, and uses a child cancellation token. A scope guard stops it on unwind and the future joins before staging cleanup or hasher destruction. Required server/user/replacement checksums remain before publication. Copies and resource exhaustion retain serial fallback. New fields distinguish full advisory time and the remaining foreground wait.
- All 1,778 native checks passed; 24 binaries, 487 frozen inputs and 42 core objects verified. Installed and independently checked all eighteen comparison outputs and final recorded hashes. Median UDM/IDM completion tails were 189.162/401.268 ms (4 MiB), 169.373/357.317 ms (64 MiB sequential), and 431.701/413.071 ms (64 MiB split). Split tail and individual outliers still failed the no-slower target, despite faster median full elapsed times in all three cases. Initial candidate fixture results are retained separately; the delivered ownership-guard build relinked and reran all targets. [Results](docs/validation/finishing-8.23/comparison.json). No setup package, release or media-helper copies.

## 8.22 / browser 0.71.86

- Preserve the first forced shutdown save and explicit saves. Avoid a duplicate destructor save only after successful shutdown, with no worker/active ownership, batch, storage error or stale checkpoint sequence. Snapshot the current state, overlay admission reservations and capture publication outcomes; any pending capture prevents the shortcut. Pin and re-read the real non-reparse catalog and require an exact match with the acknowledged checkpoint. Changed live state, missing/malformed/replaced/unreadable catalogs and failed writes retain the forced-save path. No synthetic acknowledgement is used.
- Added eleven lifecycle checks, bringing the catalog suite to 25 checks. They count actual durable-write boundaries and inspect restored catalogs/receipts, including an equal-content pending admission that must still issue a fresh write. All 1,774 focused native checks pass, including 274 completion checks through observed exit 0. Twenty-four binaries have verified 8.22 resources/hashes, 487 frozen inputs and 42 core objects; Core/CaptureAdmission/FileWorkflows/Bridge rebuilt, 38 hashes retained. A new fixture accessor type error was corrected before delivery; final Core is rebuilt and all final targets link against its final hash. [Qualification](docs/validation/finishing-8.22.json) · [Architecture](docs/finishing-performance-8.22.md)
- Installed all seven changed payloads after Windows approval, with every installed hash verified and both personal catalogs unchanged. All eighteen full-process comparison outputs and final UDM SHA-256 values match. Median UDM/IDM tails: 118.564/442.065 ms (4 MiB), 560.663/325.819 ms (64 MiB sequential), 536.626/374.405 ms (64 MiB split). The full-background target remains unmet; all outliers remain, including a 1,968.313 ms UDM 4 MiB trial. Sequential stalls include 469–828 ms waiting for an in-flight prefinish flush; split flush/hash work remains serialized. Removed eighteen verified comparison payloads (830,472,192 logical bytes). No setup package, release, push or media-helper copies. [Installation](docs/validation/finishing-8.22/installed.json) · [Results](docs/validation/finishing-8.22/comparison.json) · [Cleanup](docs/validation/finishing-8.22/comparison-cleanup.json)

## 8.21 / browser 0.71.85

- CLI automatic Exit now checks the final transfer wake-up instead of waiting exclusively for the 500 ms main timer. It retains actual inactive ownership, scanner acceptance, exact saved-catalog, modal-state and successful-message-post gates. The timer remains a fallback. Eight writeback checks establish distinct half-body/near-end operations for 4 MiB and 64 MiB sequential responses; 4 MiB now uses 3.5 MiB for its near-end request. Required final flush remains.
- Added thirteen completion-fixture checks with the main timer disabled. Publication, an early Finished event, a pending scan, dirty final catalog and disabled owner cannot bypass completion readiness; a durable idle completion posts one Exit, with no duplicate on repeated events. Transfer phases now record final writeback/hash/parts cleanup separately from earlier publication. All 1,763 focused checks pass. Twenty-four binaries have verified resources/hashes, 487 frozen inputs (including the tracked comparison runner) and 42 core objects; Transfer/Bridge rebuild and 40 hashes remain unchanged. [Qualification](docs/validation/finishing-8.21.json)
- Added a tracked full-lifetime comparator using actual Windows process-exit timestamps. It waits for observed current-session app processes, validates output hashes and final persisted UDM SHA-256 after exit, restores IDM connection values/types, retains outliers and never kills processes or deletes outputs. This includes automatic quit scheduling and shutdown, not exact internal completion or GUI dialog visibility. [Protocol](docs/finishing-performance-8.21.md)
- Installed all seven changed native/manifest payloads, hash-verified with both personal catalogs unchanged and rollback copies retained. The first elevation ended before the helper ran; a built-in Windows PowerShell retry worked, initially rejected reopened Chrome before staging, then succeeded after authorized session closure and process-exit settling. No setup package or shared-media-tool copies. [Installation](docs/validation/finishing-8.21/installed.json)
- All 18 installed comparison outputs and final UDM SHA-256 values match. Full-exit median tails UDM/IDM: 155.691/485.981 ms (4 MiB), 656.544/402.749 ms (64 MiB sequential), 484.377/445.522 ms (64 MiB split). The full-background target remains unmet. The 8.20 full-exit baseline is preserved separately; it must not be confused with earlier first-file-availability scores. [Comparison](docs/validation/finishing-8.21/comparison.json) · [Baseline](docs/validation/finishing-8.21/baseline-8.20.json)
- Removed eighteen baseline and eighteen updated comparison payloads, 830,472,192 logical bytes in each batch, after exact-root, no-reparse, size and SHA-256 checks. Reports, catalogs/journals, rollback copies, sources/helpers and current builds remain. Desktop control still fails to initialize; no fresh live IDM dialog timing claim. [Baseline cleanup](docs/validation/finishing-8.21/baseline-cleanup.json) · [Comparison cleanup](docs/validation/finishing-8.21/comparison-cleanup.json)

## 8.20 / browser 0.71.84

- Bounded known-size parallel HTTP layouts up to 64 MiB on fixed volumes reserve regular files during preparation. Fresh outputs avoid post-EOF sparse conversion; existing sparse files and larger/unknown/network layouts retain sparse-aware resume. Saved segment Done continues to bound credited data. Final writeback remains.
- Finishing queries actual length before requesting a writer; a correctly sized file avoids that reopen, with PublicationPreparation still checking/pinning source identity and length. Opportunistic prefinish writeback is limited to sequential transfers after the 8.19 parallel experiment regressed.
- All 1,742 focused native checks pass, including five new actual completed-worker assertions; 24 binaries have verified 8.20 resources/hashes, 486 frozen sources and 42 core objects. Transfer/Bridge rebuilt; 40 unchanged core hashes retained. An initial admission suite stopped at 14 checks because the fresh fixture directory lacked its media-tools junction. The original result is retained privately; one junction to existing installed tools resolved the environment and the suite passed all 73 checks. No product changes or helper copies for that retry. [Qualification](docs/validation/finishing-8.20.json) · [Environment retry](docs/validation/finishing-8.20/environment-retry.json)
- Installed five native payloads and both manifests, with all seven installed hashes verified and both known personal catalogs unchanged. Rollback copies retained. No setup package, release or media-helper copies. Browser runtime JavaScript is unchanged; active browser version and live GUI behavior are not freshly verified. [Installation](docs/validation/finishing-8.20/installed.json)
- All 18 installed-app comparison outputs match independent SHA-256. Three-trial median first-exclusive-availability tails UDM/IDM: 41.948/24.515 ms (4 MiB), 39.362/19.041 ms (64 MiB sequential), 51.836/98.307 ms (64 MiB split). Sequential gaps remain 17.433 and 20.321 ms. The split observation can precede later flush/hash work and does not establish full finishing parity. All outliers remain, including split trial 2 at 3,340.040 ms UDM / 187.270 ms IDM. IDM connection settings restored. The requested no-slower target remains unmet. [Comparison](docs/validation/finishing-8.20/comparison.json) · [Investigation and next architecture](docs/finishing-performance-8.20.md)
- Removed 18 verified comparison outputs (830,472,192 logical bytes), plus 677 individually verified generated objects, superseded fixtures and owned test payloads (699,373,854 logical bytes). Current binaries, 42 reusable core objects, shared tools, historical fixtures, catalogs/journals, results, sources/helpers and installation rollback copies remain. Physical allocation reclaimed is not measured. [Comparison cleanup](docs/validation/finishing-8.20/comparison-cleanup.json) · [Artifact cleanup](docs/validation/finishing-8.20/artifacts-cleanup.json)

## 8.19 / browser 0.71.83

- Added bounded opportunistic data-file writeback during transfer, outside the progress/model mutex, at half-body and a connection-aware near-end milestone. In-flight flushing joins and its writer handle closes before publication preparation. Final file flushes, required content/checksum gates and journal/catalog durability remain. Post-publication metadata now has its own timing phase.
- All 1,737 focused native checks pass, including 11 new writeback lifetime, nonblocking progress, coalescing, error and real-flush checks. Twenty-four binaries have verified resources/hashes, 486 frozen inputs and 42 core objects; Transfer/Bridge rebuild, 40 unchanged core hashes retained. Installed five native payloads and both manifests with rollback copies and unchanged personal catalogs. No media helpers copied or setup package created. [Qualification](docs/validation/finishing-8.19.json) · [Installation](docs/validation/finishing-8.19/installed.json)
- The three-trial paired loopback comparison still fails the finishing target in all three cases: median UDM/IDM tail 65.969/21.913 ms (4 MiB), 97.507/30.213 ms (64 MiB sequential), 710.981/139.676 ms (64 MiB split). All 18 file hashes match. IDM connection settings restored; test payloads removed (830,472,192 logical bytes). The first exclusive file-open observation is not a GUI completion measurement. [Results and limits](docs/validation/finishing-8.19/comparison.json) · [Cleanup](docs/validation/finishing-8.19/comparison-cleanup.json)

## 8.18 / browser 0.71.82

- Fresh sequential HTTP files grow with credited writes as regular files. Sparse-aware resume and parallel layouts remain. Normal outputs skip unnecessary sparse normalization while retaining final writeback. Content classification uses a bounded, coverage-checked cache of actual credited write bytes when available, falling back to disk for missing/resumed/decoded prefixes. Background prefix hashing wakes on progress rather than relying only on a polling interval.
- All 1,726 focused native checks pass, including 13 bounded prefix-cache checks and 15 additional real completed-worker checks. Twenty-three binaries built and installed payloads verified. Required verification, final flush, recovery and security metadata remain. [Qualification](docs/validation/finishing-8.18.json)
- Three paired trials per case fail all finishing medians despite correct file hashes: UDM/IDM 79.479/20.956 ms (4 MiB), 232.791/28.532 ms (64 MiB sequential), 359.581/116.088 ms (64 MiB split). Content sniffing is reduced to sub-millisecond work in the measured runs; flush/commit/normalization spikes remain. A separate owned-file zone-tag probe does not support tagging as the primary spike source, so security-zone behavior was retained. All 18 comparison payloads removed (830,472,192 logical bytes). [Comparison](docs/validation/finishing-8.18/comparison.json) · [Probe](docs/validation/finishing-8.18/zone-probe.json) · [Cleanup](docs/validation/finishing-8.18/comparison-cleanup.json)

## 8.17 / browser 0.71.81

- Before a new paired publication acquires ownership or moves any file, it rejects its own unresolved intent, honors cancellation and reconciles the selected original publication tree. Recovery covers both roles of an earlier paired operation. Complete status and the selected destination are revalidated afterward. Locked, malformed or changed predecessor evidence stops the next operation with its new staging intact. Failed prior catalog saves can be recovered before the original complete-state check.
- The new checks reproduce the defect against the qualified 8.16 core at chain-ordinary after 282 checks. The corrected build passes 395 replacement checks, including 99 new chained-journal checks, and all 1,698 focused native checks. Twenty-two binaries have verified 8.17 resources/hashes, 482 frozen inputs and 42 core objects; Duplicates/Bridge rebuild and 40 core hashes remain unchanged. Eight isolated browser regression suites pass. [Evidence](docs/validation/chained-recovery-8.17.json)
- Installed 8.17 and both browser packages 0.71.81 using 17 individually verified payload replacements (19,204,878 bytes), with rollback copies and both known catalogs preserved. Shared FFmpeg helpers were not copied. No setup package or release was created. Active browser reload/capture and GUI behavior remain unverified. [Installation](docs/validation/installed-8.17.json)
- Completed eight installed-app comparison transfers with matching hashes. UDM total times were 0.996/2.488/8.764/1.954 seconds against IDM 2.506/3.146/9.321/3.229 seconds for 4 MiB, 16 MiB, 64 MiB sequential and 64 MiB split cases. One trial each; private empty UDM catalog versus existing IDM profile, unchanged IDM connection settings. UDM finishing tail remained longer in three of four cases. No GUI timing claim. [Comparison](docs/validation/idm-comparison-8.17.json)
- Removed 286 generated fixtures/objects/payloads (257,553,421 logical bytes), plus eight completed comparison files (310,378,496 logical bytes). Current binaries, reusable core, historical fixtures, catalogs/journals and rollback copies remain.
- Predecessor recovery still uses model-locked filesystem/hash/save work. Fresh GUI dialog/focus comparisons are blocked by desktop-control initialization failure. The CLI comparison is one cold-launch trial per case, with an initially empty private UDM catalog and the existing IDM profile; it does not prove general speed or exact IDM parity. Next: isolate the observed post-transfer finishing cost, detach recovery with explicit ownership/outcomes, and restore live desktop/browser comparisons.

- Removed 90 obsolete November 11 compiler objects/debug symbols from four explicitly checked D:/udm-ab benchmark build folders (1,231,831,158 logical bytes), retaining sources, executables and logs. Observed D: free space rose from 4,194,304 to 1,244,135,424 bytes during cleanup; this is an observation, not isolated physical-allocation accounting. [Evidence](docs/validation/old-d-benchmark-artifacts-8.17.json)

## 8.16 / browser 0.71.80

- Paired replacements now write their intent, perform ReplaceFile, serialize/write the final catalog and clean up the journal outside the publisher model acquisition. Runtime ownership protects both records and the previous-version path. One identity and one containing-snapshot proof bind both completed rows, including an exact private receipt when the new output is internal and unregistered. Current metadata is merged at final apply; failed or superseded original writes may succeed only through an actual containing pair snapshot. Unacknowledged outcomes conditionally roll back matching new-row completion contributions while keeping original history bound to its actual previous-version file. Both physical versions and the journal remain; no potentially acknowledged swap is reversed. New-format recovery validates correlation, destinations and hashes, finds dependent journals through either record, and preserves later queue/scanner metadata on each exact row while merging interrupted outcomes into current records.
- All 1,599 focused checks pass, including 296 new paired replacement, 347 ordinary/child publication, 186 ownership, 46 catalog coordinator/proof and 261 completion checks through observed exit 0, plus preparation/admission/HTTP/recovery/queue/gzip suites. Four owned subprocess crashes exercise intent-written, file-moved, catalog-written and journal-removed boundaries. Twenty-two binaries have verified 8.16 resources/hashes, 482 inputs and 42 core objects; Core/Duplicates/Bridge rebuild while 39 unchanged core hashes remain. Both manifests are 0.71.80; browser runtime code is unchanged. [Evidence](docs/validation/replacement-io-8.16.json)
- Removed 562 individually verified generated files (498,700,261 logical bytes), preserving current executables, reusable core, historical fixtures, sources/helpers, catalogs/journals and shared media tools. [Cleanup](docs/validation/replacement-io-8.16-artifacts.json)
- Initial protected-directory handle checks and recovery filesystem/hash/save work remain model-locked. Outer recursive-lock callers, private-pair process crashes, broad lifecycle/media/live/prefetch/volume qualification and external namespace races remain open. No fresh installed-browser/IDM timing, zero-delay or full parity claim. No installer, release or installation; installed desktop remains 7.11.

## 8.15 / browser 0.71.79 — unqualified intermediate

- All 1,551 focused checks passed and 22 binaries built. Review then found that original-row rollback could point live history at the new file before recovery. No installer or installation. Corrected in 8.16 with retained physical relocation, original-scoped recovery and explicit pre-recovery path checks. [Record](docs/validation/replacement-io-8.15-intermediate.json)

## 8.14 / browser 0.71.78

- Ordinary and attached-child publication now releases the model mutex during final catalog serialization/write. Snapshot-bound acknowledgements require the exact completed row, not merely its ID or a newer sequence. A newer containing write can prove completion when the original write fails or is superseded; otherwise matching owned contributions roll back and the recovery journal remains. Standalone internal publications use exact private receipt metadata without inventing visible downloads. Journal unlink retires those receipts, and restart reconciles pending receipts or prunes orphan metadata while preserving file bytes. The 8.13 row-only candidate failed the existing standalone ownership case and is recorded as unqualified; 8.14 restores that contract.
- All 1,296 focused native checks pass: 347 publication, 39 catalog coordinator/proof, 186 ownership and 261 completion checks, plus the maintained preparation/admission/HTTP/recovery/queue/gzip suites. Four owned subprocess crashes exercise intent-written, file-moved, catalog-written and journal-removed boundaries. Twenty-one binaries have verified 8.14 resources/hashes, 480 inputs and 42 core objects; Core/Duplicates/Bridge rebuild and 39 unchanged core hashes are retained. Both browser manifests are 0.71.78; browser runtime code is unchanged. [Evidence](docs/validation/publication-catalog-8.14.json)
- Removed 246 individually verified generated files (477,349,006 logical bytes), retaining current binaries, historical fixtures, reusable core, sources/helpers, shared media tools and all catalogs/journals. [Cleanup](docs/validation/publication-catalog-8.14-artifacts.json)
- Paired replacement and recovery filesystem I/O still retain their existing locks. Outer recursive-lock callers, broad media/live/prefetch/cancellation/power/volume matrices, external namespace races and fresh installed-browser/IDM timing remain open. No zero-delay or complete parity claim. No installer or installation; installed desktop remains 7.11.

## 8.13 / browser 0.71.77 — unqualified intermediate

- Added exact containing-snapshot completion acknowledgement and conditional rollback, but failed the existing standalone internal publication contract. All 21 targets built; 304 publication checks passed; ownership failed after 167 checks in move-first. No installer or installation. Corrected in 8.14 with explicit private receipts. [Failure record](docs/validation/publication-catalog-8.13-intermediate.json)

## 8.12 / browser 0.71.76

- Release the model mutex during ordinary/child intent serialization and durable write, final source/project checks, write-through rename and journal cleanup. Keep runtime job/tree/path ownership and commit-phase guards; final completion/catalog save and paired replacement/recovery I/O retain their existing locks.
- Correlate new ordinary journals and completed rows. Merge completion-owned fields into fresh current metadata, preserve accepted later synchronization/scanner choices during verified recovery, and reject malformed/newer identities. Preserve unrelated staging at an acknowledged target. Legacy journals remain compatible.
- Pass 193 new commit checks with actual detached intent/move/cleanup gates, real denied final-save/unlink failures, physical-move acknowledgement and source-name faults, ordinary/child/opaque/decoded output, independent metadata, later queue choices and reopening. Final focused qualification passes 1,116 native checks, including 193 new commit, 186 ownership and 78 detached admission checks and 261 completion checks with observed exit 0. Twenty-one binaries have verified 8.12 resources/hashes, 479 inputs and 42 core objects; Duplicates/Bridge are rebuilt and 40 core hashes are retained. Shared media tools have zero new copies. [Evidence](docs/validation/publication-io-8.12.json)
- Detached final catalog outcomes, paired replacement identity/rollback, broad lifecycle/namespace/crash matrices and actual IDM timing remain work. No installer or installation; installed desktop remains 7.11.

- Remove 135 individually verified generated files (235,812,007 logical bytes), retaining current builds, historical fixtures, core cache and shared tools. [Cleanup](docs/validation/publication-io-8.12-artifacts.json)

## 8.11 / browser 0.71.75

- Share Windows ordinal logical path keys/equality/order and component ancestry across catalog naming, previous-version reservations, Move/Rename, case-only recovery, retained-output protection, publication and browser reservations. Preserve original stored names and URL semantics.
- Pass 186 ownership checks, including absent paused Unicode names, thirteen numbered aliases, Cyrillic pairs, physical case-only leaf spelling and byte preservation, catalog exact/prefix move conflicts, before/after journal recovery with independent metadata retained, and reopening. Final focused qualification passes 923 native checks, including 186 ownership and 78 detached admission checks and 261 completion checks with observed exit 0. Twenty binaries have verified 8.11 resources/hashes, 477 inputs and 42 core objects; Core/Duplicates/CaptureAdmission/FileWorkflows/Bridge are rebuilt and 37 core hashes are retained. Shared media tools have zero new copies. [Evidence](docs/validation/unicode-paths-8.11.json)
- Remove fifty obsolete large compiled-object names (2,346,725,906 logical bytes across 21 file objects). Shared hardlinks mean logical bytes are not recovered physical allocation. Observed C: free space rose from 224,632,832 to 982,814,720 bytes; retained 8.10 builds/core/source/tools and historical fixtures were hash-verified before source changes. Shared FFmpeg links/junction targets are preserved. [Cleanup](docs/validation/large-object-cleanup-8.10.json)
- Durable detached publication, namespace aliases/case-sensitive directories, broader timing/media/crash qualification and full IDM parity remain open. No installer or installation; installed desktop remains 7.11.

- Remove 116 individually verified generated files (230,420,156 logical bytes), retaining current builds, historical fixtures, core cache and shared tools. [Cleanup](docs/validation/unicode-paths-8.11-artifacts.json)

## 8.10 / browser 0.71.74

- Integrate Move/Rename and file-move recovery with publication and pending browser reservations. Protect exact destinations and required ancestor directories; preserve independent moves. Reject late publishers against an enrolled move.
- Use Windows ordinal case folding for active-move paths, fixing UTF-8 byte-lowercase mismatches such as Ä/ä. Scoped conflicting recovery rejects; unscoped recovery leaves journals/model/files untouched. All 168 ownership checks pass. Final focused qualification passes 905 native checks, including 168 ownership and 78 detached admission checks and 261 completion checks with observed exit 0. Twenty binaries have verified 8.10 resources/hashes, 476 inputs and 42 core objects; Core/Duplicates/CaptureAdmission/FileWorkflows/Bridge are rebuilt and 37 core hashes are retained. Shared media tools have zero new copies. [Evidence](docs/validation/publication-move-8.10.json)
- Remove twelve obsolete archived objects/debug symbols and non-core cached target objects (407,260,281 logical bytes), preserving current binaries, 42 core objects, historical fixture executables and sources/helpers. [Cleanup](docs/validation/development-object-cleanup-8.8.json)
- Full durable publication protocol, remaining mutation ingress and fresh IDM timing remain open. No installer or installation; installed desktop remains 7.11.

- Remove 113 individually verified generated files (230,053,268 logical bytes), retaining current builds, historical fixtures, core cache and shared tools. [Cleanup](docs/validation/publication-move-8.10-artifacts.json)

## 8.9 / browser 0.71.73 — unqualified development candidate

- Added move/publication/browser ownership guards. Stopped the owned build before target binaries after reviewing the existing bytewise UTF-8 active-move comparison; Unicode case aliases were not reliably reserved. Corrected and qualified separately in 8.10. No 8.9 product or fixture binary is qualified. [Intermediate record](docs/validation/publication-move-8.9-intermediate.json)

## 8.8 / browser 0.71.72

- Drain standalone publication ownership during shutdown before the forced final save, outside the model lock. Notify waiting shutdown when the last token retires; keep waits scoped to their Manager. Reject new publishFile calls once shutdown closes admission.
- Pass 157 ownership checks, including successful/canceled ordinary, nested and replacement publication during stop, accessible model, late-byte preservation, independent Manager exit, reopened final outcomes and final-save failure/retry. Final focused qualification passes 888 native checks, including 157 ownership and 72 detached admission checks and 261 completion checks with observed exit 0. Twenty binaries have verified 8.8 resources/hashes, 476 inputs and 42 core objects; Core/Duplicates/CaptureAdmission/Bridge are rebuilt and 38 core hashes are retained. Shared media tools have zero new copies. [Evidence](docs/validation/publication-shutdown-8.8.json)
- Preserve normal transport cancellation and joining. This drains owned publication work; callers still retain and join their own external threads. Reentrant/concurrent stop, broader media/crash lifetime and durable unlocked publication remain work. No installer or installation; installed desktop remains 7.11.

- Remove 109 individually verified generated files (229,595,422 logical bytes), retaining current builds, historical fixtures, core cache and shared tools. [Cleanup](docs/validation/publication-shutdown-8.8-artifacts.json)

## 8.7 / browser 0.71.71

- Protect idle parents while nested tracks publish, and retain/exclude descendants when a parent owns publication. Parent active/resume/configure/pause/removal and unscoped recovery guards now observe the track tree. Preserve independent video/audio sibling publication and token-specific retirement.
- Pass 107 ownership checks, including a real held child publisher, exact parent/child preservation, nested/whole-parent conflicts, sibling release and commit-phase isolation, and cycle-safe traversal. Final focused qualification passes 838 native checks, including 107 ownership and 72 detached admission checks and 261 completion checks with observed exit 0. Twenty binaries have verified 8.7 resources/hashes, 476 inputs and 42 core objects; Core/Duplicates/CaptureAdmission/Bridge are rebuilt and 38 core hashes are retained. Shared media tools have zero new copies. [Evidence](docs/validation/publication-family-8.7.json)
- The first compile caught a missing direct unordered_set include before any product or fixture was built; corrected before qualification. Keep standalone shutdown, raw graph edits, broader media lifetime/cancellation, remaining mutation and durable publication I/O work open. No installer or installation; installed desktop remains 7.11.

- Remove 99 individually verified generated files (229,080,804 logical bytes), retaining current builds, historical fixtures, core cache and shared tools. [Cleanup](docs/validation/publication-family-8.7-artifacts.json)

## 8.6 / browser 0.71.70

- Close the destination-ownership race between standalone publication and protocol-3 browser acceptance. Check pending acceptance before acquiring a publication bundle, and active publication before preparing a browser reservation, under the same model lock.
- Add both reservation-order cases to the detached admission fixture. Its 72 checks pass: blocked publishers preserve bytes/metadata, and later browser admission uses an independent filename/receipt while the earlier publisher completes separately. Final focused qualification passes 818 native checks, including 72 detached admission checks and 261 completion checks with observed exit 0. Twenty binaries have verified 8.6 resources/hashes, 476 inputs and 42 core objects; Core/Duplicates/CaptureAdmission/Bridge are rebuilt and 38 core hashes are retained. Shared media tools have zero new copies. [Evidence](docs/validation/publication-admission-8.6.json)
- Preserve existing journal/catalog protection and durable admission outcomes. Parent/session lifetime, standalone shutdown and irreversible publication lock/durability work remain. No installer or installation; installed desktop remains 7.11.

- Remove 99 individually verified generated files (228,613,381 logical bytes), retaining current builds, historical fixtures, core cache and shared tools. [Cleanup](docs/validation/publication-admission-8.6-artifacts.json)

## 8.5 / browser 0.71.69

- Reserve explicit publication jobs and destination paths atomically while preparation runs outside the model lock. Protect workerless and internal-child publication from normal conflicting actions; include replacement originals and previous paths in the same lease. Release only matching ownership tokens on every exit.
- Preserve preparation-phase transport cancellation, reject commit-phase Pause, include child reservations in direct naming, and defer matching unscoped recovery while scoped recovery rejects active publication. Keep existing journal/rename/catalog transaction protection.
- Pass 810 focused native checks, including 87 new ownership checks and 261 completion checks with observed process exit 0. Record the initial 30-second wrapper timeout and later results separately. Verify twenty 8.5 binaries, 476 inputs, 42 core objects and both 0.71.69 manifests. Reuse shared media tools with zero copies. [Evidence](docs/validation/publication-ownership-8.5.json)
- Parent/group lifetime, detached browser child admission, all mutation ingress, standalone shutdown and durable receipt/recovery work remain. This does not remove publication lock/flush delay or establish new IDM timing. No installer or installation; installed desktop remains 7.11.

- Remove 108 individually verified generated objects, superseded fixtures and owned payloads (225,570,146 logical bytes), preserving current builds, historical fixtures, core cache and shared tools. [Cleanup](docs/validation/publication-ownership-8.5-artifacts.json)

## 8.4 / browser 0.71.68

- Prepare publication staging and protected Grabber folders from owned Folder/FileName/ProjectDestinationRoot inputs outside the model mutex. Use an actual read-access source pin, measure its length and verify its named identity before commit; reject changed destination policy and source names before writing an intent. Reuse measured length for ordinary, empty, decoded and internal-track completion metadata. Legacy recovery keeps its previous size wrapper.
- Share directory writes so normal child-file publication works while validated directories remain protected against rename/deletion. Retain a fresh handle-attribute check under the commit lock for protected projects. Release the replacement source pin at the ReplaceFile boundary because Windows opens that file with write access. Missing/busy source or protected folders preserve staging and reject before intent.
- Final focused validation passes 723 native checks: 69 preparation checks across fifteen cases, 16 replacement recovery, the existing 595-check HTTP/catalog/queue/completion set, and 43 actual gzip transfer/confirmation/resume checks. The gzip runner verifies seven one-response reuse cases and strong-validator encoded-offset resume. Nineteen native binaries and both manifests are version/hash verified; shared media helpers are reused with zero copies.
- Keep journal/rename/replacement/catalog persistence and cleanup in the existing protected transaction. This is detached preparation, with a scoped project-type recheck still inside the model lock; it does not eliminate physical flush or publication-transaction latency. Stage delete sharing and external reparse/namespace races require a stronger ownership/recovery protocol. No installer or installation; installed desktop remains 7.11. Full IDM parity remains incomplete. [Evidence](docs/validation/publication-preparation-8.4.json)

- Remove 134 individually verified generated files (519,065,695 logical bytes), preserving current nineteen builds, latest historical fixtures, reusable core, shared tools and structured evidence. [Cleanup](docs/validation/publication-preparation-8.4-artifacts.json)

## 8.3 / browser 0.71.67 — development build, not released

- Correct metadata-only pin handles to actual read access. Source writer exclusion, stale/canceled input and directory-rename tests pass, but the project-folder case stops after 58 checks: write-restricted directory pins also prevent UDM from renaming its child file into that parent. Staging and intent remain; target is absent. Corrected and qualified in 8.4 with directory write sharing and a fresh handle-type check. [Intermediate evidence](docs/validation/publication-preparation-8.3-intermediate.json)

## 8.2 / browser 0.71.66 — development build, not released

- Separate publication preparation and pure measured-length completion metadata from its final model-locked transaction. All eighteen targets build, but the first held-ready source-sharing case fails after 11 checks: FILE_READ_ATTRIBUTES does not enforce the intended sharing restriction. The unqualified build is preserved and corrected in subsequent increments. [Intermediate evidence](docs/validation/publication-preparation-8.2-intermediate.json)

## 8.1 / browser 0.71.65

- Run a successful transfer worker's final catalog serialization, writer acquisition and durable IO outside its outer model lock. Keep active worker ownership until the checkpoint attempt settles, then erase only the matching Cancel owner. Final events and prefetch wakeups remain after retirement; failed/deferred worker retirement is unchanged.
- Keep the catalog accessible during this post-publication checkpoint while rejecting duplicate Resume and premature removal. Preserve committed output and original storage-error semantics on a failed checkpoint; an explicit retry restores current durability proof. Newer catalog snapshots still supersede older ones, and shutdown still joins the owned worker.
- Add registered self-contained CompletedCheckpointTests. Its five real 128 KiB HTTP downloads verify independent size/SHA-256, held pre-writer/flush behavior, model access, active ownership, resume/removal gates, error/retry, newer saves, shutdown and catalog reopening. It stops periodic work while body delivery is held to isolate this phase; broader concurrent periodic/transport/media soak remains open.
- Final focused validation passes 595 checks, including 62 new HTTP checks, 261 native completion checks and catalog/coordinator/admission/reservation/detached/publication/queue/decision suites. Sixteen binaries have verified 8.1 resources/hashes; Core and Bridge are rebuilt, forty other shared core objects retain verified hashes, and both manifests are 0.71.65.
- Preserve the initial reservation environment failure after 14 checks. The local tools folder was absent; a junction to installed media helpers fixed lookup with zero tool copies or product-source changes. Its fresh run passes 73 checks. No installer or installation; installed desktop remains 7.11. This removes a post-publication model-lock wait, not disk flush cost or earlier publication locks, and does not establish IDM timing/pixel parity. [Evidence](docs/validation/completed-checkpoint-8.1.json)

- Remove 47 individually verified generated objects, superseded fixtures and owned HTTP/synthetic payloads (174,814,322 logical bytes). Current builds, latest historical fixtures, reusable core, shared tools and structured evidence remain. [Cleanup](docs/validation/completed-checkpoint-8.1-artifacts.json)

## 8.0 / browser 0.71.64

- Release the catalog mutex before accepted Options reads current Windows Startup state, writes the shortcut, compensates a failed change or persists protected Windows connection credentials. Preserve ordering behind an existing catalog owner with a short initial barrier. Current settings merge/persistence and final snapshots retain separate scoped locks.
- Roll back only this attempt's contributions that still match its actually applied settings. Preserve independent scalar replacements/removals, new fields, nested sibling edits and per-entry legacy category changes. Retain later conflicting ordered arrays. Avoid unnecessary rollback writes when the current model already matches the compensated result.
- Compensate Windows Startup only when this attempt tried to change it and the current observed state still matches that attempt. A credential failure with untouched Startup intent cannot undo another owner's external Startup change. Preserve original errors, partial credential acknowledgement, local draft, shared decision guard and explicit retry.
- Final focused validation passes 2,236 checks: 558 Options checks per theme, 319 connection/editor checks per theme, 88 preferences checks per theme, 45 backend checks and 261 completion checks. Six held-provider cases plus twelve rollback semantics checks cover actual catalog access, model concurrency, accepted and compensated state, error handling, retry and catalog reopening. Twelve current native binaries and both manifests are verified. An initial fixture-only Cancel-button failure is retained; only that fixture was rebuilt before its fresh final two-theme run.
- Carry the desktop version after 7.99 to 8.0 under the two-digit policy. This is a development build, with no installer or installation; installed desktop remains 7.11. OS calls may still block the joined writer, and catalog flushes still hold the mutex. Same-value/ABA writes and cross-process OS races are not covered by value-based compensation. Full IDM parity remains incomplete. [Evidence](docs/validation/options-external-8.0.json)

- Remove 28 individually verified generated objects, superseded fixtures and tiny synthetic payloads (315,062,766 logical bytes). Current builds, reusable core objects, shared tools and evidence remain. [Cleanup](docs/validation/options-external-8.0-artifacts.json)

## 7.99 / browser 0.71.63

- Move custom sound-file validation out of Options draft collection and onto the bounded read-only lookup service after Save/Restart. Opening another editor no longer checks the filesystem. Identical nonempty paths are validated once in first-seen event order; Windows-default choices need no check.
- Keep the uncommitted save intent on the UI thread while the worker owns only copied paths and its validator. No catalog lock or global decision guard is taken for this read. Close invalidates delivery without waiting for the held provider; successful delivery alone attempts the existing joined persistence worker. Original errors, local edits and explicit corrected-path retry are preserved. A competing real save cannot have its ownership released by rejected admission.
- Reload sound paths and checkboxes after accepted settings. Collect only checkbox changes relative to the last UI refresh, preserving a concurrently inserted enabled flag across Restart and a later no-edit save.
- Final focused validation: 2,068 checks; twelve verified 7.99 native binaries; both manifests 0.71.63. Real filesystem cases cover an existing Unicode regular file, missing files and directories. Held-read/retry/cancellation/admission cases and light/dark renders are retained. This checks file presence, not WAV decoding or playback.
- Preserve the first incomplete dark connection run separately. Its deadline check prevented failure cleanup; the test driver now retains the first error, writes owned case/phase diagnostics and handles cleanup before checking the expired deadline. A fresh complete maintained two-theme run passes. The original triggering condition is unknown. Browser runtime code is unchanged apart from the requested paired manifest increments. No 7.99 installer or installation; installed desktop remains 7.11. [Evidence](docs/validation/sound-preflight-7.99.json)

- Remove 46 individually verified generated files (610,212,143 logical bytes), retaining current builds, the historical progress fixture, reusable core objects, evidence and shared tools. [Cleanup](docs/validation/sound-preflight-7.99-artifacts.json)

## 7.98 / browser 0.71.62 — development build, not released

- Introduce the separate accepted-save sound preflight and sound-row reload. The native build and lookup/connection/preferences/completion suites passed, but the new Options concurrency case failed after 369 checks: collecting an absent false default as an explicit edit masked an independently inserted true setting. That defect is fixed and qualified in 7.99. An earlier fixture setup failure stopped before the new sound cases and was corrected separately. This intermediate build was not installed or released. [Intermediate evidence](docs/validation/sound-preflight-7.98-intermediate.json)

## 7.97 / browser 0.71.61

- Fix a real main-window message collision: prepared File Info and transfer events both used WM_APP+28, so the earlier File Info handler consumed transfer wake-ups. Transfer events now use WM_APP+31; all twelve private main-window message IDs have a compile-time uniqueness guard. A ready main window can dispatch publication/progress/cleanup events without falling back to periodic refresh.
- Add real message-map routing assertions. Strengthen six completion transitions to take and dispatch the actual queued callback wake-up, covering publication before cleanup, dismissed-dialog deduplication, scan/suppression guards and retained prefetch progress.
- Finish qualifying the 7.96 main Options handoff. Native timer/queue/history/advisory messages return while another owner holds the actual catalog; Cancel returns before release. Detached appearance reads permit concurrent model access during font messages. Deferred queue/event delivery and independently changed native history details resume after closing, and accepted settings survive reopening.
- Final focused validation passes 1,898 checks: 390 Options checks per theme, 319 connection checks per theme, 88 preferences checks per theme, 43 backend checks and 261 completion checks. Thirteen binaries have verified 7.97 resources/hashes; both manifests are 0.71.61. Owned light/dark renderings and the earlier failed diagnostic are retained. No new network transfer, installer or installation; installed desktop remains 7.11. This removes one avoidable wake-up/polling defect, not a proven zero-delay or full-IDM-parity result. [Evidence](docs/validation/main-options-7.97.json)

## 7.96 / browser 0.71.60 — development build, not released

- Separate main Options presentation from periodic model maintenance. The command avoids an unnecessary selection/catalog read before opening, retains coalesced queue work during contention, defers history/events/advisory checks and applies appearance from detached preferences plus toolbar availability.
- All twelve binaries built, but the first actual-main-window regression failed at its queue/event-drain boundary after 358 checks. The diagnostic proved a transfer event remained queued with no active modal, pending save or refresh. Investigation found the existing wake-up ID collision fixed and qualified in 7.97; this intermediate build was not installed or released. [Intermediate evidence](docs/validation/main-options-7.96-intermediate.json)

## 7.95 / browser 0.71.59

- Options construction no longer acquires the Manager mutex or reads the Windows Startup shortcut. A loading shell keeps Help and Close available, ignores premature OK, and populates settings after a nonblocking try-lock succeeds.
- Read Windows startup metadata on the bounded background service with owned providers and a per-action STA. Only the startup checkbox waits; other settings and save remain usable. Original failures appear inline with explicit Retry, and closed dialogs discard late delivery without joining blocked calls.
- Preserve other unfinished edits through advisory reads and retry. Track explicit startup intent, including returning to the stored value after a read failure; the joined save still rechecks current Windows state and preserves external changes when startup is untouched.
- Apply normal theming and original keyboard order to late-created settings pages. Keep temporary Startup status/Retry in unused space left of OK/Help, verified by geometry, edit-menu subclass and tab-order checks plus light/dark fixture renderings.
- Seven initial-load cases plus existing regression suites pass 1,547 focused checks. Twelve binaries have verified 7.95 resources/hashes; both manifests are 0.71.59. Initial passing results and the deliberately stopped intermediate layout rebuild are retained separately. No real Startup shortcut modification, network payload, installer or installation; installed desktop remains 7.11. Parent-window timing and full IDM parity remain unproven. [Evidence](docs/validation/options-initial-7.95.json)

## 7.94 / browser 0.71.58

- Windows connection More opens without synchronously enumerating phonebooks. The shared bounded lookup service retains editable redial settings and Cancel during reads; lifetime/generation checks discard stale or closed-editor results.
- Stage all six connection settings locally. Strict whole-number and range validation precedes publication; invalid input remains editable, Cancel writes nothing and dirty-field acceptance preserves independent caller changes.
- Match the complete connection/phonebook identity, preserve selections across reordered refreshes, and require an explicit selection for ambiguous names. Main Options keeps More accessible so the child can resolve an unselected connection.
- Keep native New, Properties and Connect on the owning UI thread, protecting copied identity and deferring reentrant commands. Refresh only after New/Properties, preserving original errors and numeric edits.
- Eleven standalone cases and three parent integration cases pass in both themes. Final focused validation passes 1,427 checks; 12 binaries have verified 7.94 resources/hashes and both manifests are 0.71.58. The initial passing fixture was strengthened to distinguish stale ordering and exercise invalid numeric bounds before final qualification. No real phonebook/VPN change, network payload, installer or installation; installed desktop remains 7.11. [Evidence](docs/validation/dial-editor-7.94.json)

## 7.93 / browser 0.71.57

- Main Options no longer enumerates Windows connections or reads selected credentials on the UI thread. Two process-wide lookup workers use owned inputs, a 32-item queue and one newest queued action per receiver. The dialog opens with usable General controls while connection details load.
- Closing Options invalidates read delivery without joining blocked Windows calls. Active reads keep their own inputs/stores until return; newer selections and reopened dialogs reject older results. Cached credentials stay editable while refreshing. Dirty drafts reject late metadata, and successful saves invalidate older results before they can overwrite accepted values.
- Lookup errors remain inline. Saving unrelated General settings during pending enumeration preserves an existing enabled connection identity; credential lookup failure leaves Connect available for Windows recovery after delivery is handled. Cancellation and advisory reads write no catalog or credentials; only explicit Apply commits its selected change.
- Fixed a publication/delivery readiness race found by the Options regression: a completed worker result remains pending until consumed by the UI. Six affected local dependency closures were rebuilt; six other binaries retained verified hashes. Initial compile and regression failures are recorded separately.
- Final focused validation passes 1265 checks: credential/lookup cases 238 per theme, Options 286 per theme, customization 88 per theme, lookup backend 22, decision 13 and notification 6. Twelve binaries report 7.93. No real phonebook change, new network payload, installer package or installation. Initial settings/Startup-shell reads and the nested More editor remain synchronous. [Evidence](docs/validation/dial-read-7.93.json)

## 7.92 / browser 0.71.56

- Main Options → Dial Up / VPN → Connect now prepares dirty selected credentials on the joined worker before invoking Windows' connection dialog on the UI thread. The clicked connection/phonebook is captured as an owned identity. The success continuation stays in a separate UI-only member, outside the worker's result and credential batch.
- Credential failure opens no connection dialog and discards that Connect request. An explicit Connect retry creates a fresh request; choosing Apply after the failure saves credentials without unexpectedly connecting. Clean Connect performs no credential write or worker dispatch. Another pending decision requires a later explicit retry rather than queuing an unexpected dialog.
- A connection-dialog failure reports its original error without rolling back already accepted credentials. Retrying Connect opens the dialog without writing them again. Reentrant Connect/OK defer while its UI continuation is handled. Other unfinished connection drafts and unrelated General-page edits remain local; preparation and parent Cancel preserve catalog state and bytes.
- Final focused validation passes 1143 checks, including 188 credential/Connect checks per theme and all six Connect scenarios. Eleven binaries report 7.92. One initial fixture observation failure is preserved; the corrected fixture waits for completed UI handling as well as worker release, and ten other binaries retain verified hashes. No real VPN connection, network payload, installer package or installation. [Evidence](docs/validation/dial-connect-7.92.json)

## 7.91 / browser 0.71.55

- Main Options → Dial Up / VPN → connection Apply now writes and reads the selected connection on the joined decision worker. It snapshots only that connection's protected credential draft; unrelated Options controls and other connection drafts are neither collected nor applied. Clean Apply performs no store write or worker dispatch.
- Pending edits, duplicate Apply, final OK and Close defer while the dialog repaints. Original write errors retain the shared guard through acknowledgement and leave the same entries available for explicit retry. Success masks the accepted password and leaves Apply disabled; restored enablement can no longer overwrite that clean state.
- Explicit connection Apply survives parent Cancel without saving unrelated settings or repeating the credential update. A different unfinished invalid connection identity remains a local draft. Credential-only completion does not reload the whole Options settings draft or write the UDM catalog.
- Final focused validation passes 919 checks: credential behavior (76 per theme), Options (286 per theme), customization (88 per theme), decision (13) and notification (6). Eleven current product/fixture binaries report 7.91. No network payload, installer package, installation or live IDM timing claim. Initial credential reads, Connect's pre-apply path and broader parity/lifetime work remain open. [Evidence](docs/validation/dial-apply-7.91.json)

## 7.90 / browser 0.71.54

- Main Options OK and General Restart now save on the joined decision worker. The UI collects an owned draft, freezes conflicting edits and repaints while the worker waits for the catalog lock or publication. OK closes only after success; Restart runs only after success and keeps the reloaded window open.
- Settings still merge against the current catalog. The worker initializes COM for startup services and preserves the current startup state when that checkbox was unchanged. Credential drafts contain protected password values and no controls. Successfully saved connections are acknowledged after joining, including partial-failure cases, so retry does not repeat them.
- Catalog/startup failures preserve the original error, roll settings back and keep the same edited controls for retry. The shared decision guard remains active until the error is acknowledged. Rollback failures add context to the original error. Cancel before acceptance performs no write.
- Final focused validation passes 800 native checks: Options in both themes, credential behavior, progress customization in both themes, shared decision and lifetime-bound notification checks. Twenty-three native binaries report 7.90. The initial compile/name collision and two fixture failures are preserved separately. No new network payload, installer, installation, live IDM timing or personal-browser claim. [Evidence](docs/validation/options-save-7.90.json)

## 7.89 / browser 0.71.53

- Progress-window customization now opens from a six-field cached view and saves changed choices on the joined decision worker. Cancel and unchanged OK perform no settings write. The worker merges only dirty progress fields into current settings, preserving newer unrelated and untouched progress choices, and delegates to existing validation/publication/rollback.
- More acceptance participates in the parent draft; More/parent cancellation preserve unaccepted values. Failed persistence retains the original error and same dialog/draft for retry. Controls repaint while model/persistence work is held, conflicting actions defer, and posted completion works with polling removed. Committed choices are merged into the caller cache immediately; cancellation cannot replace that cache.
- Added a maintained preferences target/runner with 88 checks per theme, including pure Options-page behavior, real held-write recovery, cached entry and parent-polling-removed cases. Final focused validation passes 1,893 native checks, two actual HTTP transfers and three complete isolated browser workflows. Twenty-one binaries report 7.89; both browser packages are 0.71.53. Installed app remains 7.11; no setup/install. [Evidence](docs/validation/progress-preferences-7.89.json)

## 7.88 / browser 0.71.52

- Automatic completion-close requests retain the progress owner while its context menu is tracking. Both direct close requests and nested refreshes defer destruction until tracking ends; nested popup requests are ignored. The existing opening-rate display grace is still captured and preserved before popup deferral.
- Actual popup regressions exercise close requests and nested timer messages, then normal closure after tracking. A separate grace case verifies retention and uses a test-only deadline expiration to exercise the normal timer close branch. Seven affected UI targets were rebuilt after final grace review; thirteen unaffected targets/core hashes were retained.
- Final focused validation passes 1,717 native checks, two real HTTP limiter transfers and three complete isolated browser workflows. Twenty binaries report 7.88; both packages are 0.71.52 and staged Chromium sources match. The progress-dialog wrapper's initial 30-second observation expired before its process completed all 410 checks; its observation limit now matches the other runners, and a fresh run qualified normal exit without changing fixture assertions. Installed app remains 7.11; no setup/install. [Evidence](docs/validation/progress-popup-lifetime-7.88.json)

## 7.87 / browser 0.71.51

- An idle progress view now opens its cached popup while another window saves a download decision. Hide and Download information remain available; backend-dependent choices are visibly disabled. Choices are checked again after the popup returns, so an item displayed before a competing save cannot bypass that guard.
- Hiding the unrelated view does not wait for or release the other window's task. Read-only Information opens/closes while the other publication remains held. The owning view retains popup deferral during accepted/queued work; synthetic Hide/Information cannot destroy or reenter it. Normal eligible choices return after success or acknowledged failure.
- Menu fixtures pass 81 checks per theme; all focused native checks total 1,703. Two real HTTP transfers and three complete isolated browser workflows pass exact-size/integrity/selected-track/prefetch/hidden-main checks. Twenty binaries report 7.87; 94 browser source hashes and staged Chromium copies verified. Installed app remains 7.11; no setup/install. Broader synchronous dialogs, shutdown/crash behavior and fresh IDM comparisons remain open. [Evidence](docs/validation/progress-menu-busy-7.87.json)

## 7.86 / browser 0.71.50

- Progress context menus now render cached flags from the displayed snapshot without taking the backend lock. Refresh eligibility caches the backend-relevant inputs and encrypted adaptive plan, avoiding repeated plan decryption when unchanged. Later refresh catches up automatically; keyboard invocation anchors to the progress window.
- Copy address, Open folder, Open with and recording Pause/Resume capture owned requests on the joined worker. Current membership, file/scanner/relocation state and recording action are rechecked before returning results or changing the job. Clipboard and Windows shell operations remain UI followups. Recording commands preserve graph responsiveness, original errors, rollback and explicit retry; Resume refreshes limiter controls after clearing a temporary override.
- Added a maintained menu fixture/runner: 53 checks per theme, including actual popup opening under model contention and inert recording publication/error cases. All focused native suites total 1,647 checks; two active HTTP transfers and three complete isolated browser workflows pass. Twenty binaries report 7.86, and staged extension source copies are verified. One initial dark checkpoint failure did not recur and remains unexplained. Shared decision guards and other synchronous commands still need work; full IDM parity remains unproven. Installed app remains 7.11; no setup/install. [Evidence](docs/validation/progress-menu-7.86.json)

## 7.85 / browser 0.71.49

- Additional completion options now open from the cached completion snapshot and submit an owned flags/countdown draft to the joined decision worker. The modal keeps repainting during catalog persistence, disables conflicting inputs, ignores duplicate OK/Close while saving and closes only after success. Failed publication retains the original error, restores the whole job and preserves the same dialog/draft for explicit retry. Cancel after editing writes nothing.
- The worker revalidates download membership, completion status and countdown bounds, retaining existing power/force actions and current completion-dialog policy. Flags and completion plan roll back together. Expanded thin snapshots include every modal setting while excluding unrelated protected request data.
- Light/dark completion fixtures pass 133 checks each; focused native checks total 1,541. Two active HTTP limiter transfers and three ordinary complete isolated browser downloads pass, with an additional instrumented complete recovery. The initial preview HTTP failure recurred before UDM selection and remains unresolved; later passing runs do not prove a fix. Initial fixture expectation failures are retained. Nineteen binaries report 7.85. Installed desktop remains 7.11; no installer/install. [Evidence](docs/validation/progress-options-7.85.json)

## 7.84 / browser 0.71.48

- Completion-tab visibility and power/force settings now capture owned edits and save on the joined decision worker. Graph painting remains available during persistence, while conflicting actions defer. Failed writes retain the original error, restore settings and permit explicit retry in the same window. Workers revalidate membership, completion and current dialog policy before changing a job.
- Force-action consent remains on the UI thread and is required for every enabling attempt. Declined or stale consent cannot publish a forced action. App exit joins existing commands before flushing a completion choice waiting behind another progress window. Completion render snapshots omit unrelated protected request data.
- Added a maintained completion UI target and light/dark runner. All focused native suites pass 1,499 checks; two active HTTP limiter transfers and three production Edge/native HLS workflows pass complete-file validation. The initial dark fixture and browser recovery observer failures are retained in the evidence; both test-only observation errors were corrected before final qualification. Nineteen binaries report 7.84. Installed desktop remains 7.11; no setup or installation. Additional-options/modal commands, crash/session-end behavior and a fresh IDM timing comparison remain open. [Evidence](docs/validation/progress-completion-7.84.json)

## 7.83 / browser 0.71.47

- Speed-limiter edits in the progress window now submit owned numeric/remember choices to the joined decision worker. The rate, enable and remember controls remain editable while saving; one bounded pending value coalesces rapid valid edits. Generation tracking keeps completion or older failures from overwriting newer text, including invalid text and leading zeroes. Remembered-write failures retain the original exception and restore the rolled-back setting when appropriate. Temporary overrides retain their existing nonpersistent behavior. Graph/details remain usable, while conflicting commands and Close defer during pending edits.
- Exit now joins existing progress commands across all windows before flushing queued limiter edits and taking the final catalog snapshot. Added a canonical limiter UI build target/runner, held-write/error/coalescing cases and a tracked in-memory loopback server. Live checks change temporary and remembered rates during active HTTP receipt, verify every completed byte and reopen the saved rate with the completed download.

## 7.82 / browser 0.71.46

- Progress primary actions and accepted Cancel now submit owned commands to a joined worker, rechecking the current download and displayed action before applying backend changes. A displayed Close dismisses immediately without the model lock and cannot cancel a newer transfer. Competing actions stay disabled while graph/detail controls remain usable; posted completion restores controls on failure and supports explicit retry without replacing the window. Backend errors retain their original details. Exit drains accepted commands before the final Manager snapshot and avoids consuming an acknowledged modal exception twice. Login, address refresh and file opening remain UI followups, and login refreshes the limiter afterward. Progress retains its 500 ms polling period through a per-form setting; ordinary forms retain 250 ms.
- Added actual light/dark held-write and forced-prepublication-failure checks for Resume, Pause and Cancel, including duplicate-action rejection and completion with polling disabled. Browser validation also recorded two unresolved preview bootstrap HTTP errors before any UDM selection; successful replacement runs do not establish that issue is fixed.

## 7.81 / browser 0.71.45

- Progress-window refresh now takes a nonblocking model snapshot and keeps the last complete display while another backend thread owns the catalog lock. Quota status is captured with that snapshot, removing a second blocking acquisition during painting. Graph switching, detail resizing and painting remain responsive during contention, and the next available refresh catches up without generating artificial rate samples. Added actual light/dark progress-dialog contention checks before and after snapshot capture.

## 7.80 / browser 0.71.44

- Scheduled-download wizard saves now run on the joined decision worker for new downloads, existing-download scheduling, schedule revision and File Info scheduling. The wizard keeps painting while the catalog is saved, prevents conflicting decisions, and restores the same review draft after a failed save. Completion arrives through the lifetime-bound notification, including when polling is disabled. The delayed review callback retains its heading pointer by value. Added the maintained scheduled-wizard build target and light/dark runner, with success and forced-save-failure recovery across all four entry paths.

## 7.79 / browser 0.71.43

- Ordinary File Info Start and Download Later now submit an owned draft to a joined worker instead of saving on the UI thread. The dialog continues painting during persistence, prevents a conflicting second decision and restores the original field states on error. A lifetime-bound completion signal closes or recovers the form without waiting for a polling timer; completion presentation waits for File Info acknowledgement. Queued individual Start work is dispatched after saving, and periodic main/progress refreshes defer while the decision is pending. Scheduled-wizard commits retain their synchronous path.

## 7.78 / browser 0.71.42

- Eligible browser video downloads can retry a rejected admission from File Info without losing destination/description edits or replacing the window. Retry waits for the previous producer to finish, retains the original receipt token and follows the new accepted job identity. Uncertain outcomes cannot resubmit automatically. Separate joined retry/cancellation workers share presentation ownership and cancellation completion; ordinary polling cannot open a second confirmation while retry owns the offer. Added repeated-failure, existing-receipt, cancellation and close-order tests, plus a production browser test that forces a catalog-write failure, retries in the same dialog and downloads the complete video with an exact pre-Start/final size match.

## 7.77 / browser 0.71.41

- Pending File Info now distinguishes a proven uncommitted rejection from an uncertain acknowledgement. Rejection is delivered only after the owning admission has resolved captured writes without acceptance, outside the Manager lock; observer failure preserves the original persistence error. A successful containing write, accepted result or lost acknowledgement cannot be downgraded to rejection. Uncertain dialogs retain their draft and ask users to check history before recapturing. This establishes the rejection proof needed for inline retry; inline admission retry is still unfinished.

## 7.76 / browser 0.71.40

- Added the Pending File Info fixture to the standard native build selector and a maintained light/dark runner. The runner exercises every held-write decision and destination-conflict recovery case, preserves previous results, restores its environment flags and reports a still-running process instead of silently launching another test. This delivery changes build/verification integration; it does not claim a new download-speed improvement.

## 7.75 / browser 0.71.39

- Errors applying a queued File Info Start or Download Later decision now display the same warning as a direct button action. The queued intent is consumed once, the edited draft stays in the same window, and its controls unlock for correction and retry. Added regression cases for a destination owned by another download in both light and dark modes. A measured native-resource layout prototype was rejected because it increased total time to first paint; the existing layout is retained.

## 7.74 / browser 0.71.38

- Eligible new browser video offers can open File Info from private render inputs while durable admission is still writing. The same window keeps destination and description edits when it binds to the accepted job. Start and Download Later retain one frozen local intent until acceptance; pending Cancel closes immediately and delegates removal of only the still-unconfirmed offer to a joined cancellation worker. Advanced actions enable after acceptance. Prepared-view pruning now runs against pre-modal terminal candidates so newly arriving offers retain their views. Existing-record, refresh and direct-audio variants keep their synchronous fallback.

## 7.73 / browser 0.71.37

- The production browser pipe now delivers prepared File Info views through a bounded application-owned inbox using process-unique integer message keys. Worker tickets preserve accepted versus uncertain outcomes independently of UI lifetime; closing/restarting integration cannot reuse a stale key. Accepted views feed the existing dialog only when their complete render inputs still match the live job; changed offers fall back to a fresh snapshot. Early display and pending decisions remain unfinished.

## 7.72 / browser 0.71.36

- File Info now initializes from an immutable owned render snapshot, including category choices, without retaining a live job in that view. Eligible detached media admission can hand this snapshot to an in-process consumer outside the Manager lock before persistence; callback failure preserves uncommitted retry behavior. The existing dialog still opens after durable acceptance while early presentation and pending decisions are integrated.

## 7.71 / browser 0.71.35

- New standalone media jobs now persist from detached reservations without holding the manager lock through disk I/O. Pending destinations remain reserved, history excludes the tentative job, and uncertain recovery queries cannot release an in-flight token. Publication resolves against all captured writes before failure; a successful containing write preserves acceptance even when the original writer fails. Existing/duplicate/refresh and direct-audio admissions retain their synchronous path. File Info presentation still follows durable admission.

## 7.70 / browser 0.71.34

- Media admission now owns an in-flight token and prepared job/destination lease. Persistence snapshots capture the prepared reservations they include, and a successful publication marks only those entries durably accepted without allocating after the write. Pending ownership survives outstanding snapshot tickets, and a proven acceptance cannot be rolled back by a later error. Admission and File Info presentation remain synchronous while detached preparation and pending UI decisions are implemented.

## 7.69 / browser 0.71.33

- Media admission now binds an immutable job/receipt reservation into its detached catalog snapshot before persistence. Conflicting token, job or destination data is rejected before publication; overlays preserve unrelated entries and cannot silently omit the reserved receipt/job pair. Browser acceptance remains synchronous; concurrent reservation ownership and early File Info presentation are still pending.

## 7.68 / browser 0.71.32

- Catalog writes now use a shared commit coordinator with explicit published, already-published and superseded outcomes. Older or repeated sequences cannot write over a newer checkpoint; failed writes preserve the durable sequence for retry, and commit results return after releasing the writer lock. Browser acceptance remains synchronous while the pending-admission architecture is developed.

## 7.67 / browser 0.71.31

- Automatic catalog checkpoints now check whether state still needs saving in the same lock as their snapshot. A completed explicit save can supersede queued automatic work without a redundant durable write. Explicit saves, storage-error retries and transaction deferral retain their existing persistence behavior.

## 7.66 / browser 0.71.30

- Main-window startup binds ten controls directly to the saved appearance font after construction, avoiding an earlier temporary-font assignment and its synchronous font/layout messages. Default/custom fonts, column-header and toolbar fonts, DPI-scaled metrics and runtime appearance changes remain covered by the owned font fixture.

## 7.65 / browser 0.71.29

- File Info resolves category label collisions in memory using the selector's Windows locale, equal-length matching and native empty-search behavior. Repeated category searches no longer send combo-box messages; a Windows comparison error falls back to the native search. Category keys, aliases and redraw batching remain preserved.

## 7.64 / browser 0.71.28

- File Info batches category-selector rebuilds with redraw suspended, preserving native locale-aware label collision checks, category-key selection, missing-key fallback and previously hidden controls. This avoids intermediate list redraw work during initial construction and after adding a category.

## 7.63 / browser 0.71.27

- Explicit sort, category and search transitions start at the top of the effective download view, including changes that leave row order unchanged. Automatic history updates keep their viewport anchor. Navigation fallback is resolved before comparing views, avoiding a delayed reset after an unavailable category falls back to All Downloads.

## 7.62 / browser 0.71.26

- Main-window layout snapshots read Windows column controls before taking the shared download catalog lock, so download/native operations can access the catalog during those UI queries. Only changed layout data is assigned; unrelated concurrent preferences, resized/reordered columns and sort settings remain preserved. This removes a reproduced lock-contention path, without claiming an overall File Info latency fix.

## 7.61 / browser 0.71.25

- File Info queues its first prefetch pulse after painting instead of waiting for a generated low-priority timer. Queued notification traffic can no longer postpone initial admission until the queue empties. Earlier queued input keeps its order; recurring updates and the timer failure fallback remain available. This addresses prefetch scheduling, not the separate control-construction delay.

## 7.60 / browser 0.71.24

- Disconnect cancels native requests still waiting on validation or legacy replies. Late results cannot reconnect or dispatch an abandoned selection. Includes the 7.59 deadline increment in the final 7.60 native build; no separate 7.59 package or release was created.

## 7.59 / browser 0.71.23

- Bound the complete native request lifecycle, including startup, browser validation and legacy one-shot acknowledgements. Preserve one safe capture-status retry within a bounded two-attempt budget; never replay an unacknowledged download. Delivered together with 7.60.

## 7.58 / browser 0.71.22

- Resolve and snapshot only the selected primary display address for dialogs. A valid requested URL no longer decrypts its secondary redirect, and a selected video no longer parses an unused audio URL. Preserve resolved/audio fallbacks when no primary is present; the full Links view and address search still use all links.

## 7.57 / browser 0.71.21

- Keep Download complete available when its saved encrypted media address cannot be read. Mark the address unavailable without guessing another URL or erasing the saved context. Preserve local file actions, scanner gates and the normal completion layout.

## 7.56 / browser 0.71.20

- Decode gzip responses into a temporary sibling of the selected destination, avoiding a second decoded-file copy and hash across drives during ordinary publication. Keep encoded recovery data, flushes, checksum checks and the publication journal; destination changes during confirmation retain the existing relocation fallback.

## 7.55 / browser 0.71.19

- Accept the `x-gzip` HTTP content-coding alias through the existing verified gzip path. Keep canonical gzip metadata for exact decoded size, retained responses and encoded-offset resume, including a server switching between the two equivalent names.

## 7.54 / browser 0.71.18

- Clip the speed graph to its actual 30-second window. Interpolate a crossing line at the left edge and exclude expired samples from the visible peak and hover selection, preserving gaps during pauses.

## 7.53 / browser 0.71.17

- Avoid a second normalized query allocation on hot Find Next cache hits. Capture an immutable query witness only before fresh matching, recheck cache identity for every candidate, and retain a candidate reference across matching. Cancel a stale scan if the query or visible candidate changes instead of selecting an outdated row.

## 7.52 / browser 0.71.16

- Keep query-bound Find results when the search bar is hidden, and reuse verified positive and negative results during unfiltered Find Next. Preserve category/queue eviction and source/query invalidation. Bound retained search snapshots to a 32 MiB logical payload/node budget and 8,192 entries with least-recently-used eviction; oversized snapshots are matched without retention. Refuse to publish a result into another query's cache if the query changes during matching.

## 7.51 / browser 0.71.15

- Reuse source-checked filter results during Find Next instead of copying and decrypting an unchanged protected capture again after list refresh. Changed records and unfiltered navigation retain outside-lock matching. Find Next with all search fields disabled returns immediately, following the retained IDM guard. Keep cursor, wrapping and selection behavior intact.

## 7.50 / browser 0.71.14

- Reuse main-window search-filter results before cloning encrypted searchable metadata. Refreshes compare only selected fields while holding the job lock; changed records are copied once and matched outside it. Preserve negative results, query invalidation and removed-job eviction. Correct the dormant sigma test using the byte-identical IDM specimen's Windows-uppercase search path rather than changing matching semantics to satisfy the old assumption.

## 7.49 / browser 0.71.13

- Compare progress-window address inputs by reference before copying them. Unchanged legacy playlist and streaming captures no longer clone their encrypted plan on every refresh; changed sources, track bindings and manifest-address precedence still update the address.

## 7.48 / browser 0.71.12

- Keep protected request bodies, captured page plans and full recovery receipts out of progress-window refresh copies. Derive form Resume/Restart state from the original record and retain only the receipt/state summaries used by the display. Compare saved form metadata by reference instead of copying segment arrays and header strings; keep worker verification and transport data unchanged.

## 7.47 / browser 0.71.11

- Recognize an ASCII-only PDF after a completed HTML/body notice before classifying the response as HTML. Require a standalone numeric PDF version header and whitespace after the closing notice; preserve embedded HTML/script examples, incomplete windows and the existing 2,000-byte prefix bound. This resolves the text-only PDF case exposed by the 7.46 form-recovery qualification.

## 7.46 / browser 0.71.10

- Preserve completed plain HTTP form responses after verification/content/publication failures, using request-bound receipts without adding a receipt/hash pass to successful downloads. Reopen File Info with verified final size, including approved prefix removal, or resume after Download Later, catalog reopening and destination changes without another form submission. Keep damaged-response Restart explicit, preserve the original failure reason and pending publication recovery, and discard obsolete response receipts after completion.
- Qualification found an ASCII-only PDF notice case still classified as HTML; 7.47 fixes that separate detector gap. This intermediate development build was not installed or packaged.

## 7.45 / browser 0.71.9

- Decode gzip form responses, including POST-to-GET redirects. Bind completed response receipts to a fingerprint of the protected form request, headers and redirect identity; Resume verifies saved bytes instead of resubmitting the form. A failed verification requires a separately displayed Restart before another submission. Keep fresh POST forms out of speculative prefetch. Preserve wire/output size separation and recheck the confirmed final checksum before publication.

## 7.44 / browser 0.71.8

- Decode ordinary GET responses served with HTTP gzip content encoding, retaining encoded-byte offsets and validators for safe resume. Verify gzip members and checksums before publication; retain reusable encoded prefetch receipts after Download Later. Show verified decoded size rather than compressed Content-Length in File Info and output progress. Completed records retain both wire and output counts. POST and other encodings retain their existing admission policy.

## 7.43 / browser 0.71.7

- Preserve long explicit alphanumeric filename extensions during content verification. A name such as old.udmfixture or package.unitypackage no longer gains an extra content-type suffix solely because its extension exceeds eight characters. Keep numeric version handling and generic/script suffix correction; recognize the existing long temporary crdownload suffix under the same correction policy.

## 7.42 / browser 0.71.6

- Keep browser responses with a fully received positive byte total in the browser before creating a capture journal or pausing a legacy transfer. If the body finishes during pause, resume the original response without a native Add. Preserve capture eligibility for both -1 and zero length placeholders: an isolated Edge run reported zero for both an empty response and an unfinished chunked response. Completed empty responses still use the browser's complete state.

## 7.41 / browser 0.71.5 — rejected development candidate

- Built a received-response candidate that treated a zero total as proof of an empty, received response. The real Edge byte-counter check contradicted that assumption: unfinished chunked responses also reported zero. This candidate was not installed or packaged and is superseded by 7.42 / browser 0.71.6.

## 7.40 / browser 0.71.4

- Recheck a browser download after policy/request preparation and before pausing it for legacy desktop capture. Downloads canceled, completed, removed or paused during that wait stay untouched, and failed state lookups send no download. Keep the existing post-pause check so a user resume can still prevent handoff; unchanged live transfers retain one acknowledged desktop submission.

## 7.39 / browser 0.71.3

- Replace superseded same-timestamp speed-graph states instead of retaining duplicate drawable samples. A pause immediately after a measurement no longer leaves that retired value in the trace/peak; a same-tick resume retains the latest state without growing history. Later measurements and real pause intervals retain their existing history.

## 7.38 / browser 0.71.2

- Refresh an open audio/subtitle chooser from the current format catalog. Retain selected track keys and updated labels, block stale handoffs when a selected track disappears, and require an explicit replacement or None/default choice. Restore returning selections, update revoked audio-only actions, preserve keyboard focus, and send the chosen current track keys.

## 7.37 / browser 0.71.1

- Bind staged sensitive Properties header values to the target origin present when the dialog opened. Reject a late scheme/host/port change before applying Authorization, Cookie, Referer or Origin edits, preserving the current session and typed draft. Keep same-origin refreshes, ordinary header edits, explicit clearing and untouched-session commits available.

## 7.36 / browser 0.71.0

- Merge Properties request edits per logical header under the manager's commit lock. Login, Referer and Advanced header edits preserve unrelated cookies and authentication refreshed after staging or completed-file relocation. Preserve explicit removals and empty Authorization overrides, skip unnecessary re-encryption, and retain staged edits after protection failure. Roll browser versions from 0.70.99 to 0.71.0.

## 7.35 / browser 0.70.99

- Preserve an incomplete download's current destination when Properties' Save To field is untouched. External folder/filename changes survive acceptance before refresh and late changes at the commit boundary. Explicit path edits still take precedence and undergo the existing validation, including empty-path rejection and correction without losing the draft.

## 7.34 / browser 0.70.98

- Submit only explicitly changed Properties addresses and descriptions. Untouched fields now merge from the manager's latest record under its commit lock, preserving updates after acceptance staging and across completed-file relocation; captured-media addresses are not resubmitted from an old snapshot.

## 7.33 / browser 0.70.97

- Keep untouched Properties addresses and descriptions current when another workflow refreshes the download. Saving before a timer refresh preserves those live values; explicit edits remain the user's draft. Preserve selection offsets while refreshing an untouched address and keep selections in typed fields intact.

## 7.32 / browser 0.70.96

- Keep Properties field enablement stable during refresh. Captured-video login and password controls remain disabled instead of briefly enabling on each timer tick; ordinary fields change state once on background start/stop while retaining typed text and selection. Apply the same state policy during initialization.

## 7.31 / browser 0.70.95

- Accept a reused player response on a returning visit when its known media revisions advance, none regress or disappear, and the player is showing the requested content. Keep unchanged, title-only, backwards and ambiguous revisions retired. Compare bounded decimal revision strings without losing large-integer precision; retain API authority over supplementary getter metadata.

## 7.30 / browser 0.70.94

- Keep accepted player-API metadata authoritative when a getter returns fresh clones of stale data. Preserve API format revisions, transfer URLs, titles and lifecycle state while allowing getter-only formats to supplement partial metadata and update their own revisions. Getter additions cannot evict API formats at the 200-format bound; a newer accepted API response renews authority.

## 7.29 / browser 0.70.93

- Bound pending streaming captures before response headers arrive, release their copied bodies on retirement, and prevent late body production from restoring retired state. Clean up XHR capture listeners on navigation, abort and reuse, preserve native second-send errors, and inspect only a bounded XHR prefix while the caller retains the complete response.

## 7.28 / browser 0.70.92

- Bound and retire streaming response and streamed request-body inspections, with four retained readers of each kind and 30-second deadlines. Navigation, newer verified sessions and fetch failures release observer clones while preserving the site's uploads and playback bodies. Use request sequence ordering for equal timestamps in both page and browser-level capture; keep ordering metadata out of streaming offers and separate sequences between worker instances.

## 7.27 / browser 0.70.91

- Release obsolete player-API metadata readers on navigation or newer accepted metadata, cap concurrent inspections at four, and add a 30-second inspection deadline. Reject advertised oversized responses before cloning, and release oversized-body inspections without waiting for playback's original response to finish. These limits apply to the metadata clone, preserving the site's request, response body and promise.

## 7.26 / browser 0.70.90

- Bind YouTube player-API metadata to its actual visit and request order. Reject late fetch/XHR responses after navigation, stale cloned-body results, mismatched video responses and failed XHR responses. Retire reopened XHR listeners, keep an old initial snapshot from overwriting accepted API data, and prevent previously seen page objects from repopulating a later visit. Preserve site promises, response bodies and valid same-URL history updates without adding format requests.

## 7.25 / browser 0.70.89

- Keep Properties request edits transactional through header validation and credential-protection failures. Retrying invalid input remains rejected, correcting it retains the other typed login changes, and Cancel preserves the saved request. Untouched fields keep a refreshed session and avoid unnecessary credential re-encryption.

## 7.24 / browser 0.70.88

- Keep File Properties current when a download starts, stops or completes while the dialog is open. Preserve typed edits, recheck activity before actions and Save, and use the completed-file metadata transaction after completion. A destination entered before completion is applied through the existing physical file-move workflow; a failed move retains the draft for correction.

## 7.23 / browser 0.70.87

- Keep the speed graph's 30-second timeline moving during pauses, expire old rates, and show inactive hover intervals as “No transfer” instead of borrowing a neighboring rate. Preserve measured zero rates, display tenths of a second, and keep the selected readout visible in narrow charts. Sampling and idle timeline redraws remain limited to two per second.

## 7.22 / browser 0.70.86

- Block transport-level POST replay after an authentication continuation loses its response, including empty submissions. Fresh authentication rejections still permit their continuation, and explicit user restarts can submit again.

## 7.21 / browser 0.70.85

- Retain proven server capacity through transient refusals after a productive connection closes. Pace recovery opens and require fresh, sustained evidence before lowering the learned bound, so paused downloads can release their slots promptly.

## 7.20 / browser 0.70.84

- Preserve receive-timeout identity through proxy and authentication continuations. Silent response deadlines consume the configured retry budget and no longer establish shared or within-transfer connection-cap evidence.

## 7.19 / browser 0.70.83

- Classify a connection lost during proxy or Digest authentication continuation as a receive disconnect. Automatic proxy authentication can then use existing connection retry and shared admission without treating a final rejection or malformed response as capacity evidence.

## 7.18 / browser 0.70.82

- Identify proxy admission by the decrypted credential fingerprint rather than its randomized encrypted storage blob. Independently saved copies of the same proxy login share capacity evidence while distinct users and passwords stay isolated.

## 7.17 / browser 0.70.81

- Resolve Windows automatic proxy settings for each exact HTTP URL before connection admission. Pin the returned direct or proxy route to the transport, preserve returned failover order, and share capacity evidence across matching resolved routes.

## 7.16 / browser 0.70.80

- Retain recent refused opens while matching responses await their first payload. Learn the shared connection bound only after payload progress, so slow first reads do not discard earlier refusal evidence.

## 7.15 / browser 0.70.79

- Coordinate HTTP admission across downloads sharing a resolved route and request identity. Park connections when repeated refusals establish occupied capacity, preserve reusable idle slots, and keep admission waits cancelable.

## 7.14 / browser 0.70.78

- Check completed file locations in the background while Properties opens. Refresh the location after a file is removed or restored, and distinguish missing files from lookup errors.

## 7.13 / browser 0.70.77

- Open File Properties while Windows resolves the file icon and type in the background. Refresh those details when a move changes the filename extension.

## 7.12 / browser 0.70.76

- Show queue names directly in the Start Queue and Stop Queue menus, including names with literal ampersands.

## 7.11 / browser 0.70.75

- Improve the connection map with light and dark palettes, quarter marks, and clearer connection-start markers.

## 7.10 / browser 0.70.74

- Add a 30-second speed graph to the expanded download progress window. Switch between the graph and connections without resizing the dialog. Hover readings stay above the plot.

## 7.9 / browser 0.70.73

- Reuse the cross-drive range buffer for eligible responses up to 64 MiB. Check for cancellation between destination writes and retain the disk-copy path when the buffer is unavailable.

## 7.8 / browser 0.70.72

- Reuse the received ranges in one bounded buffer for eligible cross-drive HTTP files up to 16 MiB. Stage and hash those bytes without rereading the resumable partial.

## 7.7 / browser 0.70.71

- Skip normalization and writeback of the disposable source partial after cross-drive publication. Keep writeback on the published file.

## 7.6 / browser 0.70.70

- Stage fresh cross-drive HTTP downloads up to 4 MiB from their verified write buffers. Record staging and durable publication times separately.

## 7.5 / browser 0.70.69

- Show Download Complete without a synchronous Shell icon lookup or a second blocking catalog read during dialog initialization. Apply the file-type icon when the Shell worker returns it.

## 7.4 / browser 0.70.68

- Hash fresh direct HTTP files up to 4 MiB from their write buffers and reuse the bounded content prefix at completion. This avoids rereading the same bytes from disk before showing Download Complete.

## 7.3 / browser 0.70.67

- Show Download Complete after committing the published file. Filesystem cleanup and advisory hashing continue before file actions and completion commands become available.

## 7.2 / browser 0.70.66

- Save a deferred SHA-256 with the final download checkpoint to remove an extra catalog flush before the completion dialog.

## 7.1 / browser 0.70.65

- Begin saving the first HTTP response without a second catalog write when a new download has no partial files to recover.

## 7.0 / browser 0.70.64

- Keep the first HTTP response moving while a second connection is admitted. Persist the new range plan with the existing periodic or pause checkpoint.

## 6.99 / browser 0.70.63

- Explain in the extension popup why the last automatic download stayed in the browser, without storing its URL, filename or credentials.

## 6.98 / browser 0.70.62

- Hash adaptive media segments as they are written, so committing a completed segment does not require reopening and rereading it.

## 6.97 / browser 0.70.61

- Keep single downloads quiet while the popup waits for UDM and ignore repeat clicks on a link that is already being handed off.

## 6.96 / browser 0.70.60

- Refresh Download File Info more frequently during final media assembly and verification so the exact size appears sooner.

## 6.95 / browser 0.70.59

- Keep the existing main window hidden when another background-only desktop launch arrives.

## 6.94 / browser 0.70.58

- Start the desktop immediately when the browser host confirms UDM is closed, and coordinate concurrent startup requests.

## 6.93 / browser 0.70.57

- Commit ordinary command-line downloads once at admission, with their final queue or confirmation status already set.

## 6.92 / browser 0.70.56

- Adapt parallel HTTP read sizes to each response so a slow worker can release a range sooner after another connection takes over its tail.

## 6.91 / browser 0.70.55

- Keep duplicate replacements from claiming a backup path already reserved under a different Windows path spelling.

## 6.90 / browser 0.70.54

- Protect a destination reserved by another download when Save As uses a different spelling of the same Windows path.

## 6.89 / browser 0.70.53

- Refresh a changed server filename before accepting Use server name, keeping the Save As draft intact until the displayed suggestion is chosen.

## 6.88 / browser 0.70.52

- Keep the current Save As filename when choosing a recent folder in Download File Info.

## 6.87 / browser 0.70.51

- Apply the same scanner and relocation checks to file drags from the main list and Download Complete.

## 6.86 / browser 0.70.50

- Make Enter invoke the focused action in Download Complete in both light and dark mode.

## 6.85 / browser 0.70.49

- Apply Download Complete suppression as soon as the checkbox changes, so later downloads respect it while the current dialog is still open.

## 6.84 / browser 0.70.48

- Resume older HTTP downloads directly from a numbered partial file without a separate byte-zero probe. Keep the source part until migration is committed.

## 6.83 / browser 0.70.47

- Check the saved file's length and allocated bytes before the first resume request so sparse holes and truncation use the correct offset immediately.

## 6.82 / browser 0.70.46

- Recover an ordinary saved download when its first continuation is refused with HTTP 403, 405 or 501. Fall back to a full GET and keep the old checkpoint until the server accepts it.

## 6.81 / browser 0.70.45

- Keep progress-window clicks and keyboard commands bound to the action shown when a download changes state between refreshes. Close leaves a newly queued download alone.

## 6.80 / browser 0.70.44

- Offer Resume for paused and failed partial downloads while the server’s resume capability is still unknown.

## 6.79 / browser 0.70.43

- Skip JSON routing responses and HTML pages when discovering streaming playlists. Reuse HLS and DASH manifests the player already fetched after a redirect, including final URLs without a file extension.

## 6.78 / browser 0.70.42

- Resume saved HTTP partials with a strong ETag and untested range support using the first continuation response. A rejected continuation restarts from a full response.

## 6.77 / browser 0.70.41

- Keep the File Properties Open button synchronized with scan and relocation state, including commands already posted before a scan starts.

## 6.76 / browser 0.70.40

- Let Enter activate the focused, selected download through the same action as double-click.

## 6.75 / browser 0.70.39

- Keep Download Complete file actions and drag handles in sync with scanner and relocation state.

## 6.74 / browser 0.70.38

- Apply the scanner and saved-file checks to Open and Open with in the main download list.

## 6.73 / browser 0.70.37

- Keep progress-window file actions unavailable while a completion scan is pending or the job is still busy. Stop wait applies only to an active scanner.

## 6.72 / browser 0.70.36

- Close an idle progress window without changing the download record or canceling its completed-file scanner wait. Cancel still stops active and queued downloads.

## 6.71 / browser 0.70.35

- Show the exact saved size of eligible media replacements as soon as the response length and removable prefix are known.

## 6.70 / browser 0.70.34

- Show the size from an active replacement response as soon as its headers arrive. A response without a length remains pending until the transfer reaches EOF.

## 6.69 / browser 0.70.33

- Reserve a pending HTTP replacement link for its download response. File Info avoids metadata and remote ZIP preview requests while the replacement is waiting for validation.

## 6.68 / browser 0.70.32

- Start expired-link replacements with a full GET when there are no saved bytes to resume. A one-use link can supply the whole file in its first response.

## 6.67 / browser 0.70.31

- Apply the File Info transfer-slot wait to replacement links after an expired address. Brief waits use the replacement download response for size and file details.

## 6.66 / browser 0.70.30

- Let File Info reuse the download response for file details after a short wait for a transfer slot. Longer waits can still check an unknown size.

## 6.65 / browser 0.70.29

- Leave browser credentials and response details out of manual link imports when multiple requests for the URL disagree on the account.

## 6.64 / browser 0.70.28

- Keep Download Complete open and show the moved-file message if a directory has replaced the downloaded file at its saved path.

## 6.63 / browser 0.70.27

- Keep the progress window’s opening prefetch rate based on measured transfer intervals. A brief timer slice at Start can no longer inflate a previously measured peak.

## 6.62 / browser 0.70.26

- Skip the automatic File Info metadata request when the browser already supplied both the filename and size. Start still verifies the download response, and Refresh details still works on request.

## 6.61 / browser 0.70.25

- Show an available server filename and enable its adoption when Download File Info first appears. Preserve the current Save As path until the user chooses the suggestion.

## 6.60 / browser 0.70.24

- Show available media sizes when Download File Info first appears. Distinguish selected-track totals and estimates from verified final file sizes.

## 6.59 / browser 0.70.23

- Bind background capture rules to the exact observed GET so a form submission or an unrelated request cannot be handed off as a download.

## 6.58 / browser 0.70.22

- Apply current desktop site rules before taking over an ordinary browser download, including rules edited since the browser last synced its settings.

## 6.57 / browser 0.70.21

- Check current desktop rules, file types and exclusions before automatic background capture. A blocked attempt can be tried again after settings change.

## 6.56 / browser 0.70.20

- Expand the default browser capture list to cover more archives, installers, documents, media, models and datasets. Saved file-type lists are preserved.

## 6.55 / browser 0.70.19

- After a pause or restart, use the saved byte offset on the first request when a strong Last-Modified date can validate the partial file.

## 6.54 / browser 0.70.18

- Skip the startup range probe for single-connection downloads with a strong Last-Modified date when the server omits Accept-Ranges. If the response drops, verify the date and saved overlap before resuming.

## 6.53 / browser 0.70.17

- Apply the saved connection limit and queue retry settings when direct video and audio tracks start. A one-connection download fetches the tracks in sequence.

## 6.52 / browser 0.70.16

- Set the maximum connections and speed limit when scheduling from Download File Info. The limits apply to the future run, which can reuse a verified prefetch.

## 6.51 / browser 0.70.15

- Escape and title-bar Close now cancel a fresh browser download just like Cancel in Download File Info. Previously saved downloads keep their paused data.

## 6.50 / browser 0.70.14

- Keep force-download and bypass shortcuts on browser-confirmed redirects when the download event reports only the final URL.

## 6.49 / browser 0.70.13

- Move several completed downloads into one folder with Ctrl+M. UDM keeps each filename and reports where it stopped if a move fails or is cancelled.

## 6.48 / browser 0.70.12

- Keep Download all tied to the formats shown when the mouse button goes down, even when a catalog update arrives before release.

## 6.47 / browser 0.70.11

- Return keyboard focus to the video panel trigger when a refreshed format list removes the focused quality or Download all action. Require a deliberate new selection.

## 6.46 / browser 0.70.10

- Keep an in-progress background download active when ZIP Preview opens. The preview waits for the same response, and Cancel closes only the preview.

## 6.45 / browser 0.70.9

- Read ZIP Preview from a complete File Info prefetch without making another request or interrupting the waiting download.

## 6.44 / browser 0.70.8

- Resolve the current saved file when a completion drag starts, including immediately after an in-app Move or Rename.

## 6.43 / browser 0.70.7

- Return keyboard focus to the field that needs correction in the Schedule Download wizard and select invalid text for replacement.

## 6.42 / browser 0.70.6

- Open the saved server login challenge on a manual retry of a failed queue download. The queue stays silent and Cancel leaves the job available without making another request.

## 6.41 / browser 0.70.5

- Reuse the first remaining-range response after an address refresh when both responses establish the same strong Last-Modified validator and the saved bytes match at the resume boundary.

## 6.40 / browser 0.70.4

- Verify whole-file SHA-256 from the legacy HTTP `Digest` header when the origin provides no usable modern digest. Block publication if the downloaded bytes differ.

## 6.39 / browser 0.70.3

- Validate a refreshed download address with the remaining range when the saved file has a strong ETag and a valid range plan. Reuse the response after checking the range and saved boundary.

## 6.38 / browser 0.70.2

- Refresh an expired download address from File Info. Save the replacement link, then return to the same confirmation with its destination draft intact.

## 6.37 / browser 0.70.1

- Keep permanent HTTP rejections visible in Download File Info and wait for an explicit retry when the dialog reopens.

## 6.36 / browser 0.70.0

- Keep File Info from automatically probing or restarting a download that needs website verification. Show the browser step and allow an explicit retry with Start Download.

## 6.35 / browser 0.69.99

- Replace the browser's size hint once a direct prefetch accepts its own response. If the response has no length, show that the final size is pending until the body finishes.

## 6.34 / browser 0.69.98

- Find the first available numbered filename without rescanning download history for each suffix. Existing files, backup destinations, and active moves still reserve their paths.

## 6.33 / browser 0.69.97

- Reuse one duplicate lookup during a browser handoff. Compare the method and URL before opening a saved form request so unrelated protected downloads cannot delay or block File Info.

## 6.32 / browser 0.69.96

- Reduce the work before browser Download File Info opens on a large catalog. Read capture receipts in place and limit rollback copies to records the handoff can change.

## 6.31 / browser 0.69.95

- Explain when a website requires browser verification. Keep the challenge page out of the saved download and stop automatic retries that require browser interaction.

## 6.30 / browser 0.69.94

- Use the native preparation reply directly during automatic handoff. Query the native journal when recovering an interrupted handoff.

## 6.29 / browser 0.69.93

- Reuse one desktop policy and capability reply during automatic download handoff. Complete the checks before pausing a browser download on the legacy path.

## 6.28 / browser 0.69.92

- Bind a prepared browser handoff to the final response URL. If that URL changes, keep the original browser transfer.

## 6.27 / browser 0.69.91

- Recheck automatic capture and site exclusions before canceling a browser download with a prepared native handoff.

## 6.26 / browser 0.69.90

- Recheck credential sharing after native preparation and before canceling an authenticated browser download, including during recovery after a browser worker restart.

## 6.25 / browser 0.69.89

- Keep downloads that used Authorization or sent cookies for a cookie-varying response in the browser when credential sharing is unavailable.

## 6.24 / browser 0.69.88

- Keep range-bound downloads in the browser. Automatic capture and background site rules no longer start a full native transfer for a ranged request or partial response.

## 6.23 / browser 0.69.87

- Route iframe attachments through the ordinary browser download handoff so a background site rule cannot start a second UDM transfer.

## 6.22 / browser 0.69.86

- Keep the actual response size visible if background prefetch stops, and show the interruption in compact Download File Info.

## 6.21 / browser 0.69.85

- Keep the server-reported size in File Info while background prefetch connects. Replace it when the download response supplies its own metadata.

## 6.20 / browser 0.69.84

- Start reading single-connection HTTP downloads immediately when the server provides an ETag but omits Accept-Ranges. Check range support only if the connection needs to resume.

## 6.19 / browser 0.69.83

- Check SHA-256 supplied in HTTP digest headers before publishing a direct download. Keep the received data if the server checksum differs.

## 6.18 / browser 0.69.82

- Load the selected browser video offer and current tab policy together before inspecting the player.

## 6.17 / browser 0.69.81

- Reuse the desktop preferences already requested for the media receipt when checking indexed DASH support for a single selection.

## 6.16 / browser 0.69.80

- Dispatch a queued browser download after saving the job or committing the capture when File Info is skipped. Keep confirmation, Download Later, and queue limits in place.

## 6.15 / browser 0.69.79

- Dispatch a saved silent command-line download or explicit queue start as soon as the desktop can process it. Queue limits and schedule rules still apply.

## 6.14 / browser 0.69.78

- When you choose a recent destination in Download File Info, remember the folder you selected for the category.

## 6.13 / browser 0.69.77

- Set the per-file speed limit and maximum connections when scheduling a direct link or a waiting download. Review the limits before saving and reopen the schedule to edit them.

## 6.12 / browser 0.69.76

- Show the full destination while editing the folder or file name, add start and stop time shortcuts, and give the schedule review more room.

## 6.11 / browser 0.69.75

- Use the desktop preferences already returned for video batch review to check indexed DASH support before requesting media indexes.

## 6.10 / browser 0.69.74

- Share desktop capability and media preference lookups across the selected formats in one video batch review.

## 6.9 / browser 0.69.73

- Read recorded HLS captions alongside the selected video and audio without waiting for a desktop live-subtitle capability check.

## 6.8 / browser 0.69.72

- Ask for a fresh HLS selection if a video or audio playlist in a recorded format now reports a live stream.

## 6.7 / browser 0.69.71

- Let the selected recorded HLS playlist use the browser's HTTP cache after fresh discovery. This can avoid a repeat request when the server allows reuse.

## 6.6 / browser 0.69.70

- Skip cached playlist bodies when handing off a selected direct video or a saved DASH plan. Validate the stored offer against the current player before preparing the handoff.

## 6.5 / browser 0.69.69

- Keep captured playlist bodies out of browser checks that only revalidate the selected player. Discovery still reads the playlists it needs, and handoff still checks the player and permissions.

## 6.4 / browser 0.69.68

- Use one disk read for the final SHA-256 and saved-range verification even when dynamic splitting saved the ranges out of order. Keep each range paired with its original receipt hash.

## 6.3 / browser 0.69.67

- Reuse the same disk read for range verification and the final SHA-256 when resuming a completed prefetch stored as one file with ranges in order.

## 6.2 / browser 0.69.66

- Show the exact saved size for eligible direct media as soon as the completed response and its header establish it. The reusable prefetch integrity check can finish in the background.

## 6.1 / browser 0.69.65

- Accept Resume immediately after Download Later. Queue the request while the previous worker finishes, then use the normal resume path when it releases the saved data.

## 6.0 / browser 0.69.64

- Let a fully received direct prefetch finish saving its integrity receipt when you choose Download Later. Resume can reuse the checked bytes without requesting the file again.

## 5.99 / browser 0.69.63

- Reuse recent Windows file icons and type names in Download File Info. Keep completed lookups available across overlapping dialogs and use a cached icon on the first paint.

## 5.98 / browser 0.69.62

- Skip automatic File Info metadata requests after an HTTP or FTP rejection. Respect saved retry delays and pending login prompts before starting prefetch.

## 5.97 / browser 0.69.61

- Keep Download File Info's exact prefetch size tied to the login used by that response. Label metadata-only lengths as server-reported while the matching transfer is still unverified.

## 5.96 / browser 0.69.60

- Show completed media input bytes as Downloaded in the completion dialog when verified track sizes are available. Show the exact saved file size separately when assembly changes it.

## 5.95 / browser 0.69.59

- Suggest a stop one hour after the selected start when enabling a one-time deadline in Scheduler or the download wizard. Preserve the suggested instant through a repeated clock hour, and keep saved deadlines and in-dialog edits when toggling the stop.

## 5.94 / browser 0.69.58

- Schedule a waiting download from its context menu. Set a one-time window and adjust the destination and retry options while keeping the existing download and its progress.

## 5.93 / browser 0.69.57

- Open a new scheduled download directly from Tasks. Enter its address and optional login, choose a destination and start or stop date, set retry and wake options, and review the draft before saving.

## 5.92 / browser 0.69.56

- Apply media-host permission checks to cached playlists during discovery and desktop handoff. Reuse authorized playlists without a repeat request.

## 5.91 / browser 0.69.55

- Recheck media-host permissions immediately before desktop handoff for fetched HLS playlists and DASH indexes, including batch review.

## 5.90 / browser 0.69.54

- Reconcile a retried HLS or DASH selection with its saved desktop receipt before reopening the playlist. Recover the original download if the browser lost its acknowledgement.

## 5.89 / browser 0.69.53

- Divide the first HTTP split by the planned worker count so odd worker sets start with more even byte assignments.

## 5.88 / browser 0.69.52

- Keep midpoint splits when the initial HTTP worker count is a power of two. Use share-based splits for other worker counts.

## 5.87 / browser 0.69.51

- Balance initial HTTP ranges by limiting each new helper to its share of the file while connections ramp up.

## 5.86 / browser 0.69.50

- Give a fresh HTTP download one routine tail split beyond its initial range assignments. Stalled or much slower connections can still get additional help.

## 5.85 / browser 0.69.49

- Checkpoint live HTTP range splits with normal progress so a helper can request its range without waiting for a catalog flush.

## 5.84 / browser 0.69.48

- Reuse the background HTTP hash reader and wake it immediately when the network phase finishes. Leave catalog access to transfer workers when the manager lock is busy.

## 5.83 / browser 0.69.47

- Verify adjacent HTTP ranges using the tail bytes UDM just wrote. Read the staging file when those bytes came from an earlier attempt or were split across writes.

## 5.82 / browser 0.69.46

- Reopen a waiting one-time download in the scheduling wizard to change its destination, category, description, start and stop dates, retry policy, or wake setting. Review and save the changes together.
- Let the wizard set file retries to off, a fixed count, or no count limit.

## 5.81 / browser 0.69.45

- Stop the background prefix hash between chunks when HTTP workers finish. Retain the verified prefix and record separate timing for worker shutdown, range checks, and hash shutdown.

## 5.80 / browser 0.69.44

- Restore the 5.77 HTTP range allocation policy after two healthy-tail experiments failed to improve request counts consistently.

## 5.79 / browser 0.69.43

- Evaluate short HTTP tails as soon as parallel responses are established, before an idle worker can create a small extra range.

## 5.78 / browser 0.69.42

- Keep short, steadily receiving HTTP tails on their existing connections once a fresh transfer starts finishing ranges. Idle workers can still help longer tails and recover missing bytes.

## 5.77 / browser 0.69.41

- Stop making speculative splits for the rest of a fresh transfer after an extra range receives an HTTP 403. Keep the recovery path for unfinished ranges.

## 5.76 / browser 0.69.40

- When an extra HTTP range gets a 403 while other responses are still open, return its untouched bytes to the original response where possible and pace further connection attempts.

## 5.75 / browser 0.69.39

- Skip redundant font redraws and theme work while constructing native dialogs. Fill the File Info category picker without resetting its empty list.

## 5.74 / browser 0.69.38

- Display the endpoint from the selected tracks when opening a newly captured or renewed YouTube stream. File Info, Progress and Download Complete can show the address without unpacking the protected session.

## 5.73 / browser 0.69.37

- Save a fresh browser video offer and its staging path together. File Info can use that saved record to start prefetch without another catalog write.

## 5.72 / browser 0.69.36

- Start prefetch for an unchanged direct browser offer using its existing durable confirmation record. Skip a second catalog write before the first request.

## 5.71 / browser 0.69.35

- Open Download Complete without copying the download's transfer journals or both track records. Preserve the displayed address, exact saved size, destination, and scanner result.

## 5.70 / browser 0.69.34

- Start the completion dialog's file-icon drag from the pointer's screen position so Windows detects the gesture at the correct threshold.

## 5.69 / browser 0.69.33

- Configure a failed-file retry count and delay in the scheduling wizard. Review them alongside connection retries and wake settings before saving the download and its queue.

## 5.68 / browser 0.69.32

- Open the scheduling wizard directly from Download File Info. Choose a recent folder, browse or type a destination, and set start and optional stop dates and times. Today and Tomorrow shortcuts make setting a start date quicker. Included in the 5.69 build.

## 5.67 / browser 0.69.31

- Keep the completion dialog's saved path, file icon, drag handle, and Open actions in sync after a completed download is moved or renamed in UDM.

## 5.66 / browser 0.69.30

- Show the exact saved size of eligible direct media while its prefetched response is still arriving. Use the declared length and recognized file prefix from the same response. Responses with no declared length still show the exact size after EOF.

## 5.65 / browser 0.69.29

- For prefetched direct downloads that can strip inserted text, distinguish the raw response size from the exact file size. Check the prefetched bytes before Start and apply the existing media content handling to newly admitted direct YouTube audio.

## 5.64 / browser 0.69.28

- Clear the visible size when a replacement download address needs validation. File Info identifies a size from the new address's metadata check as server-reported and shows the verified size once the replacement transfer is validated.

## 5.63 / browser 0.69.27

- Keep the size already visible in Download File Info when prefetch is waiting for a transfer slot. Skip a separate metadata request in that case and verify the byte count when the actual response arrives. Files with an unknown size can still check their metadata while waiting.

## 5.62 / browser 0.69.26

- Use a renewed YouTube audio URL when the selected track and resource still match. Resolve browser request details and reported size for the current URL and verify it again before sending the download.

## 5.61 / browser 0.69.25

- Skip a new YouTube player lookup when the browser already captured a usable stream for the selected quality. Reuse a ready direct pair and retain the native lookup fallback for unavailable or expired captures.

## 5.60 / browser 0.69.24

- Prepare YouTube formats as soon as UDM verifies the main video's playback stream, even before the player reports metadata. Keep unrelated ad panels hidden and reuse the prepared list on the first click.

## 5.59 / browser 0.69.23

- Guide scheduled downloads through Destination, Dates and queue, Options, and Review. Change the folder, filename, category and description, choose a start and optional stop date and time, and set connection retries and wake behavior. Jump back to any step from Review before saving.

## 5.58 / browser 0.69.22

- Keep an open track chooser available for review when its exact format disappears. Pause its download actions until that format returns or the user goes back to select a current choice.

## 5.57 / browser 0.69.21

- Keep keyboard focus on the video panel's format and track controls when more choices arrive. Back from track selection returns to the control that opened it.

## 5.56 / browser 0.69.20

- Restore a range's original boundary when an extra connection is refused and the accepted response can still deliver its untouched tail. Save the restored plan before the worker writes those bytes.

## 5.55 / browser 0.69.19

- Treat a range response as reusable only after all its response bytes were read. A worker that stopped at a shortened range boundary follows the same admission pacing as a new connection.

## 5.54 / browser 0.69.18

- Expand fresh HTTP range downloads one new connection at a time while responses are active. After a refused open, give accepted connections time to finish work before trying another extra connection.

## 5.53 / browser 0.69.17

- Match the completion dialog's plain read-only field borders to the reference. Open and Open with both explain when the downloaded file was moved and keep the completion dialog available.

## 5.52 / browser 0.69.16

- Show the verified size of recorded HLS and DASH prefetches without waiting for a catalog write at the start of assembly. Start still saves the confirmation before publishing the file.

## 5.51 / browser 0.69.15

- Schedule a download from File Info with a three-step wizard. Edit its save folder, filename, description, start date, optional stop date, retry limit and wake preference. Review the settings before saving to a dedicated queue that can be edited in Scheduler.

## 5.50 / browser 0.69.14

- Assemble recorded HLS and DASH from the durable segment receipts already on disk. Keep the recovery checkpoint that binds a refreshed selection before requesting missing segments.

## 5.49 / browser 0.69.13

- Write each adaptive segment receipt without holding the catalog lock. File Info and other segment workers can continue updating while Windows flushes the receipt.

## 5.48 / browser 0.69.12

- Keep an HTTP range with the worker already waiting for its response headers. An idle connection can reuse unclaimed work without canceling and sending that same request again.

## 5.47 / browser 0.69.11

- Paint File Info after a single initialization and theme pass. Media confirmations skip the server-filename controls that are only used for ordinary files.

## 5.46 / browser 0.69.10

- Keep the scanner completion dialog's actions and suppression checkbox within its client area when Windows uses different frame metrics during DPI changes.

## 5.45 / browser 0.69.9

- Capture YouTube streaming calls made with a Fetch Request object while preserving the request body delivered to playback. Bound the amount of request data copied by the observer.

## 5.44 / browser 0.69.8

- Capture verified YouTube streaming requests even when playback reads the response immediately. The player still receives its original fetch promise and response.

## 5.43 / browser 0.69.7

- Use the newest eligible YouTube streaming request from the current video visit and keep late responses from replacing newer captures.

## 5.42 / browser 0.69.6

- Recover YouTube from its verified continuous prefix. After a secondary connection requires fresh attestation, resume that download on one connection once the user saves a matching replacement.

## 5.41 / browser 0.69.5

- Keep YouTube streaming on one connection until a timed segment passes verification. Show when retained setup data has yielded zero playable progress.

## 5.40 / browser 0.69.4

- Request the saved YouTube video quality with one panel click on the recovery page. Keep the matching replacement available for review in the desktop dialog.

## 5.39 / browser 0.69.3

- Save the media working folder with the prefetch confirmation record so the transfer can start after one durable catalog write.

## 5.38 / browser 0.69.2

- Give the File Info dialog a chance to process queued input and paint before it saves the background download admission.

## 5.37 / browser 0.69.1

- Open compact Download File Info without building the controls behind More. Keep the retained media status visible from the first paint when the selected session needs refresh.

## 5.36 / browser 0.69.0

- Open media recovery from Resume in the download list. Start ordinary downloads in a mixed selection before asking for fresh media sessions, and stop opening prompts when the user cancels.

## 5.35 / browser 0.68.99

- Show verified progress and the refresh action in Download File Info when a YouTube session expires. Open session recovery directly from the progress window's primary button.

## 5.34 / browser 0.68.98

- Hold a YouTube download when the server explicitly requests session renewal. Keep the rejected window, playback cursor and request number for diagnosis without storing request credentials in the rejection record.

## 5.33 / browser 0.68.97

- Open YouTube session recovery at the saved playback position when a single streaming window has verified continuous media.

## 5.32 / browser 0.68.96

- Start YouTube streaming with one request. Open the other timeline connections as soon as selected media arrives, and hold the job for refresh if the origin rejects the session first.

## 5.31 / browser 0.68.95

- Continue a resumed YouTube stream when the server repeats an already verified segment across responses. Discard the repeated bytes without adding them to progress.

## 5.30 / browser 0.68.94

- Hold YouTube streams that require fresh browser attestation. Keep verified segments and resume only after the matching media session is refreshed.

## 5.29 / browser 0.68.93

- Keep the edited run-once stop date and time in the open Scheduler when Apply saves the stop as disabled. Re-enabling it restores the edited deadline.

## 5.28 / browser 0.68.92

- Let range workers restore parallelism faster after a complete connection outage. Keep the slower retry rate when healthy responses are still transferring.

## 5.27 / browser 0.68.91

- Keep the pending video menu open when the same player reloads without changing its source. Discard the stale format lookup and carry the keyboard selection intent into the replacement catalog.

## 5.26 / browser 0.68.90

- Share retry slots among range workers after a connection fails before response headers. Keep healthy ranges moving and probe more often when no response is active.

## 5.25 / browser 0.68.89

- Keep the next wake scheduled when a daily queue's disabled stop time matches its start time. The wake timer and queue runner now use the same effective stop time.

## 5.24 / browser 0.68.88

### Use current format details at browser handoff

- Send the verified YouTube track lengths and direct address pair from the final check before handoff. If the player loses a length, clear the old hint.
- Delay speculative HLS subtitle requests when the desktop lacks live subtitle support. Recorded subtitles can still use the normal capability checks.
- Keep repeat attempts separate in the transfer benchmark and isolate each run from existing app instances.

## 5.23 / browser 0.68.87

### Verify YouTube subtitle downloads before accepting the file

- Check that the downloaded response is a complete TTML document containing caption text before keeping a finished prefetch or publishing the file.
- Reject empty and malformed replies and reset the saved transfer so retry fetches a new response. The check applies to YouTube timed-text addresses requesting TTML.

## 5.22 / browser 0.68.86

### Reuse YouTube's observed caption request

- Prefer the address from a successful caption request in the same video visit when it matches the selected track and signed parameters.
- Preserve the playback request's session parameters when requesting TTML. Fall back to the player track address when no observation matches.

## 5.21 / browser 0.68.85

### Offer YouTube subtitle tracks alongside video formats

- List available manual and auto-generated subtitle tracks as separate TTML entries. A subtitle selection uses the normal File Info handoff, and Download all includes subtitle rows in the batch review.
- Recheck the video visit, selected track and caption address before handoff. TTML can be disabled in browser panel settings.

## 5.20 / browser 0.68.84

### Reuse the prepared YouTube format for desktop warmup

- When a prepared menu opens, verify the current player visit and reuse its format metadata for desktop warmup. Fall back to a full read if the background cache has expired or been cleared.
- Keep the prepared metadata in memory. The download itself still revalidates the selected format before handoff.

## 5.19 / browser 0.68.83

### Reuse prepared YouTube qualities in Browser Tools

- Opening Browser Tools reuses the current video's prepared qualities when available. If the player has no current list, Show video qualities starts a lookup on demand.
- The tools page keeps the selection disabled until it has choices and shows any lookup error without a reading message.

## 5.18 / browser 0.68.82

### Show recorded DASH byte coverage in the connection map

- Show received ranges and active connection starts across the selected tracks when every part has a known length.
- Include verified saved prefixes and remove bytes that must be fetched again when a transfer falls back to an older checkpoint.

## 5.17 / browser 0.68.81

### Track recorded DASH progress by bytes when lengths are known

- Use the total source bytes for percentage and time remaining when every recorded DASH part has an exact length. Show the total as track data while the final output size is pending.
- Label downloads whose parts all support protected partial recovery as Yes (saved ranges). Use segment counts when source lengths are incomplete.

## 5.16 / browser 0.68.80

### Reduce redundant work in the progress window

- Update the caption, status, buttons and transfer fields only when their displayed values change. Publish the final status text once per refresh.
- Skip copying connection rows and segment maps when the Details panel is collapsed.

## 5.15 / browser 0.68.79

### Use the saved stream address throughout download dialogs

- File Info, progress, the progress window's Copy download address command and Download Complete use the protected playlist or manifest address when available. Older records still use their existing source data.
- Progress updates copy only the inputs needed to resolve the address and leave the full capture plan and session credentials out of the progress snapshot.

## 5.14 / browser 0.68.78

### Show playlist addresses for more recorded streams

- Carry the selected recorded HLS playlist or DASH manifest through the browser handoff, including the actual selected audio playlist for audio-only HLS.
- Show that address in File Info and Download links. Treat it as display metadata when matching a replacement address or saved selection.

## 5.13 / browser 0.68.77

### Show source addresses for recorded DASH with Period timing

- Keep the captured manifest address in the browser handoff so File Info can show it. The address is display metadata and does not change saved selection identity.
- Show the first captured Period's track links in Download links. Earlier captures without a manifest address use those links as a fallback.

## 5.12 / browser 0.68.76

### Reduce work before Download File Info appears

- Copy the fields File Info needs without duplicating the job’s full transfer record, track journals, or settings.
- Save the protected display address when accepting or refreshing an HLS or DASH capture. File Info can display it without reopening the entire protected plan. Older records use the existing lookup.

## 5.11 / browser 0.68.75

### Prioritize File Info over pending browser reviews

- Present accepted browser downloads before browser batch review and the Other qualities message, both on capture notification and on the fallback timer.
- Keep browser review inside the same refresh guard so captures arriving during a modal dialog are handled on the next pass.

## 5.10 / browser 0.68.74

### Show prefetch progress when the download finishes immediately

- A normally visible progress dialog can show its opening prefetch rate even when saving finishes before the Start event reaches the window.
- If automatic closure is enabled, let the opening rate finish displaying before closing the progress window. Completion actions can run immediately.

## 5.9 / browser 0.68.73

### Show the measured prefetch speed when progress opens

- After Start, briefly show the measured prefetch peak as the main Transfer rate and label it as activity before Start. The field then returns to the live rate.
- Keep time remaining and the Speed Limiter tab tied to the current transfer rate.

## 5.8 / browser 0.68.72

### Verify partial DASH segments when refreshing a download address

- Keep eligible recorded DASH prefixes when the server rejects an expired or inaccessible link.
- Compare saved prefixes against the corresponding range from the replacement link before downloading missing data. A failed comparison leaves the saved bytes available for another refresh.

## 5.7 / browser 0.68.71

### Recover recorded DASH segments after an unexpected exit

- Save protected checkpoints during eligible transfers, with at least 1 MiB of new data and two seconds between periodic writes. Pause can still checkpoint smaller amounts.
- On restart, verify the latest usable checkpoint and truncate any unverified tail before requesting the remaining range. A flushed temporary receipt can also be recovered.

## 5.6 / browser 0.68.70

### Resume partially saved recorded DASH segments after Pause

- Save an eligible unfinished segment and a protected checkpoint when its transfer pauses. Resume verifies the saved length, SHA-256 and selection before requesting the remaining range.
- Remove the checkpoint when its segment completes or when the associated browser download is discarded.

## 5.5 / browser 0.68.69

### Continue interrupted recorded DASH segments

- Keep the unfinished segment during an automatic retry when a strong ETag identifies the captured resource. Request and verify the exact remaining byte range before appending it.
- Keep the existing completed-segment recovery across restarts. Incomplete bytes are discarded on cancellation or terminal failure.

## 5.4 / browser 0.68.68

### Wake a YouTube selection when the browser captures its stream

- A browser-observed streaming session wakes the matching tab, frame, document and video after the session is stored.
- Keep the timed fallback and validate the current stream and selected tracks before sending the request to the desktop.

## 5.3 / browser 0.68.67

### Reuse the confirmed YouTube format read

- Reuse the selected video’s format snapshot when it contains the captured stream session or when a playback retry has just found the selected transport.
- Recheck the current video, tracks, session validity and settings before dispatching the download.

## 5.2 / browser 0.68.66

### Keep prepared video choices usable on feeds

- Keep background format lookups from evicting another player’s cached choices. When the user opens a menu during preparation, give that lookup foreground priority.
- Make room for an explicit request by evicting whole player catalogs, including their associated DASH data. Notify the affected frame so it can drop stale choices.

## 5.1 / browser 0.68.65

### Prioritize formats for the video the user is choosing

- Give the hovered or keyboard-focused download strip priority over other visible players preparing in the background.
- When the user opens a menu that still needs formats, let that lookup finish before preparing another player.

## 5.0 / browser 0.68.64

### Respect server retry delays on recorded streams

- Apply the existing HTTP retry rules to recorded HLS and DASH segments. An explicit retry delay on a transient rejection defers new segment requests to the same origin within the transfer.
- Keep completed segments available after a failure and report the actual HTTP rejection when sibling workers stop.

## 4.99 / browser 0.68.63

### Read saved HLS segments directly during assembly

- Use a seekable, local byte stream for eligible recorded HLS tracks with up to 32 saved parts. Avoid making a second full track file before muxing.
- Fall back to the existing track copy if FFmpeg cannot read or verify the streamed input. Subtitled captures, checksum-pinned downloads, and tracks that require a local HLS playlist retain their established assembly paths.

## 4.98 / browser 0.68.62

### Share HLS playlist reads within Download all

- Let MP4, TS and other selected HLS rows reuse the same playlist response during batch preparation. Keep each row’s output and track selection independent.
- Bound the temporary cache to the current batch. Recheck requested and redirected playlist host permissions before sending the desktop review.

## 4.97 / browser 0.68.61

### Offer declared HLS audio variants as M4A

- List explicitly AAC-only HLS variants as audio downloads. Variants with video metadata or alternate rendition groups keep their existing choices.
- Send the audio-only variant through the existing recorded or live audio transfer path and distinguish standalone variants in browser recovery receipts.

## 4.96 / browser 0.68.60

### Share format discovery across YouTube Download all

- Reuse one fresh player catalog across the selected qualities after the desktop capability check. Direct and streamed choices still use their own transport checks.
- Recheck every prepared quality against the current video immediately before sending the native review.

## 4.95 / browser 0.68.59

### Skip a duplicate format read for direct YouTube downloads

- When the first lookup already has the selected direct video pair or M4A address, prepare the handoff from that result. Check the current document, selection, and permissions again before sending.
- Keep the additional catalog read for selections that wait on the native player or a browser streaming session.

## 4.94 / browser 0.68.58

### Recognize refreshed links for locally renamed downloads

- Address refresh can match the saved server filename for a capture from the same download origin or confirmed recovery page. It also uses the new response filename when supplied by the browser.
- Recheck the match when committing a browser capture. A changed match goes to review, and saving a replacement still requires the existing saved-part validation before resuming.

## 4.93 / browser 0.68.57

### Use the originating document for download-page recovery

- When a GET request lacks a full source-page URL, use the browser’s current navigation record only if the tab, frame, document ID and request initiator match. Skip the hint while a navigation update is pending.
- Keep those page hints in extension memory. Source-page discovery adds no browser API call during handoff.

## 4.92 / browser 0.68.56

### Keep the original download page across hosts

- Preserve the browser-reported page for an observed GET separately from its HTTP Referer. Download history and replacement-link review can then use the original page even when the file comes from a different host.
- Require a full page match for a differently named replacement. A cross-origin Referer reduced to the site origin alone does not qualify.

## 4.91 / browser 0.68.55

### Ask about unrecognized replacement links

- During address refresh, a browser-captured link with a different filename from the original recovery page can now open a review. Choose to review it as a replacement, download it separately, discard it, or leave it for later.
- Keep the captured request protected until a choice is made. The original address and saved parts still require the existing review and resume checks.

## 4.90 / browser 0.68.54

### Reduce waiting work in download address recovery

- Keep the media refresh dialog idle between changes to a current captured session. Reuse its decoded candidate and avoid repainting unchanged controls.
- Limit refresh dialog presentation lookups to the selected download. Saving or resuming still rechecks the captured candidate and its review receipts.

## 4.89 / browser 0.68.53

### Move keyboard focus into the video format menu

- Opening the video strip with Enter or Space focuses the first available menu command when formats are ready. A single eligible format still uses the existing direct handoff.
- Holding Enter does not activate a newly focused command through key repeat. Arrow keys and Escape keep their existing navigation and dismissal behavior.

## 4.88 / browser 0.68.52

### Load gallery toolbar skins that depend on stock small icons

- Accept skins such as Flat2015 and Globe when their two references to the stock small icon strips are absent. Use UDM’s built-in small 3D buttons for those skins in Small Buttons mode.
- Load the skin’s own large artwork and use its small artwork when both strips are supplied. Keep validation for other missing or malformed images.

## 4.87 / browser 0.68.51

### Verify refreshed stream segments as they arrive

- When a recorded HLS or DASH link is refreshed, compare the length and SHA-256 of replacement bytes as they arrive. Reuse saved segments only after the local files and replacement bytes match their receipts.
- Skip creating and rereading temporary copies for verification. New segments still go through the existing file and receipt checks.

## 4.86 / browser 0.68.50

### Prepare the video menu when a paused player’s strip is approached

- Start format discovery when the user hovers over or focuses the download strip on a paused or ended video with loaded metadata. Keep the existing source, visibility, and policy checks.
- Show selectable formats as soon as they are ready. Announce continued discovery to assistive technology and reserve visible messages for useful notes or errors.

## 4.85 / browser 0.68.49

### Show the assembled media size while verification runs

- Display the measured byte count in File Info as soon as the assembled YouTube or recorded HLS/DASH output is locked for verification.
- Keep the assembled size separate from the verified final size. A canceled or failed verification clears the temporary display, and publication and saved-output receipts still require successful verification.

## 4.84 / browser 0.68.48

### Close stale video menus when the page or player changes

- Cancel the old format lookup and close the menu when a single-page navigation or changed player source replaces the video.
- Clear prepared choices for the old video. A download handoff already underway keeps its original identity and confirmation flow.

## 4.83 / browser 0.68.47

### Reduce idle video player discovery work

- Separate routine positioning and catalog retries from full player discovery. Discover inserted players from DOM changes and use a bounded fallback for late shadow roots.
- Skip discovery while the document is hidden and rescan when it becomes visible. Walk the existing shadow discovery limit without collecting every element on the page.

## 4.82 / browser 0.68.46

### Reduce video panel layout work during playback

- Skip continuous pointer-move placement passes when hover-only mode is off. Hover-only mode still responds to pointer and keyboard focus changes.
- Measure eligible players before moving their overlays and reuse shared ancestor styles and clipping bounds during each pass.

## 4.81 / browser 0.68.45

### Open the video menu when its first choices are ready

- Keep a pending click on the video strip instead of showing an empty popup during format discovery. Show available rows or actionable feedback as soon as the lookup produces it.
- Keep keyboard focus on the strip while waiting and announce the pending state to assistive technology. Closing the panel, switching sources or leaving the page cancels a pending opening.

## 4.80 / browser 0.68.44

### Retry an empty video menu when capture becomes ready

- Signal the originating browser document after saving a newly observed HLS or DASH playlist. An eligible blob player can retry format discovery immediately, including when its menu is already open.
- Use YouTube's existing verified playback signal to wake the same retry path. Limit capture-driven retries to one per player identity and preserve choices already shown.

## 4.79 / browser 0.68.43

### Prepare video choices as playback starts

- Wake background format preparation on playback and metadata events rather than waiting for the next periodic scan.
- Prepare choices for eligible playing videos before the pointer arrives in hover-only mode. The panel still appears according to the hover setting.

## 4.78 / browser 0.68.42

### Discover direct and streaming video formats in one lookup

- Send verified direct file choices through the existing progress channel while the same lookup reads HLS and DASH playlists.
- Keep early rows usable while richer choices arrive. Callers without progress delivery retain the two-request direct preview fallback.

## 4.77 / browser 0.68.41

### Start the desktop handshake when the prepared video menu opens

- Begin the native host handshake while the user reads a prepared quality list. Opening the menu does not wait for the handshake.
- For YouTube choices that may need a native player pair, recheck the current video and warm that lookup off the menu path. Passive format preparation still performs no native startup or player-pair retrieval.

## 4.76 / browser 0.68.40

### Prepare video quality menus during playback

- Look up verified formats for a visible playing video before the user opens the panel. Open from the ready list or reuse the lookup already underway.
- Keep choices tied to the current page, player source and settings. Refresh runs a fresh lookup, and download handoff still verifies the selection.
- Limit preparation to one player lookup at a time per document. Background preparation reads format metadata without warming the desktop or retrieving a native player pair.

## 4.75 / browser 0.68.39

### Reuse the FTP metadata connection for the first transfer worker

- Let one worker take the authenticated startup session, revalidate file metadata and begin data transfer without logging in again. Reissue the restart offset immediately before RETR on a session used to probe restart support.
- Fall back to a fresh login if the reused session cannot set up the transfer. A file identity mismatch still stops the download. Replacement-link byte verification and confirmation waits for non-resumable FTP release the original session.

## 4.74 / browser 0.68.38

### Reuse the FTP control session for the final file check

- Reuse the authenticated session that received the successful end-of-file reply for the final size and timestamp check. This avoids another connection and login when the session is still responsive.
- Give the reused session a two-second budget for both metadata commands. Fall back to a fresh login if the reused session cannot answer the checks or when all ranges were already saved. A changed file still fails validation before publication.

## 4.73 / browser 0.68.37

### Assemble the same numbered parts verified for a saved prefetch

- Keep verified FTP and legacy HTTP prefetch parts open through staging so a path cannot be replaced or opened for writes between receipt verification and assembly.
- Keep receipt-based repair for damaged parts. Reuse one read buffer during verification and another during assembly. Continue hashing the staged output as it is written, including when UDM removes an inserted content prefix.

## 4.72 / browser 0.68.36

### Match the browser panel frame and controls

- Add the continuous beveled blue frame from the IDM reference to the full and compact video and selected-link strips. Match the inset controls, compact button height and green arrow with UDM artwork.
- Keep the compact quality menu aligned with the right edge of the strip and add the inner highlight visible in the reference menu.

## 4.71 / browser 0.68.35

### Match the Other qualities prompt and one-row review

- Show Other qualities in a Windows message box owned separately from the main UDM window. Keep the browser prompt available when the desktop does not support the request.
- Show Download all above a single format when the picker also offers Other qualities. The command opens the desktop review, while selecting the format opens Download File Info.

## 4.70 / browser 0.68.34

### Review YouTube video qualities together

- Download all prepares the selected YouTube video qualities for one desktop review. Direct playback URLs and captured SABR tracks use the same validation and protected storage as individual video downloads.
- Recheck the video visit, selected qualities and stream identity before sending the batch. If a quality cannot be prepared, identify it before sending anything. Batch preparation does not switch the web player's playback quality.

## 4.69 / browser 0.68.33

### Review direct files and adaptive video together

- Send final non-YouTube video catalogs to one desktop review, including mixed direct files and HLS or DASH formats. Adaptive rows keep their captured track plans and origin-scoped request context. Their source address and credentials are fixed in the review.
- Add checked rows in the existing catalog transaction. Adaptive rows bypass ordinary file-size probes against the source page, and different stream formats can share that page address in the list.

## 4.68 / browser 0.68.32

### Review direct video formats together

- Download all opens one desktop review when every format in the catalog is a direct media file. Each row shows its quality and receives a distinct suggested filename. The desktop adds only the rows checked in the review.
- Recheck the originating video, document, capture settings, and browser request context before handing off the batch. A review with an uncertain acknowledgement is never retried as individual downloads.

## 4.67 / browser 0.68.31

### Guide users to additional qualities in the web player

- Add Other qualities to the video menu when UDM has captured a playable format from a single blob-backed player without reading a variant catalog. The prompt explains how to select and play another quality, then reopen the panel.
- Keep the menu open for explicit selection when that command is offered. The guidance closes when the player changes or the panel is disabled.

## 4.66 / browser 0.68.30

### Open File Info directly for a sole confirmed video format

- One activation of the video strip sends a sole confirmed format through the usual desktop confirmation path. The picker remains available for provisional results, additional tracks, warned or live streams, and explicit keyboard browsing.
- A canceled or replaced catalog cannot select a format. After a handoff error, reopening the panel offers the choice and shows the warning before another handoff.

## 4.65 / browser 0.68.29

### Match the compact video menu spacing

- Give titled format rows the trailing space visible in IDM's compact menu. Reduce that space when the viewport or desktop menu width setting is narrow, and keep the full and options menus sized to their contents.
- Show the video strip after its first placement so it starts next to the player.

## 4.64 / browser 0.68.28

### Align browser panel and right-click controls with IDM

- Keep the numbered video format menu separate from panel controls. Open panel controls with the question mark on the full strip, or by right-clicking or pressing Shift+F10 on the compact strip. Opening options does not look up video formats.
- Give the compact format list a gray menu gutter and retain the format's own quality label.
- Show the tab toggle and UDM Browser Tools on the toolbar menu. Match IDM's enabled and disabled toolbar tooltip layout, and offer the link command on HTTP and HTTPS images as well as links and direct media.

## 4.63 / browser 0.68.27

### Prefetch resumable FTP downloads from File Info

- Start receiving into numbered parts before confirmation when FTP resume can be verified. Check file details and wait for Start Download before opening a data transfer on non-resumable servers.
- Keep publication behind confirmation. A completed FTP prefetch can be verified from its numbered parts on a later Start, even if the server is unavailable.
- Exclude the time waiting for confirmation from transfer and elapsed measurements.

## 4.62 / browser 0.68.26

### Keep Download File Info updates lightweight

- Copy only the fields the dialog displays and the two track sizes and addresses. Leave saved ranges, capture plans and private request context on the live job during polling.
- Check eligibility against the live job, including its current tracks and any draft login. Explicit preview commands still capture the request context they need.

## 4.61 / browser 0.68.25

### Open Download File Info before setting up background work

- Begin prefetch and metadata work from a posted dialog update after initialization. Skip automatic prefetch if a draft login or an explicit metadata check is already in use.
- Report a background startup error in File Info and keep the confirmation controls available.
- Update the displayed size, filename, status, and preview controls only when their values change.

## 4.60 / browser 0.68.24

### Reduce catalog work before browser video confirmation

- Use the live jobs as the in-memory history after startup, so a save does not also copy the original history array.
- Prepare the media request fingerprint before taking the catalog lock. Check an existing receipt without copying the journal.
- For video and adaptive captures, save only the armed refresh record for rollback. Validate the adaptive plan at admission and commit the download and receipt together.

## 4.59 / browser 0.68.23

### Begin HTTP resume with the missing range

- For a saved download with a consolidated partial file, a complete range plan and a strong ETag, request an unfinished range directly with `If-Range`.
- Reuse a matching response after checking the local offsets, then verify the overlap bytes before writing. Keep the existing probe for refreshed links, numbered-part-only layouts and nonmatching partial responses.
- If the server returns a complete response, retain it for the existing restart path and apply the saved-identity protection policy.

## 4.58 / browser 0.68.22

### Recover downloads after a stalled HTTP response through a proxy

- Report a stalled response as a header or body interruption so ordinary downloads use the existing retry and resume rules.
- Keep an accepted original response running if an optional range helper stalls.
- Keep response stalls out of proxy-route failover. An interrupted form submission still requires an explicit restart.

## 4.57 / browser 0.68.21

### Set how long UDM waits for a server response

- Set a server response wait of 1 to 300 seconds from Options → Connection → More. Existing profiles default to 30 seconds.
- Use the setting for HTTP and HTTPS receives on WinHTTP and explicit proxy routes, and for FTP replies and data reads.
- Apply changes when the next transfer or preview takes its settings snapshot.

## 4.56 / browser 0.68.20

### Show actual connection starts and coverage in download progress

- Place HTTP and FTP markers where each active connection starts saving new bytes, excluding the overlap it rereads to verify the file.
- Show the byte coverage of separate video and audio tracks when their sizes are known. Audio-only transfers now show their connection rows.
- Fill the map when the file is complete, including a saved output that has no remaining range records.

## 4.55 / browser 0.68.19

### Hand adjacent HTTP ranges to a worker that has finished

- Take the next untouched range when its assigned worker is still opening the HTTP request.
- Cancel the displaced request and discard any late reply before it can change the range or write file bytes. The displaced worker can claim other work.
- Keep existing response, validator and overlap checks for the new owner.

## 4.54 / browser 0.68.18

### Keep accepted HTTP ranges running when an extra connection is forbidden

- Split an owned range after its response and any required resume overlap pass validation.
- When retries are enabled, return a range refused with HTTP 403 to the remaining workers if one still has an accepted response. Retire the refused worker for this transfer.
- Preserve the terminal error path when there is no accepted peer or the refusal includes a usable retry instruction.

## 4.53 / browser 0.68.17

### Keep the original HTTP download when a helper fails

- Skip the initial range probe when the server advertises `Accept-Ranges: none`.
- Keep the original response when an optional range is refused, fails to connect, or returns a partial response that cannot be verified against the original.
- Apply transient server backoff to the next request while the accepted response continues.

## 4.52 / browser 0.68.16

### Refine the IDM-style browser controls

- Match the reference strip bevels and compact menu colors, spacing and selection highlight. Keep the format number visible and use more of each row for the title and format.
- Open the quality list below the strip when the list fits. Open upward when the lower space is insufficient and more room is available above.
- Use UDM Integration Module as the visible browser extension name.

## 4.51 / browser 0.68.15

### Bound HTTP reads at reassigned range boundaries

- Limit each ranged body read to the worker's current span, including the pipelined read started before writing the previous buffer.
- If the span shrinks during an outstanding read, apply the live boundary to the write. Fully consumed responses still get an end-of-response check.

## 4.50 / browser 0.68.14

### Start and reassign HTTP ranges promptly

- Use the retained full response immediately when the file has one planned connection and the server advertises byte ranges with a validator. Check any later resume response before appending.
- Wake waiting workers when the midpoint probe finishes, a connection releases its range, or the transfer finishes. Keep timed checks for cancellation and eligibility based on elapsed time.

## 4.49 / browser 0.68.13

### Collect selected links from the frame that opened the menu

- Scope the selected-link menu to the document where you right-clicked. Reject a frame URL that changed before collection.
- Use the same source tab and frame checks for desktop review and the browser fallback.

## 4.48 / browser 0.68.12

### Keep one Chromium proxy configuration per browser batch

- Resolve unobserved links against one settings snapshot while keeping each route bound to its own URL.
- Stop native batch review if Chromium reports a proxy change during preparation or before dispatch. The user can collect the links again with the new configuration.

## 4.47 / browser 0.68.11

### Share cookie-store checks across browser batch review

- Read the tab and its cookie-store mapping once for signed-in batch preparation. Select cookies per file URL and verify the source and cookie permission before native dispatch.
- Route form requests to browser review before staging a body. Stop review if the captured session's source changes during preparation.

## 4.46 / browser 0.68.10

### Remove request encryption work from batch review startup

- Pass temporary request context to the preview workers in memory. Check headers, browser proxy settings and the website session in those workers before reading file details.
- Avoid encrypting and decrypting every preview on the dialog startup path. Saved downloads still use the existing protected storage path.

## 4.45 / browser 0.68.9

### Keep batch file details when one link changes

- Refresh only the edited browser row when its address or request credentials change. Keep the sizes, types, and active lookups for the other links.
- Cancel the superseded lookup and discard its pending result. Give the replacement priority within the existing four-worker limit.

## 4.44 / browser 0.68.8

### Use the File Properties layout for browser batch edits

- Show the file details, one Save To path, the original page link, and request fields in the compact File Properties layout.
- Preserve category and server defaults when their parts of the destination are left alone. Save an explicitly cleared description as blank.
- Keep browser credentials scoped to their original address. An address change to another origin drops untouched per-file overrides.

## 4.43 / browser 0.68.7

### Resize and maximize browser batch review

- Add the resizable border and minimize and maximize controls from the IDM browser batch dialog.
- Expand the file list with the window. Keep Save To at the bottom left and the filters and confirmation buttons at the bottom right.
- Keep the reference dialog dimensions as the minimum tracking size. Scale the resized window and its controls when moving between monitors.

## 4.42 / browser 0.68.6

### Match the browser batch review layout and queue workflow

- Use IDM's dialog-unit geometry for the browser batch list, Save To group, filters and buttons.
- On OK, add the checked links to the primary download queue without opening a second dialog. The Tasks batch workflow retains the configurable queue picker.

## 4.41 / browser 0.68.5

### Apply browser batch filters to the files queued

- Count and queue only checked rows visible in browser batch review. Showing a filtered row again restores its previous check state.
- Start with repeated links hidden. Disable the repeat filter when all addresses are unique. Recognize HTML content types regardless of case or MIME parameters.

## 4.40 / browser 0.68.4

### Position the selected-link strip at the end of the selection

- Show the strip after the mouse is released and anchor it near the selection endpoint for both drag directions. Hide it when that endpoint scrolls out of view.
- Match the full and compact strip heights from the IDM reference. Keep the one-click handoff to batch review for the selected HTTP links.

## 4.39 / browser 0.68.3

### Bring the browser video menu closer to IDM

- Keep the compact player strip to the download arrow and close button. Open its panel controls with a right-click or Shift+F10.
- Use the reference menu gutters and highlight colors for the expanded and compact format lists. Keep the menu within the viewport when space is limited.

## 4.38 / browser 0.68.2

### Keep hidden desktop history out of the browser handoff

- Defer history, category tree, sorting and row painting while the main window is hidden or minimized. Refresh as soon as the window is restored.
- Keep browser confirmations, clipboard monitoring, transfer events and download alerts active in the background.
- During modal confirmation, update file sizes only in the visible rows and release the catalog lock before painting. Handle durable browser offers without a full history scan for legacy confirmations.

## 4.37 / browser 0.68.1

### Cancel playlist discovery when the video menu closes

- Stop pending HLS and DASH checks when the user closes the menu or changes players. A superseded lookup releases its pending requests.
- Start a fresh lookup immediately on reopen. Cancel messages match the tab, frame, document and specific lookup.
- Preserve verified selections so choosing an early format still opens the desktop download flow.

## 4.36 / browser 0.68.0

### Show verified streaming qualities while other playlists load

- Publish confirmed HLS variants and DASH qualities during discovery. Keep the existing request and byte limits for the remaining playlists.
- Keep the same download keys when early choices remain available, and reject publication from a superseded lookup.
- Keep keyboard focus, pressed pointer selections and open audio options during menu updates.

## 4.35 / browser 0.67.99

### Keep confirmed video qualities when a playlist stalls

- Check independent HLS and DASH sources in parallel, with four shared read slots and one nine-second discovery deadline.
- Keep completed qualities available when another playlist or variant misses the deadline. Tell the user that additional formats may be available on refresh.
- Enforce the shared HLS request and byte limits while reading. Validate the player again before showing any offers.

## 4.34 / browser 0.67.98

### Show direct video files while stream formats load

- Make verified direct files selectable while the same player's HLS or DASH formats are being discovered.
- Preserve direct selections across the catalog update. Keep focus on the selected format and wait for a pressed pointer to release before replacing menu rows.
- Keep the direct choices usable after playlist lookup fails. Recheck the player, source list and settings before handing off a selection.

## 4.33 / browser 0.67.97

### Reduce the wait for formats on non-YouTube players

- Read tab state and capture settings concurrently, then ask the browser to inspect the clicked player immediately.
- Skip unrelated elements for direct players. Keep the visible-player ambiguity check for blob playback and Dailymotion.
- Reuse one capture snapshot when associating a playlist and checking its format details. Recheck the player and its document before publishing formats.

## 4.32 / browser 0.67.96

### Match the IDM browser panel and toolbar appearance

- Align the video strip to the player corner with space above the video when available.
- Give the compact strip a right-aligned, gray format menu with the player title, numbered entries and visible format details when titles are truncated.
- Use an original globe-and-arrow toolbar icon with a crossed-out disabled state. The toolbar still toggles integration for the active tab.

## 4.31 / browser 0.67.95

### Reduce browser work before the Windows batch opens

- Read tab, cookie policy, and desktop capability once when preparing a batch. Each link still retains its own request context, proxy route, and session verification.
- Recheck the source documents, cookie store, and permission before sending the batch to Windows.

## 4.30 / browser 0.67.94

### Reuse browser response details in batch review

- Fill in the file size, type, and filename from a matching, unambiguous browser response when all required details are available.
- Use the desktop metadata lookup for missing or changed request details. The transfer still verifies its own response.

## 4.29 / browser 0.67.93

### Match browser batch file details and apply the previewed filename

- Show Windows file icons and type names from the background Shell worker. Format known sizes in bytes, KB, MB, or larger units while keeping numeric sorting.
- Use the displayed filename to preview the category folder and to add the file. Server-suggested names carry through confirmation, and per-file name edits take precedence.

## 4.28 / browser 0.67.92

### Match the browser batch destination controls

- Use a Save To group with three radio buttons. Choose each file's category folder, one category for all files, or one directory for all files.
- Show more rows in the batch list and arrange the selection buttons and filters beside the destination group. The displayed save paths follow the active destination choice.

## 4.27 / browser 0.67.91

### Review right-click link batches in Windows

- Both right-click batch commands open the Windows review when the desktop supports this workflow. UDM checks the contributing browser documents before opening it.
- The browser batch list follows IDM's column order, shows the planned save path for each file, and hides images found on the page by default.

## 4.26 / browser 0.67.90

### Open selected browser links in the Windows batch dialog

- The **Download with UDM** strip opens the native batch review over the browser. Links start unchecked and no download is added before confirmation.
- The dialog shows link text and checks file details using the browser request context. Older desktop versions and batches that require the browser review still use the extension tab.

## 4.25 / browser 0.67.89

### Bring the browser download strips closer to IDM

- Selected links show a classic **Download with UDM** strip. One click opens the existing batch review with the selected links checked.
- Compact video and selected-link strips keep the close button visible. Compact video menus align with the right edge of the strip and use denser format rows.

## 4.24 / browser 0.67.88

### Start the desktop handshake as the video menu opens

- Prepare the native connection after the browser finds available formats, so the handshake can run while you choose a quality. Selecting a format remains the step that submits the download.
- Browser Tools uses the original video tab and can open desktop settings, restore panels, and recover interrupted downloads from its own tab.

## 4.23 / browser 0.67.87

### Close the quality menu when handing a video to UDM

- Selecting a single video or audio format dismisses the menu immediately. Download File Info handles ordinary successful handoffs.
- A failed or uncertain handoff reports the outcome without retrying it or overriding a later dismissal. Bulk selections retain their progress and partial-failure counts.

## 4.22 / browser 0.67.86

### Match the IDM browser panel and toolbar interaction

- Use a slim player strip with a green play glyph and a compact, numbered format menu. Open secondary panel controls with the question-mark button.
- Click the toolbar icon to toggle UDM on the current tab. Right-click and choose UDM browser tools for settings, Site Grabber, link discovery and recovery.

## 4.21 / browser 0.67.85

### Refresh a failed video session from Download File Info

- Recover a matching browser capture from the open File Info dialog after background prefetch fails. Save the session, then let UDM retry prefetch on the same download while retaining the selected destination.
- The refresh dialog opened from File Info only saves the session. Start Download, Download Later and Cancel remain available in File Info.

## 4.20 / browser 0.67.84

### Show progress while saving a prepared video

- After Start, show the verified file size and publication status through the usual progress window while saving a prepared file.
- Honor the keep-open setting even when publication finishes before the window appears. The retained window shows the measured prefetch peak.

## 4.19 / browser 0.67.83

### Capture prefetch speed between progress updates

- Measure the prefetch peak with a clock and byte counter for each download. Take a final sample at Start or when the transfer phase ends, so a short prefetch can register even before the next desktop tick.
- Keep the prefetch peak separate from the current transfer rate and ETA.

## 4.18 / browser 0.67.82

### Start background downloads when a slot frees up

- Retry eligible direct-file prefetch while Download File Info is open if all transfer slots were busy. A metadata lookup can show the size while the file waits.
- Suspend retries while login edits or a nested dialog are active. Start and Download Later remain available while waiting.

## 4.17 / browser 0.67.81

### Explain saved bytes in the progress details

- Show a separate saved-data row when a completed or verified prefetched file has no measured connection traffic.
- Keep the original number and byte count for connections that transferred data. Hide unused rows after completion.

## 4.16 / browser 0.67.80

### Show the measured prefetch peak after Start

- Record the highest sampled transfer rate while File Info prefetch is active and save it with the bytes already received at Start.
- Display that peak separately from the current transfer rate. Keep the limiter and estimated time based on the live rate.

## 4.15 / browser 0.67.79

### Keep a verified video when closing Download File Info

- Closing File Info saves an already verified media assembly with the paused download. Resume can verify and reuse it.
- Save the assembly record before stopping the worker. A failed save leaves the dialog available to retry. Explicit Cancel on a fresh offer still discards its temporary output.

## 4.14 / browser 0.67.78

### Show the full byte count in compact Download File Info

- Keep a readable size beside the file icon and show the full byte count in a visible row above the dialog buttons.
- Distinguish verified final output, calculated output, retained output, selected tracks and browser-reported sizes. Keep preparation and capacity messages visible when the byte count is pending.

## 4.13 / browser 0.67.77

### Configure new categories from Download File Info

- Open the shared category editor from the plus button. Set a folder, file types, site restrictions and the remember-path preference while creating the category.
- Select the new category after saving and update the proposed destination through the existing category selection workflow.

## 4.12 / browser 0.67.76

### Recover the saved YouTube streaming format from the browser

- Opening the video from Refresh media session identifies the paused download's streaming format to the extension. The recovery page offers that format even when the normal panel prefers a direct link at the same resolution.
- Route the replacement to the open recovery dialog and check the original track identities before saving. An expired recovery page cannot create a separate download.

## 4.11 / browser 0.67.75

### Show progress while checking saved YouTube streams

- Use the existing saved-data verification display while hashing retained YouTube segments or completed tracks.
- Check for Pause between disk reads. An interrupted segment check preserves its prior cache index for the next Resume.

## 4.10 / browser 0.67.74

### Reclaim YouTube streaming files after Cancel

- After cancellation is saved, remove known streaming segments, cache receipts and internal track files from the canceled offer's temporary folder.
- Preserve shared folders, redirected paths and unrelated files.

## 4.9 / browser 0.67.73

### Keep a prepared YouTube file after refreshing its session

- Preserve the assembled file's receipt when the approved replacement session matches the saved stream selection and the receipt belongs to the current download.
- Save the refreshed session and updated receipt together. Resume verifies the existing output before publication.

## 4.8 / browser 0.67.72

### Rebuild fully saved YouTube streams after the session expires

- Resume can rebuild video and audio tracks from a complete verified segment cache after an interruption during assembly.
- Check playback session freshness and establish the network connection only when a track still needs media bytes. A missing or damaged segment still requires a fresh session.

## 4.7 / browser 0.67.71

### Keep the YouTube playback session valid through handoff

- Apply the same document, visit and expiry checks to captured sessions for video and audio.
- Recheck the prepared playback session before native dispatch, including reconnects. Ask for refreshed playback if the session expires while UDM connects.

## 4.6 / browser 0.67.70

### Keep the selected YouTube transport current through connector startup

- Recheck direct video links and streaming audio/video revisions and duration immediately before native dispatch, including reconnects.
- Revalidate native-retrieved pairs for freshness and the chosen quality; apply the current panel size setting to the prepared transfer.
- Reject an outdated selection before creating a desktop download.

## 4.5 / browser 0.67.69

### Account for earlier GUI improvements that reused version numbers

- Restore 39 separately documented functional deliveries to the cumulative count, including category editing, queue membership, menus, columns, DPI scaling and recovery controls.
- Update historical accounting labels and preserve recorded artifact versions. Exclude verification-only continuations and the previously counted Properties build.

## 3.65 / browser 0.67.68 (recorded 0.67.29)

### Check the selected video after connecting to UDM

- Run the final video, page, and permission checks after the native connection is ready. Recheck if the connection drops before the request can be sent.
- Keep the preparation deadline active through validation. A changed selection or expired request sends nothing to the desktop.

## 3.64 / browser 0.67.67 (recorded 0.67.28)

### Prevent late video submission during native connection waits

- Check the selection deadline at the actual native send, including after a reconnect. An expired selection leaves the browser without submitting a download.
- Start the acknowledgement timer at dispatch. Discard a draft recovery receipt if dispatch never occurs. Connection startup still overlaps video preparation.

## 3.63 / browser 0.67.66 (recorded 0.67.27)

### Share the desktop capability read for direct YouTube audio

- Reuse a fresh desktop reply for both request preparation and handoff receipts. Concurrent consumers share a refresh when the earlier read expires.
- Keep browser proxy and session validation and the final selected-audio check before submission.

## 3.62 / browser 0.67.65 (recorded 0.67.26)

### Read desktop capabilities during selected media preparation

- After validating the selected YouTube format or adaptive site offer, read desktop capabilities while preparing the streams. A cold desktop can start in the background during that wait.
- Scope the result to the selection and refresh it if it is stale. A download still requires its receipt and final page and permission checks.

## 3.61 / browser 0.67.64 (recorded 0.67.25)

### Reuse host capabilities on the negotiated connection

- Reuse the native hello reply for capability checks on the same connection, including YouTube audio, HLS and DASH handoffs.
- Clear cached capabilities when the port closes. Legacy hosts and desktop settings retain their existing request paths.

## 3.60 / browser 0.67.63 (recorded 0.67.24)

### Connect to the native host while preparing a selected video

- Overlap native protocol negotiation with YouTube quality preparation and supported site playlist or direct media preparation.
- Reuse the negotiated connection for the eventual handoff. The handshake creates no download, and the final selection and receipt checks still gate submission.

## 3.59 / browser 0.67.62 (recorded 0.67.23)

### Open video quality choices without waiting for Site Grabber

- Load YouTube qualities as soon as the popup identifies the active video. Site Grabber and integration settings load independently.
- Use a dedicated status line for popup video actions. The popup and the page panel only show a preparation message if the handoff is still pending after a short delay. Keep quality selection locked during handoff.

## 3.58 / browser 0.67.61 (recorded 0.67.22)

### Reclaim canceled HLS and DASH segment caches

- After a fresh recorded HLS or DASH offer is canceled, remove the known segment files and recovery receipts from its verified temporary folder.
- Leave shared folders, redirected paths, live recording data and other assembly files in place.

## 3.57 / browser 0.67.60 (recorded 0.67.21)

### Reclaim canceled browser prefetch ranges

- After saving the cancellation of a fresh browser offer, remove its known HTTP range files, including ranges for separate audio and video tracks.
- Preserve shared or redirected folders and files that cannot be safely opened. Closing File Info still preserves the paused download.

## 3.56 / browser 0.67.59 (recorded 0.67.20)

### Reject a fresh browser offer from File Info

- Cancel removes a fresh unconfirmed browser offer from the download list after its prefetch stops. Closing the window keeps the paused record available.
- Existing downloads and original browser responses use the existing pause behavior. A failed removal keeps the dialog available for retry.

## 3.55 / browser 0.67.58 (recorded 0.67.19)

### Add downloads to a running queue without restarting it

- When Start queue processing is selected in Download Later or batch import, add the selected eligible downloads to the current run. Other queue members keep their retry counts, and the schedule and completion cycle keep running.
- If the queue is idle, start it normally. A deferred prefetch still releases its worker slot before the queue resumes the file.

## 3.54 / browser 0.67.57 (recorded 0.67.18)

### Prevent repeated desktop Start queue commands

- Dim Start for a running queue in the Downloads, tray and toolbar menus. Recheck the queue state when the command runs, including from the large-catalog chooser.
- Use Scheduler's existing run-state check for the primary toolbar Start command. Repeated clicks leave the current queue cycle and retry counts intact.

## 3.53 / browser 0.67.56 (recorded 0.67.17)

### Save the selected batch and its queue together

- Commit the selected downloads, any new queue and the batch prompt preference together. A failed admission leaves the selection available for retry.
- Keep the existing duplicate policy and retain old partial files until the batch commits. Report optional queue-start failures separately from the saved batch.

## 3.52 / browser 0.67.55 (recorded 0.67.16)

### Create the selected queue with Download Later

- Save a queue created in the Download Later chooser together with its first download and the chooser preference. Invalid file details or a failed catalog save leave the queue list intact.
- Optional queue processing starts after the download and its queue are saved.

## 3.51 / browser 0.67.54 (recorded 0.67.15)

### Save the Download Later queue prompt preference with the download

- Match the reference queue chooser's “Don't ask me again” caption. Save the single-download preference with Download Later whether prefetch is running or stopped.
- If the optional queue start fails after the download is saved, report that it was added and close File Info after the message.

## 3.50 / browser 0.67.53 (recorded 0.67.14)

### Match File Info category names and destination choices

- Show General and Compressed in Download File Info and name the selected category in the remember-folder checkbox. Keep saved category identifiers and custom names distinct.
- Use the same category folder rules as Options and new download admission when the user changes the category. The destination now follows the configured or default folder for that category.

## 3.49 / browser 0.67.52 (recorded 0.67.13)

### Queue interrupted browser responses with the edited request

- Verify the saved response and prepare its retained bytes for range continuation before committing Start. File Info saves the new destination, login and folder preferences with the queued continuation.
- Resume uses the same admission. On failure the captured body and receipt stay recoverable, and a verified range seed can be reused on retry.

## 3.48 / browser 0.67.51 (recorded 0.67.12)

### Save the Start decision for complete browser responses

- When File Info starts saving a complete response already received from the browser, save the edited destination and preferences with the worker admission. A failed catalog save leaves the previous record and pause state available for retry.
- Publication still checks the saved response and its receipt before writing the final file. Interrupted responses continue through range recovery.

## 3.47 / browser 0.67.50 (recorded 0.67.11)

### Save Start Download with the edited request

- After prefetch stops, save the edited request, remembered preferences and individual start decision together. A failed queue admission or catalog write leaves the dialog available for retry.
- Original browser responses retain their dedicated publication or continuation workflow.

## 3.46 / browser 0.67.49 (recorded 0.67.10)

### Save File Info edits together when prefetch has stopped

- Save the destination, description, folder preferences and per-file or remembered login together. If validation or catalog storage fails, the dialog keeps the edits available to retry.
- Download Later saves the selected queue and pause decision with those edits. Start schedules the transfer after the edited request is saved.

## 3.45 / browser 0.67.48 (recorded 0.67.9)

### Count older builds that reused version numbers

- Recover 42 additional increments from delivered 0.79–0.82 installers and independently built 0.84 desktop and browser candidates. Carry those increments into all later historical browser numbers and into the current desktop and browser versions.
- Give the recovered 0.84 builds their own desktop numbers in the historical ledger. Keep the original numbers with the source tags, receipts and artifacts.

## 3.2 / browser 0.67.47 (recorded 0.66.66)

### Remember an unchanged login without restarting the prefetch

- Save the site login with the Start or Download Later decision when it matches the current request. Start continues the same worker, and Download Later keeps its saved data.
- If the login differs, apply it through the existing request restart path. A failed confirmation save restores the previous site settings.

## 3.1 / browser 0.67.46 (recorded 0.66.65)

### Refresh File Info for edited login details

- Hide the old size and server filename when the login fields change. Cancel any details lookup started with the previous values.
- Refresh details can check the edited login without changing the saved request or writing its session cookies into the download record. Saving a different login clears the old browser size and filename.

## 3.0 / browser 0.67.45 (recorded 0.66.64)

### Show the browser-observed server filename in Download File Info

- Offer the filename from a matching browser response while the desktop waits for its own response or metadata check. Use server name applies it only when selected.
- Prefer the desktop's own response once available. Clear the browser filename when the address or request headers change.

## 2.99 / browser 0.67.44 (recorded 0.66.63)

### Show the reported size for ordinary browser downloads

- Use a matching browser GET response to show the reported byte count when File Info opens. For a valid partial response, use the total from Content-Range.
- Skip conflicting lengths, encoded responses and responses that no longer match the request being handed off. The native response still determines the transfer size.

## 2.98 / browser 0.67.43 (recorded 0.66.62)

### Show the browser-reported size of direct YouTube M4A downloads

- Show the reported byte count as soon as File Info opens when the selected direct audio track provides one. If the signed link and track metadata disagree, wait for the server response.
- Replace the reported size with the response size when available. An address or request-header edit clears the hint.

## 2.97 / browser 0.67.42 (recorded 0.66.61)

### Recover interrupted direct YouTube M4A handoffs

- Give direct YouTube M4A downloads a durable receipt while preserving their ordinary file-transfer and duplicate handling. Check the current audio URL and cookie permissions immediately before sending.
- If the desktop reply is lost, check the original selection's receipt before sending another request. Older desktop versions still accept direct audio through the ordinary handoff.

## 2.96 / browser 0.67.41 (recorded 0.66.60)

### Recover an earlier YouTube handoff before preparing playback

- Check the saved receipt for the selected quality or audio track before reading playback URLs. An accepted handoff can be recovered even if its old playback URL has expired.
- Show the recovered result in the extension popup and acknowledge it after displaying the result.

## 2.95 / browser 0.67.40 (recorded 0.66.59)

### Recover interrupted YouTube handoffs

- Record the selected YouTube video or captured streaming audio with a durable media receipt. If the reply is lost, check whether UDM already accepted the selection before trying again.
- Confirm the current video visit and selection after preparing the receipt. Direct M4A links retain the ordinary file handoff.

## 2.94 / browser 0.67.39 (recorded 0.66.58)

### Recheck a YouTube quality as soon as its stream becomes ready

- While a selection is waiting for playback capture, the panel can wake the next browser check as soon as a verified stream becomes available. The 400 ms polling fallback still covers missed notifications.
- Confirm the selected video, visit and track before submitting. A readiness notification only requests another check.

## 2.93 / browser 0.67.38 (recorded 0.66.57)

### Read ready YouTube streaming sessions with the selected formats

- Read a ready streaming session in the same browser call as the format metadata when a video or audio track is selected. Keep the separate lookup for sessions that arrive later.
- Check that the selection still belongs to the same visit before handoff. Native player results cached for an earlier visit are excluded even if the video ID matches.

## 2.92 / browser 0.67.37 (recorded 0.66.56)

### Keep YouTube stream verification tied to the current visit

- Clear verified streaming evidence when YouTube starts a new navigation or the page video changes. A response that arrives after its visit ended cannot make the panel ready, even after returning to the same video.
- Keep the early handoff path for a stream whose response verifies the current video's identity while an ad is still marked as playing.

## 2.91 / browser 0.67.36 (recorded 0.66.55)

### Show progress while checking a saved HTTP prefetch

- Show the saved-data check separately from download completion when resuming a fully prefetched file. Report progress for both consolidated files and older numbered parts.
- Use the same progress display for FTP replacement checks. Keep Pause available and suppress the transfer rate and ETA during the check.

## 2.90 / browser 0.67.35 (recorded 0.66.54)

### Show progress while checking a replacement FTP address

- Display the percentage of retained bytes verified in the progress dialog. Keep the saved download percentage separate and preserve the Pause action.
- Count verification traffic for quota and diagnostics. Hide the download rate and ETA during the check and start the rate meter from the updated byte count when transfer resumes.

## 2.89 / browser 0.67.34 (recorded 0.66.53)

### Apply browser routing and cookies to the correct replacement address

- Replace a captured proxy route when the browser supplies one for the new address. If the previous route was tied to a different URL, prompt for a new browser capture before using a browser route again.
- Discard the saved browser session on cross-origin address changes. Keep manually entered FTP replacement links separate from browser handoff and review.

## 2.88 / browser 0.67.33 (recorded 0.66.52)

### Refresh a paused FTP download address

- Accept a replacement FTP link in Refresh download address. For a different source, check the server's size and modification time, then compare every retained byte before binding the partial download to the new link.
- Preserve saved parts on mismatch or interruption. Keep strict FTP validation through the final metadata and range boundary checks.

## 2.87 / browser 0.67.32 (recorded 0.66.51)

### Preserve partial bytes while validating a replacement HTTP link

- Keep refresh validation pending through the range requests and their overlap checks. Pause, cancellation and failed requests keep that protection for the next attempt.
- For a refreshed link with saved bytes, check the worker responses against the same validator used by the probe. If a worker sees a changed file or the server stops honoring ranges, keep the partial download available for another link.

## 2.86 / browser 0.67.31 (recorded 0.66.50)

### Repair every damaged range in a saved prefetch

- Check the entire saved plan on Resume. Keep verified ranges and reset every missing, shortened or changed range before fetching again.
- Save the repaired offsets before retrying so an interruption cannot restore the old completed offsets. Remove damaged numbered parts and prevent leftover numbered parts from replacing bytes already verified in the consolidated file.

## 2.85 / browser 0.67.30 (recorded 0.66.49)

### Reuse the verified checksum of a fully prefetched file

- On Resume, use the checksum calculated while verifying a saved single-range receipt for final publication when the file identity and size still match.
- Keep the existing hashing path for files with multiple ranges and for output that is changed during assembly.

## 2.84 / browser 0.67.29 (recorded 0.66.48)

### Separate direct-download work from time spent waiting for Start

- End network timing before creating the saved prefetch receipt. Record receipt work and confirmation wait separately in the finish diagnostics.
- Exclude confirmation wait from active elapsed time, including when the download is deferred or canceled, and from the total time spent finishing the file.

## 2.83 / browser 0.67.28 (recorded 0.66.47)

### Publish sooner when Start arrives during direct-download prefetch verification

- Stop calculating the optional saved prefetch receipt after Start is committed. Continue with normal publication and reuse any running hash progress.
- Keep independent verification for resumed receipts and preserve cancellation and required checksum checks.

## 2.82 / browser 0.67.27 (recorded 0.66.46)

- Account for four additional build increments. Restore the native 0.28.0 offline-site/SOCKS5 build and the 0.75.0 automation candidate to the paired ledger. Split the durable Pause build and the later context-menu rebuild out of the old 2.05 heading.
- Carry native 1.100.0 through 1.105.0 into 2.00 through 2.05. The first build recorded as 2.05 becomes 2.06, and the later 2.05 context-menu build takes the gap after the dark-menu build in chronological order. The previous development build, recorded as 2.80 / 0.66.41, accounts as 2.81 / 0.66.45.
- List previously implicit browser-only milestones within their existing numeric jumps. Archived tags, packages and build receipts retain their recorded numbers. [Counting policy](docs/versioning.md)

## 2.81 / browser 0.67.26 (recorded 0.66.41) (recorded desktop 2.80)

### Reuse the running hash for a single-range prefetch
- Finish the hash already trailing a single-range HTTP transfer when creating its completed prefetch receipt. Publication reuses the finished digest.
- Resume continues to verify saved prefetch bytes against their receipt before reusing them.

## 2.80 / browser 0.67.25 (recorded 0.66.40) (recorded desktop 2.79)

### Reuse empty HTTP prefetches after Download Later
- Record an empty response with the same range plan whether its length was declared or measured at EOF.
- Require the saved zero-byte file before reusing an empty prefetch receipt.

## 2.79 / browser 0.67.24 (recorded 0.66.39) (recorded desktop 2.78)

### Show the measured size of a prefetched response with no length header
- Record the received byte count as the response size after a successful end of stream. File Info can display that count before Start, and a completed nonempty prefetch can use the existing verified receipt.

## 2.78 / browser 0.67.23 (recorded 0.66.38) (recorded desktop 2.77)

### Reuse the source inspection for a preserved MP4
- Reuse the container and track metadata read during MP4 selection. Independently inspect the staged output and verify its hash before reporting the exact size.

## 2.77 / browser 0.67.22 (recorded 0.66.37) (recorded desktop 2.76)

### Stage verified combined MP4s with less temporary storage
- Use a hard link for eligible captures when the filesystem allows it, with a verified copy as the fallback.
- Unlink a leftover staging name before preparing a replacement so a retry cannot overwrite the source through that name.

## 2.76 / browser 0.67.21 (recorded 0.66.36) (recorded desktop 2.75)

### Preserve eligible combined YouTube MP4 files
- Stage a supported MP4 with one video track and one audio track byte for byte. Verify its saved hash and selected tracks before showing its exact size or publishing it.
- Keep the media assembly path for separate tracks and other containers.

## 2.75 / browser 0.67.20 (recorded 0.66.35) (recorded desktop 2.74)

### Preserve browser request context for direct YouTube audio
- Prepare direct M4A downloads through the existing browser handoff path, including the proxy route and permitted request credentials.
- Recheck the selected track and its direct URL after preparing that context and before submitting it to UDM.

## 2.74 / browser 0.67.19 (recorded 0.66.34) (recorded desktop 2.73)

### Bound YouTube video and audio preparation
- Apply a 20-second preparation deadline before submitting a YouTube download. Late player or stream data cannot submit a download after that deadline.
- Keep a separate 40-second acknowledgement window after submission.

## 2.73 / browser 0.67.18 (recorded 0.66.33) (recorded desktop 2.72)

### Recheck direct video selections after preparing browser credentials
- Apply the same final selection check to direct video and adaptive downloads. Abort before submission if the video, panel settings or required cookie permissions changed.

## 2.72 / browser 0.67.17 (recorded 0.66.32) (recorded desktop 2.71)

### Recheck the video immediately before adaptive handoff
- Validate the selected video, panel settings and cookie permission after credential collection and receipt preparation. A changed selection stops before the download request is sent.

## 2.71 / browser 0.67.16 (recorded 0.66.31) (recorded desktop 2.70)

### Reuse the desktop capability check during live DASH handoff
- Skip the redundant desktop capability request when the same selection already checked live DASH Period support. Still revalidate the active Period before submitting the download.

## 2.70 / browser 0.67.15 (recorded 0.66.30) (recorded desktop 2.69)

- Restore 89 previously uncounted increments using paired changelog entries, archived tag manifests, and development records. That audit mapped the 2.68 build to 0.66.29 and used 0.66.30 for this development build.
- Keep recorded version jumps and assign one increment to each documented update that reused a browser number. Count earlier reconciliation builds once each. Unversioned changes do not add increments without evidence of a separate build.

## 2.69 / browser 0.67.14 (recorded 0.65.40) (recorded desktop 2.68)

### Fetch HLS subtitles alongside the selected streams
- Fetch a selected subtitle playlist concurrently with the video and audio playlists. Validate it against the resulting live or recorded selection before handing the download to UDM.

## 2.68 / browser 0.67.13 (recorded 0.65.39) (recorded desktop 2.67)

### Prepare HLS video and audio together
- Fetch the selected video and separate audio playlists in parallel before handing the download to the desktop.
- Preserve track order, the capture deadline, and the audio-only path that skips the video playlist.

## 2.67 / browser 0.67.12 (recorded 0.65.38) (recorded desktop 2.66)

- Preserve all 236 increments already represented by 0.63.236. Under the two-digit rule that version maps to 0.65.36. Count the 2.65 rollover build and this correction as two further increments.

## 2.66 / browser 0.67.11 (recorded 0.64.0) (recorded desktop 2.65)

- Roll both browser manifests forward to 0.64.0 and use at most two digits per version component for future desktop and browser increments.

## 2.65 / browser 0.67.10 (recorded 0.63.236) (recorded desktop 2.64)

### Recover a browser connection before sending a download
- Reconnect once if the native connection closes before the download request is sent.
- Send the request through the port that completed the handshake. A disconnect after submission preserves the existing duplicate-prevention behavior.

## 2.64 / browser 0.67.9 (recorded 0.63.235) (recorded desktop 2.63)

### Keep automatic capture prompts accessible
- Give browser duplicate and capture-exception prompts the same foreground request and taskbar entry as File Info.

## 2.63 / browser 0.67.8 (recorded 0.63.234) (recorded desktop 2.62)

### Keep browser-triggered address and media refresh dialogs separate
- Open replacement-link confirmations with the same hidden owner and taskbar behavior as browser File Info.
- Keep the desktop-initiated refresh workflow available from the download list.

## 2.62 / browser 0.67.7 (recorded 0.63.233) (recorded desktop 2.61)

### Prioritize pending browser confirmations
- Present saved browser captures before refreshing or sorting the download list.
- Schedule another capture pass when a browser notification arrives while an earlier dialog is open.

## 2.61 / browser 0.67.6 (recorded 0.63.232) (recorded desktop 2.60)

### Start media prefetch when a transfer slot frees up
- If all transfer slots are occupied, keep checking for capacity while File Info is open and prefetch as soon as a slot is available.
- Show Waiting or Preparing when the media size is still unknown. Keep Start Download available throughout.

## 2.60 / browser 0.67.5 (recorded 0.63.231) (recorded desktop 2.59)

### Label media track sizes while the final file is being prepared
- Show known input track totals as Selected track size on the first paint and during prefetch.
- Use the browser-reported track size until the individual lengths are known. A verified or calculated output size takes priority when available.

## 2.59 / browser 0.67.4 (recorded 0.63.230) (recorded desktop 2.58)

### Show exact byte counts for direct downloads in File Info
- Show a known direct response size in bytes on the first paint or when metadata arrives.
- Preserve a known size when a later metadata lookup returns no length. Sizes awaiting refresh validation keep the existing display.

## 2.58 / browser 0.67.3 (recorded 0.63.229) (recorded desktop 2.57)

### Show the current media progress when Start is clicked
- Update the received and transferred totals from the audio and video tracks when confirming prefetch, before opening progress.
- Keep the prefetched byte count consistent with the first progress update and continue showing the sampled transfer rate.

## 2.57 / browser 0.67.2 (recorded 0.63.228) (recorded desktop 2.56)

### Close Download File Info promptly after Download Later
- Save the destination and queue choice, then close File Info while the prefetch worker stops.
- Keep a queue start requested during shutdown pending until the prefetch worker releases the transfer slot.

## 2.56 / browser 0.67.1 (recorded 0.63.227) (recorded desktop 2.55)

### Settle cancelled prefetch after the worker exits
- Wait for the background worker's exit signal before clearing confirmation and the provisional final size. Paused media now records a matching paused phase.
- Keep the pending confirmation state available for retry if saving the cancellation fails. Verified partial data remains available to resume.

## 2.55 / browser 0.67.0 (recorded 0.63.226) (recorded desktop 2.54)

### Recover file details after background download failure
- Download File Info reports a failed background transfer while leaving Start Download available.
- If an eligible direct download still has no known size, the dialog tries a metadata lookup once. The lookup does not replace the size of a saved partial transfer.

## 2.54 / browser 0.66.99 (recorded 0.63.225) (recorded desktop 2.53)

### Repair an isolated FTP resume mismatch
- When the remote file metadata still matches and other ranges have saved bytes, retry the affected range from its beginning once.
- Keep the saved parts for manual redownload if repair is ineligible or the mismatch repeats.

## 2.53 / browser 0.66.98 (recorded 0.63.224) (recorded desktop 2.52)

### Check the boundaries between FTP ranges
- Compare up to 64 overlapping bytes before publishing a segmented FTP download. Keep the parts available if the overlap differs.

## 2.52 / browser 0.66.97 (recorded 0.63.223) (recorded desktop 2.51)

### Advance the desktop and browser versions together
- Bring the Chromium and Firefox manifests forward by the 138 development increments recorded since browser version 0.63.85. This is a paired version update.

## 2.51 / browser 0.66.96 (recorded 0.63.85) (recorded desktop 2.50)

### Repair a single mismatched HTTP resume range
- If the server still identifies the same file and other ranges have saved progress, retry the affected range from its beginning.
- Keep the single-connection fallback for mismatches that cannot be isolated or persist after repair.

## 2.50 / browser 0.66.95 (recorded 0.63.85) (recorded desktop 2.49)

### Check the boundaries between HTTP ranges
- New nonzero ranges request up to 64 bytes of overlap. Save the overlap hash with the range and compare it against the adjoining bytes before publishing.
- Apply the existing single-connection recovery when an ordinary download has inconsistent boundaries.

## 2.49 / browser 0.66.94 (recorded 0.63.85) (recorded desktop 2.48)

### Verify HTTP resume bytes before appending
- Compare up to 64 bytes of overlap before writing more data to a resumed HTTP range.
- Restart ordinary transfers on one connection if the overlap differs. Preserve partials when a refreshed address must match the saved file.

## 2.48 / browser 0.66.93 (recorded 0.63.85) (recorded desktop 2.47)

### Verify FTP resume bytes before appending
- Request up to 64 bytes before a saved FTP offset and compare them with the retained part.
- Preserve the partial files and direct the user to Redownload if the overlap differs.

## 2.47 / browser 0.66.92 (recorded 0.63.85) (recorded desktop 2.46)

### Recover from an unreachable FTP passive port
- Try PASV if the EPSV data connection fails and the route can use PASV.
- Keep the fallback on the existing FTP route and perform it before requesting file data.

## 2.46 / browser 0.66.91 (recorded 0.63.85) (recorded desktop 2.45)

### Assemble FTP downloads from their own partial files
- Use the numbered FTP parts for content checks and publication even if a previous HTTP partial file exists in the same folder.

## 2.45 / browser 0.66.90 (recorded 0.63.85) (recorded desktop 2.44)

### Save HTTP range resets before removing obsolete data
- Preserve old partials if the zero-progress reset cannot be saved.
- Recover interrupted cleanup before any worker can adopt stale partials.

## 2.44 / browser 0.66.89 (recorded 0.63.85) (recorded desktop 2.43)

### Preserve old partial files until migration is saved
- Flush the consolidated partial file and save its resume offsets before deleting legacy parts.
- Complete legacy cleanup before workers start writing new data.

## 2.43 / browser 0.66.88 (recorded 0.63.85) (recorded desktop 2.42)

### Recover missing data in sparse HTTP partial files
- On resume, roll a saved range back to the first unallocated region within its recorded progress.
- Keep length-based recovery when the file system cannot report allocation.

## 2.42 / browser 0.66.87 (recorded 0.63.85) (recorded desktop 2.41)

### Checkpoint downloads independently of the desktop timer
- Run catalog checkpoints on an owned background timer so a stalled desktop thread does not stop resume-offset saves.
- Keep unchanged catalogs write-free and join the checkpoint worker before the final shutdown save.

## 2.41 / browser 0.66.86 (recorded 0.63.85) (recorded desktop 2.40)

### Finish HTTP responses cleanly for connection reuse
- Skip speculative reads beyond a worker's assigned range.
- Observe EOF for fully consumed responses before releasing them to the connection pool.

## 2.40 / browser 0.66.85 (recorded 0.63.85) (recorded desktop 2.39)

### Start downloading before an unadvertised range is verified
- On fresh HTTP transfers with a usable validator, write the full response while the midpoint helper checks range support, even if the server omits the range header.
- Require a verified range response before enabling resume or splitting in that case.

## 2.39 / browser 0.66.84 (recorded 0.63.85) (recorded desktop 2.38)

### Keep parallel HTTP available when the first helper arrives late
- If a validated midpoint response overlaps the primary worker's progress, discard its body and split the live remainder when it is still large enough.

## 2.38 / browser 0.66.83 (recorded 0.63.85) (recorded desktop 2.37)

### Keep the first HTTP response moving during helper startup
- For servers advertising byte ranges with a usable validator, read the accepted full response while the midpoint helper connects.
- Give the validated helper its range only if no committed or in-flight write overlaps it. If the full response finishes first, cancel the pending helper.

## 2.37 / browser 0.66.82 (recorded 0.63.85) (recorded desktop 2.36)

### Split fresh HTTP downloads as workers join
- Start with the accepted full response and a validated midpoint range. Additional workers obtain ranges by splitting the current largest eligible span.
- Reuse the validated helper response if another worker shortens its assigned range before it starts reading.

## 2.36 / browser 0.66.81 (recorded 0.63.85) (recorded desktop 2.35)

### Split the largest eligible HTTP range
- Rank candidates by bytes remaining and split the largest eligible span in half after excluding the current disk write.

## 2.35 / browser 0.66.80 (recorded 0.63.85) (recorded desktop 2.34)

### Prefer the next unstarted HTTP range
- When a worker finishes its range, give it the immediately following range if that range is unassigned and has received no bytes. This takes priority over splitting another worker's range.

## 2.34 / browser 0.66.79 (recorded 0.63.85) (recorded desktop 2.33)

### Honor the progress window start view
- Apply the selected start view to automatic browser-video progress windows and leave existing progress windows in their current state when a transfer resumes.
- Start queued progress in the system tray by default. Use a minimized taskbar window if Windows cannot register a tray icon.

## 2.33 / browser 0.66.78 (recorded 0.63.85) (recorded desktop 2.32)

### Match the Options on completion controls
- Disable modem, exit and power choices while the completion dialog is enabled.
- Clear those choices when showing the dialog again and lock completion controls once the download finishes.

## 2.32 / browser 0.66.77 (recorded 0.63.85) (recorded desktop 2.31)

### Match the progress Speed Limiter controls
- Use a right-aligned numeric rate field and disable its labels while the limiter is off.
- Skip catalog saves when the remembered rate is already set to the same value.

## 2.31 / browser 0.66.76 (recorded 0.63.85) (recorded desktop 2.30)

### Match the download connection table
- Show N., Downloaded, and Info columns with zero-based connection numbers.
- Use grouped whole-KB values for transfers of at least 1 KB and match the captured connecting and receiving captions.

## 2.30 / browser 0.66.75 (recorded 0.63.85) (recorded desktop 2.29)

### Match the download segment chart
- Show downloaded coverage in blue and connection starts in magenta over a white track, with Windows colors in high-contrast mode.
- Place HTTP and FTP markers at the latest accepted request offset, including offsets used on resume.

## 2.29 / browser 0.66.74 (recorded 0.63.85) (recorded desktop 2.28)

### Distinguish resuming from starting in the progress window
- Show Resume when a paused or failed download has saved progress and reports resume support.
- Show a disabled Pausing… action until the stop finishes.

## 2.28 / browser 0.66.73 (recorded 0.63.85) (recorded desktop 2.27)

### Reduce unnecessary progress-window redraws
- Update the overall progress bar and segment chart only when their values change.
- Update changed connection rows and skip the details panel while it is collapsed. Refresh the panel as soon as it expands.

## 2.27 / browser 0.66.72 (recorded 0.63.85) (recorded desktop 2.26)

### Restart parallel HTTP downloads safely
- Reuse one eligible full response when a parallel range request requires a restart. Stop the previous workers before writing from byte zero.
- Save the reset range map before recreating the partial file so recovery cannot use old byte offsets for the new file.

## 2.26 / browser 0.66.71 (recorded 0.63.85) (recorded desktop 2.25)

### Reuse a full response when a single-connection resume must restart
- If the server returns a full response to a resume request and restart is allowed, write that response from byte zero.
- Wait for the prefix hasher to release the old partial file before replacing it.

## 2.25 / browser 0.66.70 (recorded 0.63.85) (recorded desktop 2.24)

### Preserve active downloads during long server retry waits
- Let an accepted full response finish if its optional helper receives a retry wait longer than five minutes.
- Keep partial bytes and require review if completing the download would need a new request.

## 2.24 / browser 0.66.69 (recorded 0.63.85) (recorded desktop 2.23)

### Preserve resume after a temporary helper failure
- Keep the accepted response as one resumable range when the server advertises byte ranges and provides a usable validator.
- On reconnect, request the remaining bytes and verify the returned range and resource identity before appending.

## 2.23 / browser 0.66.68 (recorded 0.63.85) (recorded desktop 2.22)

### Keep the original download moving when a helper fails to open
- Continue reading the accepted full response when the optional range request encounters a recoverable connection error, retryable HTTP response, or HTTP 416.
- Honor a retryable helper response's delay if the full transfer later needs to open another request.

## 2.22 / browser 0.66.67 (recorded 0.63.85) (recorded desktop 2.21)

### Reuse the HTTP range verification response
- When a fresh download can use multiple initial ranges, verify range support with the first helper's actual byte range.
- Reuse the open response after checking its offsets, total size and validator. The worker reads it through the normal speed limit and resume path.

## 2.21 / browser 0.66.66 (recorded 0.63.85) (recorded desktop 2.20)

### Reduce the number of initial HTTP ranges
- Start fresh, resumable downloads with balanced ranges bounded by the configured connection count.
- Split slow ranges on demand. Resume existing downloads from their saved range maps.

## 2.20 / browser 0.66.65 (recorded 0.63.85) (recorded desktop 2.19)

### Show the permission warning for direct downloads
- Show an IDM-style warning titled with the file name when a confirmed individual download ends with HTTP 403.
- Keep the failed download in the list so it can be retried or given a refreshed address.

## 2.19 / browser 0.66.64 (recorded 0.63.85) (recorded desktop 2.18)

### Match the Download Complete dialog
- Put Open, Open with, Open folder, Close and the suppression checkbox before the read-only fields in keyboard order.
- Display the abbreviated size to two decimal places without rounding up, with the exact byte count alongside it.

## 2.18 / browser 0.66.63 (recorded 0.63.85) (recorded desktop 2.17)

### Match IDM's Batch Download dialog on open
- Prefill and select a supported clipboard URL in the address field.
- Leave the file previews empty for a plain URL and populate them when the address contains a sequence.
- Disable the authorization labels and fields together until Use authorization is selected.

## 2.17 / browser 0.66.62 (recorded 0.63.85) (recorded desktop 2.16)

### Match IDM's Add URL dialog
- Prefill and select a supported clipboard URL when opening Add URL, including when automatic clipboard monitoring is disabled.
- Disable the Login and Password labels and fields until Use authorization is selected.

## 2.16 / browser 0.66.61 (recorded 0.63.85) (recorded desktop 2.15)

### Match the floating drop target
- Size the window from the 59 × 70 pixel capture at 125% scaling and use a native caption and close button with UDM artwork.
- Accept the browser's native URL drag format and retain support for dropped text and URL lists.

## 2.15 / browser 0.66.60 (recorded 0.63.85) (recorded desktop 2.05, later build)

### Match the download context menu
- Follow the captured IDM command order, group UDM commands in More actions, and allow right-click selection across the download row. [Build receipt](docs/validation/native-2.05-current-build.json)

## 2.14 / browser 0.66.59 (recorded 0.63.85) (recorded desktop 2.13)

### Match the captured dark menus
- Use distinct reference shades for the menu strip, popup menus and their highlighted items.
- Scale popup rows, separators and label insets from the capture while retaining native menu navigation.

## 2.13 / browser 0.66.58 (recorded 0.63.85) (recorded desktop 2.12)

### Match the main toolbar's dark surface and default spacing
- Use the captured gray behind toolbar buttons in dark mode.
- Target the captured 71-pixel toolbar height at 125% scaling for the built-in large icons with captions. Allow the toolbar to grow when the font or native button geometry needs more space.

## 2.12 / browser 0.66.57 (recorded 0.63.85) (recorded desktop 2.11)

### Match the Categories pane's dark colors
- Use the captured colors for the Categories tree, heading, close button and frame. Match the gray surface around the download list.
- Keep Windows light and high-contrast colors available through the existing theme controls.

## 2.11 / browser 0.66.56 (recorded 0.63.85) (recorded desktop 2.10)

### Match the main download list's dark palette
- Apply the captured IDM shades to the dark list body, grid, header and column dividers. Light and Windows high-contrast themes use the system palette.

## 2.10 / browser 0.66.55 (recorded 0.63.85) (recorded desktop 2.09)

### Match the captured download list columns
- Scale the eight visible column defaults from the captured IDM header. Use the same defaults when resetting the layout.
- Use the captured Date Added and Save To captions in the Columns dialog and download list.

## 2.09 / browser 0.66.54 (recorded 0.63.85) (recorded desktop 2.08)

### Match the reference Categories pane layout
- Frame the Categories pane and align its top with the download list beneath the toolbar.
- Scale the pane insets, tree width and close button from the captured layout. Give larger configured fonts extra room in the heading.

## 2.08 / browser 0.66.53 (recorded 0.63.85) (recorded desktop 2.07)

### Match the reference queue column width
- Use the captured IDM width for the Q column on a fresh layout and when resetting the Columns dialog. Scale it with the display DPI.
- Preserve saved column widths and order for existing profiles.

## 2.07 / browser 0.66.52 (recorded 0.63.85) (recorded desktop 2.06)

### Match the reference desktop frame
- Show the build version in the main window title.
- Use the captured IDM frame as the starting size for the main window, scaled to the display DPI.

## 2.06 / browser 0.66.51 (recorded 0.63.85) (recorded desktop 2.05)

### Show local save progress for captured browser downloads
- Show verification, copying and final save as separate steps. The copy percentage uses the bytes written to the destination staging file.
- Show the received file size, clear the network rate and ETA during the local save, and offer Retry save when the worker stops.

## 2.05 / browser 0.66.50 (recorded 0.63.85) (recorded desktop 1.105.0)

### Keep captured browser downloads paused across restarts
- Save the user's Pause decision before stopping a final-save worker. If the browser finishes receiving just as Pause arrives, the complete response still waits for Resume.
- Save the Resume decision before restarting the local copy. A worker stopped only by app shutdown can recover on startup.

## 2.04 / browser 0.66.49 (recorded 0.63.85) (recorded desktop 1.104.0)

### Resume after an interrupted staging copy
- Record the staging file's Windows identity before copying a captured response into it.
- Resume rebuilds a partial copy from the saved response when the staging file still belongs to that download. Older or unrecognized staging files are preserved and a fresh staging file is used.

## 2.03 / browser 0.66.48 (recorded 0.63.85) (recorded desktop 1.103.0)

### Choose a new destination after a captured-download save conflict
- If the destination becomes occupied before the verified staging file moves, UDM keeps the captured response and lets you choose a new destination in File Properties.
- Resume saves from the captured data. A pending save whose outcome cannot be verified keeps its original destination for recovery.

## 2.02 / browser 0.66.47 (recorded 0.63.85) (recorded desktop 1.102.0)

### Recover captured downloads after the desktop opens
- The desktop opens the download list before preparing saves left incomplete in the previous session.
- A worker can finish a captured response from its saved body and receipt, even when the browser integration is disconnected. Recovered downloads keep automatic completion actions disabled.

## 2.01 / browser 0.66.46 (recorded 0.63.85) (recorded desktop 1.101.0)

### Keep the desktop responsive while saving a captured browser response
- Verify and copy the received file on a worker. The catalog lock covers the final save transaction.
- Pause can stop preparation. Resume retries the saved browser response locally after a pause or destination error.

## 2.00 / browser 0.66.45 (recorded 0.63.85) (recorded desktop 1.100.0)

### Keep the desktop responsive during captured-download virus checks
- Run configured virus checks for original browser responses on a worker after saving the file.
- Post the completion notification after the scanner records its result. Downloads with no configured scanner report completion immediately.

## 1.99.0 / browser 0.66.44 (recorded 0.63.85)

### Carry configured virus checks through save recovery
- Save the pending check with the completed file record. After a restart or recovered save, UDM runs any pending check on a background worker.
- Preserve an existing scan result for the same completed file. A scan interrupted while the antivirus was running remains marked as interrupted.
- Keep automatic completion actions disarmed for recovered files.

## 1.98.0 / browser 0.66.43 (recorded 0.63.85)

### Recover ordinary downloads interrupted during the final save
- Record the staged file's Windows identity when the checksum is deferred. Recovery checks that identity and computes the checksum before completing the catalog entry.
- Resume can finish the pending rename for a matching staged download without contacting the server.
- Synchronization replacements record the new file's checksum before updating the original.

## 1.97.0 / browser 0.66.42 (recorded 0.63.85)

### Retry saving an assembled live DASH recording
- Save the verified output size, checksum and caption result with the sealed recording before publishing the file.
- Retry verifies the saved assembly before rereading individual segments. If the assembly is unavailable or fails verification, UDM uses its existing recording recovery.
- Show the exact assembled byte count during saving and retain the saved size after a destination failure.

## 1.96.0 / browser 0.66.41 (recorded 0.63.85)

### Protect the measured video while Download File Info is open
- Read the exact size and SHA-256 from the same open assembly for direct and recorded HLS/DASH downloads.
- Block writes and deletion while waiting for Start. Check that the staging path still identifies the measured file before handing it to the existing save workflow.

## 1.95.0 / browser 0.66.40 (recorded 0.63.85)

### Retain media assembled after Start
- Save a verified output receipt before publication when assembly finishes after Start. Retry can reuse that file if publication fails.
- Show the verified final size during publication for freshly assembled and reused media.

## 1.94.0 / browser 0.66.39 (recorded 0.63.85)

### Keep prepared video available when Start cannot save it
- Save the verified assembly receipt with Start so the video can be retried after a destination or publication failure.
- Show the failed save in the progress window when progress dialogs are enabled.

## 1.93.0 / browser 0.66.38 (recorded 0.63.85)

### Resolve interrupted saves before editing File Properties
- Recover pending file publication at its original destination before applying property changes. If recovery completes the download, reopen File Properties and use Move to choose a different location.

## 1.92.0 / browser 0.66.37 (recorded 0.63.85)

### Offer Retry save for sealed live recordings
- Keep the sealed capture state across restarts. The progress window distinguishes recording, saving and retrying a save.
- Show the saved assembly size for live HLS when a verified assembly receipt is available.

## 1.91.0 / browser 0.66.36 (recorded 0.63.85)

### Reuse a verified live HLS recording after a failed save
- Checkpoint the assembled recording before publication. Resume verifies its size, checksum and sealed recording identity before reusing it.
- Keep the source recording for normal recovery if the saved assembly is missing or fails verification.

## 1.90.0 / browser 0.66.35 (recorded 0.63.85)

### Save live recording details with the completed file
- Include completion state, assembly timing and the DASH caption result in the publication journal. An interrupted catalog save can recover those details with the file.

## 1.89.0 / browser 0.66.34 (recorded 0.63.85)

### Finish saved live HLS and DASH recordings offline
- Let local recording recovery and publication proceed before dialing. Establish the configured connection only when live capture needs another playlist, manifest or media request.

## 1.88.0 / browser 0.66.33 (recorded 0.63.85)

### Resume verified local downloads without dialing
- Check saved HTTP prefetch, completed media tracks and retained assemblies before establishing a dial-up connection.
- Recorded HLS and DASH request a connection only when a segment must be fetched or verified against the server.

## 1.87.0 / browser 0.66.32 (recorded 0.63.85)

### Show the saved assembly size for deferred video
- Download File Info, File Properties and progress show the saved output size while a retained assembly is available.
- The progress window identifies the verified final byte count when publication begins.

## 1.86.0 / browser 0.66.31 (recorded 0.63.85)

### Recover interrupted publication of retained media
- Finish a confirmed download from its verified staging file when the final name is free.
- Reconcile the catalog and clean up the retained source after a completed publication is recovered.

## 1.85.0 / browser 0.66.30 (recorded 0.63.85)

### Verify cross-drive media copies through the destination handle
- Copy prepared media in 1 MiB chunks and hash the flushed destination through the same file handle before publication.
- Clean up only temporary files created by this publication attempt.

## 1.84.0 / browser 0.66.29 (recorded 0.63.85)

### Reuse recorded HLS and DASH assemblies after Download Later
- Keep verified MP4, M4A and TS output when Download Later defers an assembled recording.
- Resume verifies the saved file and its media selection before publishing it. Missing or mismatched output goes through the regular segment workflow.

## 1.83.0 / browser 0.66.28 (recorded 0.63.85)

### Reuse verified YouTube media after Download Later
- Keep a completed MP4 or M4A assembly when Download Later defers a direct or SABR capture. Verify its size, SHA-256 and selected-stream binding before publishing it on Resume.
- Fall back to the saved tracks if the assembly is missing, damaged or belongs to an earlier selection.

## 1.82.0 / browser 0.66.27 (recorded 0.63.85)

### Save Download Later before stopping prefetch
- Commit the destination, queue membership and folder preferences together before stopping the background transfer.
- Keep prefetch available when the destination is invalid or the catalog save fails. A successful defer keeps resumable parts for the queue.

## 1.81.0 / browser 0.66.26 (recorded 0.63.85)

### Save folder history with prefetched download confirmation
- Commit the selected destination and category folder preferences with Start in one catalog write. Preserve the prepared download and previous folder settings when confirmation fails.

## 1.80.0 / browser 0.66.25 (recorded 0.63.85)

### Signal prepared downloads when Start is saved
- Wake the publication worker as soon as confirmation is saved. Keep the verified media size available without repeatedly reading the staging file.
- Include adaptive media completion metadata in the publication transaction.

## 1.79.0 / browser 0.66.24 (recorded 0.63.85)

### Show prefetched bytes when Start opens progress
- Deliver transfer notifications directly to the desktop UI. The periodic refresh can still pick up events deferred by a modal dialog.
- Show how many bytes had already been received at confirmation, including separate audio and video tracks. Keep the transfer rate based on live measurements.

## 1.78.0 / browser 0.66.23 (recorded 0.63.85)

### Prioritize accepted browser download dialogs
- Open accepted browser offers before scanning older confirmation records. Read the saved receipt journal in place and revalidate only the selected download's offer before opening it.

## 1.77.0 / browser 0.66.22 (recorded 0.63.85)

### Show the exact final byte count in compact Download File Info
- Display the byte count when verified prefetch or a supported metadata check supplies it. Label selected-track totals and estimates separately.
- Expose Check size in the compact dialog for supported captures while prefetch is inactive.

## 1.76.0 / browser 0.66.21 (recorded 0.63.85)

### Show Download File Info before loading the file association
- Load the Windows file icon and type name in the background after the dialog is visible. The dialog displays any known content type immediately and fills in the Shell details when available.

## 1.75.0 / browser 0.66.20 (recorded 0.63.85)

### Keep downloads awaiting user action out of Scheduler runs
- Share eligibility checks across queue starts, retries, synchronization recovery and wake planning. Browser recapture, pending confirmation and unresolved response handling take precedence over scheduled work.
- Preserve pending jobs for follow-up without counting them as runnable work or as exhausted retries.

## 1.74.0 / browser 0.66.19 (recorded 0.63.85)

### Apply edits to a one-time Scheduler run
- Reevaluate a rearmed one-time schedule on the next Scheduler tick, even if the previous window was still open when the edit was saved.

## 1.73.0 / browser 0.66.18 (recorded 0.63.85)

### Honor server retry deadlines in scheduled wakes
- When every eligible file must wait, defer the scheduled wake until the earliest server retry deadline. If that deadline falls outside the queue window, use the next eligible opening.

## 1.72.0 / browser 0.66.17 (recorded 0.63.85)

### Match scheduled wake work to queue preparation
- Keep the synchronization original out of wake planning while a replacement is reserved. An eligible replacement can still supply the work for the wake.
- Exclude jobs that require review before retrying. Share synchronization eligibility between queue preparation and wake planning.

## 1.71.0 / browser 0.66.16 (recorded 0.63.85)

### Recover interrupted synchronization on startup
- Recover a checkpointed synchronization check or replacement after an interruption when its queue is enabled and configured for scheduled or startup processing.
- Reconcile completed publication journals first and avoid starting a duplicate check when a replacement is already reserved. An explicit file Stop clears the active check marker.

## 1.70.0 / browser 0.66.15 (recorded 0.63.85)

### Save synchronization admission in one operation
- Commit a replacement or missing-file restoration together with its queue, connection settings, and synchronization metadata.
- Roll back the reservation if the catalog cannot be saved. A restoration rejects a destination that appeared after the missing-file check.

## 1.69.0 / browser 0.66.14 (recorded 0.63.85)

### Resume synchronization after a recurring schedule stop
- Keep an interrupted synchronization check or replacement pending when an enabled queue reaches its daily stop. The next eligible window can resume the work.
- Clear scheduled retry intent on explicit file Stop. Terminal deadlines and stopped queues leave synchronization work paused.

## 1.68.0 / browser 0.66.13 (recorded 0.63.85)

### Retain server retry deadlines after a transfer fails
- Record Retry-After when an HTTP probe or range receives it. A subsequent error without a deadline or with a shorter deadline preserves the later wait for automatic queue retries.
- Measure the saved deadline from receipt of the response so reporting that error later does not restart the wait.

## 1.67.0 / browser 0.66.12 (recorded 0.63.85)

### Honor server retry timing across HTTP range handoffs
- Share the server's Retry-After deadline before releasing a failed range. New requests for the same download wait for the latest deadline, including probes after a resource change.
- Keep reads from responses already open moving while new requests wait.

## 1.66.0 / browser 0.66.11 (recorded 0.63.85)

### Use current response progress for HTTP range handoff
- Return a failed range to peers only when another live response has delivered bytes recently. Finished responses and stale progress no longer qualify.
- Release the worker's pending disk-write reservation when its response ends, including failed write paths.

## 1.65.0 / browser 0.66.10 (recorded 0.63.85)

### Keep reassigned HTTP ranges large enough
- Check the helper's actual byte range after excluding the active worker's reserved disk write. If the preferred split leaves too little to transfer, consider another eligible range.
- Rank ordinary tail splits by the bytes the helper can actually take.

## 1.64.0 / browser 0.66.9 (recorded 0.63.85)

### Keep queue operations tied to membership
- Count and dispatch queue members when filling simultaneous-download slots. Browser-owned responses outside the queue can continue during a queue stop.
- Use registered worker tokens for queue cancellation and block global completion actions while a browser-owned response is active.

## 1.63.0 / browser 0.66.8 (recorded 0.63.85)

### Match new queue concurrency defaults
- Start new built-in queues at four simultaneous files and new custom queues at one. Retain previously saved queue limits.
- Set the fresh-profile global transfer limit to four so the main queue can reach its default concurrency.

## 1.62.0 / browser 0.66.7 (recorded 0.63.85)

### Match the Scheduler file-list presentation
- Use the reference column captions and starting widths, scaled to the display DPI, and show gridlines.
- Use a numeric simultaneous-download field and align its caption to the reference dialog.

## 1.61.0 / browser 0.66.6 (recorded 0.63.85)

### Use IDM's built-in queue labels
- Display Main download queue and Synchronization queue in the tree, Scheduler, menus, selectors, and queue notices. Scheduler places both built-in queues above custom queues.
- Keep saved queue identities and download assignments. Distinguish custom queues with a conflicting display name by appending (custom).

## 1.60.0 / browser 0.66.5 (recorded 0.63.85)

### Match queue Start and Stop availability
- Show Start now for an idle queue and Stop for an active queue cycle in the queue tree context menu and Scheduler. Recheck the queue state when either command runs.
- Show the queue menu on the Queues root, with Create new queue available.

## 1.59.0 / browser 0.66.4 (recorded 0.63.85)

### Match IDM's queue context menu
- Open a selected queue on the Files in the queue tab from Edit queue, or on the Schedule tab from Schedule.
- Use the reference command order and separators. Protect built-in queues from Delete and reject duplicate names when creating a queue from the main window.

## 1.58.0 / browser 0.66.3 (recorded 0.63.85)

### Match IDM's toolbar queue routing
- Start Queue and Stop Queue on the main toolbar target the primary download queue, including when the tree shows a different queue. Use the dropdown arrows to choose a specific queue.
- Start and Stop on a queue's tree context menu act on the queue that opened the menu. Stop uses the same availability check as the named dropdown action.

## 1.57.0 / browser 0.66.2 (recorded 0.63.85)

### Apply Save To changes immediately
- Save category folders, the remember-folder setting, the server-date setting, and temporary folders as each change is made. Closing Options with Cancel leaves these choices saved.
- Display the effective temporary directory in a read-only field. Use Browse to choose a folder or Use default to restore UDM's default location.

## 1.56.0 / browser 0.66.1 (recorded 0.63.85)

### Match category controls in Save To
- Show General first and display Compressed for the archive category. Keep category identities separate from the labels so saved rules and destinations follow the correct category.
- Display category file types and destination paths in read-only fields. Browse changes the selected destination, and General explains fallback routing with Edit disabled.
- Open the same category dialog from Save To and the tree. Its OK button commits the category immediately. Options retains pending choices for other categories and tabs.

## 1.55.0 / browser 0.66.0 (recorded 0.63.85)

### Match IDM's five default categories on new profiles
- Use the Compressed, Documents, Music, Programs, and Video extension lists found in IDM 6.43 build 10. Match archive prefixes such as `r0*` in built-in and editable category rules.
- Save unmatched downloads in the main download folder and use Compressed as the default archive subfolder. Existing profiles retain their category defaults and saved destinations.

## 1.54.0 / browser 0.65.99 (recorded 0.63.85)

### Keep file-details actions responsive during a slow lookup
- Refresh, Start, and Cancel can abandon the current metadata lookup without joining its worker. Each lookup owns its result, so late responses cannot replace a newer preview.
- Limit outstanding file-details workers to four per process. Show a timeout when the lookup deadline expires and offer a retry when the worker limit is reached.
- Recheck cancellation under the catalog lock before persisting a rotated browser session. An already committed session update keeps the existing durable save semantics.

## 1.53.0 / browser 0.65.98 (recorded 0.63.85)

### Close remote ZIP preview without joining blocked network work
- Give remote ZIP tasks independently owned request/result state and nonjoining cancellation, with at most four outstanding workers. Report cancellation immediately and a pending timeout without waiting for an operating-system call to return.
- Replace raw Manager captures in browser-session persistence with short lifetime leases. Destruction rejects new callbacks and drains callbacks already using Manager; ordinary shutdown rejects session writes while stopping.
- Retain current-session/catalog membership checks, synchronous durable cookie persistence and rollback. Preview workers retain no window/task pointer; custom callbacks must own their captured state.
- Keep ZIP byte-range/parser/validator limits and extraction policy. Network calls are not forcibly interrupted, and in-flight persistence can still delay shutdown or catalog access.
- Runtime cancellation, lifetime, session rotation, pool saturation and IDM responsiveness comparisons remain deferred.

## 1.52.0 / browser 0.65.97 (recorded 0.63.85)

### Match Find's bounded Windows input and empty-dialog behavior
- Read at most 2,047 UTF-16 units when submitting either Find UI, following the reference's 2,048-unit buffer. The draft control can still contain more text.
- Accept an empty Find dialog by retaining the prior query and applying the selected fields/case/whole-string options. Keep inline empty submission's separate session-clear behavior.
- Preserve exact Windows units for an unpaired surrogate, including one created by truncation, in bounded optional query metadata. Use those units for matching and native control restoration while keeping JSON Text valid UTF-8.
- Validate the raw-unit metadata and invalidate query/filter caches with it; remove it from an inactive filter. Existing ordinary query shape and synchronous save semantics remain.
- Runtime Unicode, clipboard, limit, option-change, persistence and IDM comparisons remain deferred.

## 1.51.0 / browser 0.65.96 (recorded 0.63.85)

### Add the reference-style submitted search bar
- Add the full-width Find label/input, Find and Settings buttons, and accessible close control using extracted IDM resource geometry and the current UI font's dialog units.
- Keep typed text as a draft until Find or Enter; timer refresh and Find Next continue using the committed query. Settings opens the existing Find dialog.
- Closing disables filtering and restores the category while retaining the query. A submitted empty field clears the session filter without overwriting the previous saved text.
- Preserve shared matching/caching and explicit-save failure handling. Move both categories and list below the bar and update control fonts with appearance/DPI changes.
- Runtime keyboard, focus, accessibility, narrow-window, DPI/theme and IDM comparisons remain deferred.

## 1.50.0 / browser 0.65.95 (recorded 0.63.85)

### Connect Find results to the visible list filter
- Accepting Find reveals and fills the inline search field, filters the current category with the selected filename/description/address and case/whole-string options, then selects the next result.
- Use the same metadata-aware matcher for inline filtering and Find Next, including protected redirected/media addresses and Referer. Snapshot search inputs under the catalog lock and match outside it; cache unchanged source/query results without retaining decrypted data.
- Include query options and exact text in view/cursor invalidation. Inline text edits update the session query; clearing the field restores the current category.
- Restore the filename checkbox after an all-fields-unchecked rejection. Find Next with no query does nothing, following the reference guard.
- Runtime interaction, filter performance, captured-address changes, accessibility and IDM comparison remain deferred. Inline typing timing and full search-bar layout still differ from the reference command-driven path.

## 1.49.0 / browser 0.65.94 (recorded 0.63.85)

### Prevent one stalled file check from blocking every advisory answer
- Replace the single command-file probe worker with up to four shared-state workers and one current deduplicated path request.
- Publish completed path answers on the next observation without waiting for the entire selection. Free workers can service a new selection while stale probes remain blocked.
- Retain matching answers during periodic refresh, reject stale generations, and keep fresh command-time validation authoritative. Closing still never joins blocked filesystem calls.
- Pool saturation, menu timing, storage faults and interaction remain runtime-unverified; tests remain deferred.

## 1.48.0 / browser 0.65.93 (recorded 0.63.85)

### Move periodic history checkpoints off the window thread
- Run periodic dirty scans, detached JSON serialization and durable catalog writes on one owned worker, coalescing pending requests.
- Preserve synchronous explicit saves, ordered snapshot publication and completion durability checks. Stop and join periodic work before the final shutdown save; allow restart after a failed shutdown.
- Keep the worker alive only within Manager lifetime. Existing checkpoint assertions are adapted for eventual persistence but remain unrun.
- Scheduler transitions and explicit actions may still save synchronously; runtime responsiveness, concurrency and shutdown tests remain deferred.

## 1.47.0 / browser 0.65.92 (recorded 0.63.85)

### Recover interrupted DASH prefix checkpoint publication
- Read protected primary and scratch prefix checkpoints independently, selecting the largest hash-verified prefix for the exact request URL and falling back to the other candidate when needed.
- Promote a verified scratch checkpoint before requesting more bytes, preserving its evidence before later checkpoint writes reuse the scratch filename. Changed/unpublishable verified metadata produces an error instead of an unguarded restart.
- Clear both old checkpoint names before resetting a retained prefix for an accepted full replacement. Preserve validator/range, unverified-tail, traffic-accounting and final whole-resource checks.
- Runtime crash, corruption, permissions, range, filesystem and IDM comparisons remain deferred.

## 1.46.0 / browser 0.65.91 (recorded 0.63.85)

### Remove completed history when the saved file is already missing
- Let the combined remove-and-recycle workflow continue approved catalog removal when Windows reports file/path not found after scoped recovery.
- Skip Shell work and successful-recycle metadata for the absent file; standalone Recycle still requires a regular existing target.
- Reject stale catalog references before recycling and recheck membership, activity and completion after recovery. Other attribute errors, directory/reparse rejection, Shell cancellation/failure and no-permanent-delete policy remain.
- Runtime missing-file, inaccessible-storage, recovery, recycle and IDM comparisons remain deferred.

## 1.45.0 / browser 0.65.90 (recorded 0.63.85)

### Separate mixed-selection removal decisions
- Confirm completed records first and unfinished records separately, with the reference-derived single/plural/mixed unfinished wording.
- Let No on the unfinished prompt preserve that group while removing the already-approved completed records; No on the completed prompt cancels the operation.
- Keep independent suppression preferences and apply Recycle Bin selection only to approved completed files. Preserve original selection processing order.
- Recheck the approved completion class before actions and after scoped recovery, retaining changed records for review. Runtime dialog, partial-failure, recycle and IDM comparisons remain deferred.

## 1.44.0 / browser 0.65.89 (recorded 0.63.85)

### Retain Time Left estimates for stopped downloads
- Save the current attempt's sampled integer transfer rate on the download record, including zero samples, and retain it when active sampling stops.
- Use the cached rate for main-list/Scheduler Time Left and Time Left sorting when the live rate is unavailable. Paused, failed and queued records can retain a remaining-transfer estimate without displaying ongoing traffic.
- Include cached rates in immutable row snapshots and normal history checkpoints; existing records without them retain an unknown estimate until a transfer supplies a sample.
- Preserve completed/live/assembly exclusions, coverage and numeric bounds, the list's 10 B/s display threshold, and active-only progress-dialog calculations. Runtime pause/restart/schedule and IDM comparisons remain deferred.

## 1.43.0 / browser 0.65.88 (recorded 0.63.85)

### Responsive completed-ZIP preview
- Create the completed-file ZIP dialog before directory reading and use the existing virtual list instead of inserting up to 100,000 rows on the window thread.
- Preserve type-to-select through an explicit virtual-list search callback with case-insensitive exact/prefix matching and requested wrap behavior, shared by local and remote ZIP lists.
- Open/read/parse on a shared-state local task, with cancellation between bounded reads and immediate dialog closure without joining blocked I/O.
- Hold a read-only file handle, reject non-disk/directory sources, apply the existing bounded parser/read budget, and verify file identity, size and last-write metadata before publishing the listing.
- Check the ZIP's authoritative open in the background task; retain current selection/completion/extension gates without a duplicate synchronous disk probe during dispatch.
- Limit outstanding local preview workers to four. Runtime file-mutation, cancellation, large-list, DPI and IDM comparison acceptance remain deferred.

## 1.42.0 / browser 0.65.87 (recorded 0.63.85)

### Background file checks for menu responsiveness
- Move ZIP/recycle advisory regular-file probes off the window thread into one background worker with a replaceable pending selection batch.
- Prewarm checks after coalesced selection changes and during regular toolbar refresh; ignore results from superseded selections and periodically refresh matching results.
- Keep unknown paths disabled until their first result, retain known advisory results while refreshing, and perform fresh filesystem checks when invoking commands.
- Keep blocked filesystem work independent of the window/manager and avoid waiting for it during shutdown. Runtime latency, menu interaction, large-selection and shutdown acceptance remain deferred.

## 1.41.0 / browser 0.65.86 (recorded 0.63.85)

### Resume DASH resources with strong modification dates
- Retain and reuse verified whole-resource prefixes on servers without ETags when a complete Last-Modified/Date pair supplies sufficient clock separation.
- Persist the date evidence in protected checkpoints and send the saved Last-Modified value in If-Range; prefer strong ETags and exclude weak, duplicated or malformed tags from date fallback.
- Require the same validator, exact remaining range, stable total and existing framing/hash checks before appending. Changed or insufficient validators use the existing full-response fallback.
- Runtime restart, server/proxy clock behavior, corruption and IDM comparisons remain deferred.

## 1.40.0 / browser 0.65.85 (recorded 0.63.85)

### Recover legacy completed DASH receipt scratch
- Recognize the known protected full-receipt scratch format from older builds and recover its already-promoted payload after exact binding, size and SHA-256 verification.
- Keep prefix checkpoints, arbitrary orphan payloads and unknown receipt schemas excluded.
- Persist an independent promoted-payload completion intent before rewriting the legacy receipt scratch, allowing another interruption to retain completion evidence.
- Runtime migration, corruption, restart and IDM comparisons remain deferred.

## 1.39.0 / browser 0.65.84 (recorded 0.63.85)

### Recover completed DASH resource publication
- Persist a protected completion intent after body completion, held-file size/hash verification and flushing, before promoting the payload and publishing its full receipt.
- Recover a matching staged or already-promoted payload during inspection/reconciliation without another HTTP request; retain range/version length requirements.
- Keep completed staging on local publication failure and surface that failure instead of deleting finished bytes or retrying another CDN.
- Remove redundant internal staging after a verified full receipt, preserving committed media. Runtime crash/fault, reconciliation and IDM comparisons remain deferred.

## 1.38.0 / browser 0.65.83 (recorded 0.63.85)

### Resumable DASH resource prefixes
- Keep protected, SHA-256-bound checkpoints for eligible whole resources with a strong ETag, including early streaming responses.
- Flush the first received prefix, then checkpoint at most once per second as data arrives; preserve the latest valid prefix on cancellation or read failure.
- Verify saved bytes before offering Range/If-Range, require the same canonical URL/entity tag and an exact single remaining-range response, and restart fully when the range cannot be used.
- Ignore uncheckpointed crash tails, compare the completed disk file with the running digest, and remove prefix state only after publishing the full-resource receipt.
- Weak/missing validators, URL changes and shared/version-bound resources retain full-request behavior. Runtime faults, pause/resume, crash and IDM comparisons remain deferred.

## 1.37.0 / browser 0.65.82 (recorded 0.63.85)

### Early live-DASH segment streaming
- Admit availabilityTimeComplete=false whole-segment locations at their signaled availability time instead of rejecting the live presentation.
- Carry per-location streaming policy through clock-uncertainty filtering and track workers; compare availability locations using the same URL canonicalization as requests.
- Require unambiguous Content-Length, HTTP/1 chunked framing or HTTP/2/3 stream framing for early responses. Read through completion before publishing a resource receipt.
- Retain existing credential scopes, cancellation, bounded temporary files and whole-segment recovery. Growing shared byte-range/version-bound resources and individual-chunk resumption remain unfinished.
- Browser, live-server, framing-fault, recovery and IDM runtime comparisons remain deferred.

## 1.36.0 / browser 0.65.81 (recorded 0.63.85)

### Find Next session cursor
- Continue Find from its own next-row cursor instead of restarting after each manually selected row, following the retained IDM Find Next routine.
- Advance after a match, wrap through row zero after reaching the end, and keep the cursor at zero after a full unsuccessful pass.
- Restart on changed query/scope; synchronize with the first restored selection after same-scope list rebuilds or ordering changes. Keep the cursor session-only.
- Follow the reference partial-visibility request and focus/selection sequence for a found row. Runtime navigation, scrolling, rebuild and IDM comparisons remain deferred.

## 1.35.0 / browser 0.65.80 (recorded 0.63.85)

### Remember Find text and options
- Restore the last accepted Find text, field choices, Match Case and Whole String options when the main window is created; F3 can use the restored query.
- Save accepted Find state through a targeted settings update with rollback on save failure, rather than replacing unrelated preferences from a stale copy.
- Keep Cancel and failed saves from replacing the active or saved query. Normalize absent/malformed saved Find fields to usable defaults during catalog loading.
- Preserve reference-derived matching and current selection-based Find Next navigation. Restart, storage-failure, GUI and IDM comparisons remain deferred.

## 1.34.0 / browser 0.65.79 (recorded 0.63.85)

### Reference-derived Find site matching
- Whole-string address search now accepts either the complete URL or its literal site portion, following the retained IDM Find helper.
- Use Windows uppercase folding for case-insensitive Find and prepare the query once per Find Next operation.
- Copy only the selected search scopes for one row at a time under the manager lock; protected header/media decoding, URL parsing and matching now run after release.
- Preserve field choices, case-sensitive search, whole filename/description matching, visible-list order and wrap behavior. Existing assertion coverage was corrected/extended but remains unrun.
- Runtime search/Unicode, GUI responsiveness and IDM comparisons remain deferred.

## 1.33.0 / browser 0.65.78 (recorded 0.63.85)

### Queue menu availability and access
- Capture ordered queue names and Stop availability under one manager lock, with at most one shared catalog pass instead of a pass per queue.
- Share the tray Start/Stop snapshot, refresh dynamic queue submenus when opened, and use captured flags during MFC's per-item update routing.
- Keep queue submenus accessible when their first queue cannot stop; each child retains its own enabled state.
- Preserve relocation exclusions, active-transfer handling, enabled queued/synchronization work, manual/cycling work, bounded menu IDs and the large-queue chooser. Recheck Stop eligibility on execution.
- Runtime timing, menu interaction and IDM comparisons remain deferred.

## 1.32.0 / browser 0.65.77 (recorded 0.63.85)

### Batched command availability and shorter UI lock scopes
- Read list selection outside the manager lock and batch toolbar/context-menu decisions from one captured selection.
- Share the catalog pass for requested global Stop All, open-progress, completed-cleanup and recovery decisions, with short-circuit completion when possible.
- Remove the context menu's outer manager lock; resolve detached ZIP/recycle file checks and redownload FTP URL parsing before creating/updating controls.
- Reuse exact-path file status only within one batch; filesystem errors disable the relevant command. Action-time eligibility is still freshly queried.
- Skip selection/manager/settings work for commands that do not need it, and return an empty selection while the list control is absent.
- File probes remain synchronous on the UI thread; runtime timing, concurrency and IDM comparisons remain deferred.

## 1.31.0 / browser 0.65.76 (recorded 0.63.85)

### Navigation snapshots and stable tree refresh
- Limit navigation's manager-lock scope to category names, queue names/icon kinds and project IDs/names; compare snapshots, serialize structural keys and update Windows controls afterward.
- Update queue schedule/type icons in place, preserving node handles and avoiding full tree rebuilds for image-only changes.
- Preserve expanded groups, selected identity and vertical item anchor across structural rebuilds. Removed selections fall back to the nearest surviving ancestor; the Queues root has a distinct identity from All Downloads.
- Restore redraw and invalidate the rebuild cache after interrupted construction. Keep queue-image invalidation compatible with image-list/DPI refresh.
- Existing navigation assertions were maintained; runtime interaction, concurrency, performance and IDM comparisons remain deferred.

## 1.30.0 / browser 0.65.75 (recorded 0.63.85)

### Fractional media-tick MP4 caption timestamps
- Keep inline WebVTT millisecond offsets independent of the MP4 track timescale, avoiding premature whole-tick truncation before forward edit-rate scaling.
- Combine rational inline offsets with sample/edit, presentation-offset and movie clocks before final millisecond rounding, with checked integer additions and bounded clock inputs.
- Subtract the edit origin before adding inline offsets, avoiding unnecessary large absolute-clock intermediates.
- Advance caption assembly compatibility to revision 4 so retained live Period/session outputs rebuild under the corrected mapper; audio/video-only reuse is unchanged.
- Reverse/dwell timed text still requires temporal text-state support and remains explicitly unsupported. Runtime tests, installers and installation remain deferred.

## 1.29.0 / browser 0.65.74 (recorded 0.63.85)

### Reference-derived Size and Transfer Rate ordering
- Sort unknown/zero-byte-size media by unrounded duration, with positive byte sizes forming a separate group, following IDM's Size comparator.
- Sort and display main-list transfer rates as whole bytes per second; use the same conversion for list ETA. Invalid/non-finite rates become zero, and finite overflow is guarded before integer conversion.
- Reuse ordering only when the byte/duration or integer-rate keys match, preserving stable equal-key catalog order and snapshot-based work outside the manager lock.
- Last Try Date comparator review is recorded separately; malformed/legacy date behavior and all runtime comparisons remain unverified. Tests and installers remain deferred.

## 1.28.0 / browser 0.65.73 (recorded 0.63.85)

### Uncapped ETA calculation and reference-derived Time Left behavior
- Remove the one-day cap from duration-based media ETA, retaining the current-attempt resume baseline.
- Guard byte/media estimates before integer conversion; invalid coverage stays unknown and estimates beyond the integer range saturate safely. Fully received active downloads show zero seconds remaining.
- Use integer-rate estimates in list cells and suppress Time Left at 10 B/s or less, while progress dialogs retain fractional-rate estimates.
- Sort Time Left with zero for unavailable values and completion-state tie breaking, following the reference callback; low-rate estimates can still participate in sorting while their cells are blank.
- Preserve the reference formatter's blank-space result above 10,000 hours. Native runtime and reference comparisons remain deferred.

## 1.27.0 / browser 0.65.72 (recorded 0.63.85)

### Reference-derived Status percentage display
- Preserve percentage text for unfinished downloads with retained progress, including failures, instead of replacing it with Error/Not Found solely because the state is Failed.
- Use duration, segment or byte coverage in that order; format unrounded source coverage through the reference's float and truncated-hundredths path.
- Honor the Windows decimal separator, with a thread-local cache invalidated on settings changes; repaint unchanged rows when locale settings change.
- Show neutral blank cells before progress is available. Keep unstarted failure labels, live-recording states and explicit assembly phases.
- Percentage text can reach 100 before publication, as in the reference; Complete still requires the stored completion state. Runtime tests and visual/locale comparisons remain deferred.

## 1.26.0 / browser 0.65.71 (recorded 0.63.85)

### IDM Status ordering and HTTP 404 labels
- Sort Status by completion group and unrounded progress across unfinished downloads, including paused and queued rows, instead of alphabetic state names and rounded active-only percentages.
- Follow the reference's byte-ratio/media-percent score distinction and zero-progress fallback-label ordering in both directions; keep stable equal keys.
- Show Not Found for failed HTTP 404 downloads and retain Error for other failures. Include the HTTP result in immutable list snapshots so changes refresh correctly.
- Bind sort reuse to completion, full progress score and fallback label. Runtime reference, media-clock mapping and visual comparisons remain deferred.

## 1.25.0 / browser 0.65.70 (recorded 0.63.85)

### IDM multibyte Description and Referer sorting
- Compare temporary Windows ANSI keys using the multibyte CRT path traced in IDM, instead of lowercasing UTF-8 bytes.
- Retain full Unicode metadata/display text and raw-key cache invalidation; these columns have dynamically sized comparison keys, without the wide getter's 255-unit limit.
- Keep empty-last ascending order and stable ties. If the CRT reports a comparison error, restart the entire sort with a consistent byte comparator instead of mixing error results into ordering.
- Runtime code-page, locale, visual and performance comparisons remain deferred.

## 1.24.0 / browser 0.65.69 (recorded 0.63.85)

### IDM optional-text column ordering
- Sort populated Description, Save Path, Referer and Parent Web Page values before empty values ascending, reversing that order descending.
- Compare Save Path and Parent Web Page with case-insensitive UTF-16 CRT comparison, following the reference callbacks.
- Bound filename, Save Path and Parent Web Page comparison keys to the reference getter's 255 UTF-16 units; retain complete stored and displayed text.
- Preserve stable ties and unchanged-order reuse. Runtime comparisons and locale qualification remain deferred.

## 1.23.0 / browser 0.65.68 (recorded 0.63.85)

### IDM-style logical filename ordering
- Use Windows StrCmpLogicalW for the main list's File Name sort, matching the call traced from the installed IDM reference's By File Name command.
- Compare Unicode filenames with numerical digit runs and case-insensitive Windows ordering in both directions, retaining stable equal-key order.
- Build UTF-16 keys only when a new filename sort is needed; unchanged-order validation reads immutable raw names without extra string copies.
- Invalidate cached ordering on Windows settings broadcasts. Runtime comparative and locale tests remain deferred.

## 1.22.0 / browser 0.65.67 (recorded 0.63.85)

### Reuse unchanged download-list ordering
- Skip repeated stable sorting when the selected column/direction, candidate sequence and relevant keys match the prior refresh.
- Re-sort after additions, removals, job replacement, catalog reordering or relevant key changes, preserving existing stable-tie behavior.
- Continue updating progress, speed, quota state and asynchronous icons independently of sort reuse.
- Runtime performance and interaction tests remain deferred.

## 1.21.0 / browser 0.65.66 (recorded 0.63.85)

### Background file-type icons
- Show category/cached icons immediately and resolve Windows file-type icons on a dedicated COM-initialized background worker.
- Load icons as rows enter the viewport, deduplicate and bound pending work, and discard stale results when appearance/DPI image lists are rebuilt.
- Keep all list/image-list updates on the UI thread; closing UDM does not wait for a blocked Shell lookup.
- Recompute category fallbacks per row when an association lookup fails; runtime responsiveness and visual tests remain deferred.

## 1.20.0 / browser 0.65.65 (recorded 0.63.85)

### Download-list refresh responsiveness
- Snapshot list fields and queue indicators under the manager lock, then release it before sorting, header decoding, shell-icon lookup and row painting.
- Cache display-only row records instead of full download records containing transfer journals and capture state.
- Preserve selection, focus, scroll anchoring, filters, quota indicators, sort keys and displayed progress from one consistent refresh snapshot.
- Runtime performance comparisons and interaction tests remain deferred.

## 1.19.0 / browser 0.65.64 (recorded 0.63.85)

### Live-DASH captions across silent Periods
- Join audio/video Periods independently and mux rebased caption cues once for the full recording.
- Preserve verified caption coverage and configuration checks, bounded cue storage, language/name metadata and retained-fragment recovery.
- Save all-silent intervals without fabricated cues, with a persisted caption result visible in progress details and Last attempt and result.
- Advance caption output receipt revision and retain cue counts across assembled-output reuse; runtime tests remain deferred.

## 1.18.0 / browser 0.65.63 (recorded 0.63.85)

### Direct queue controls from the tray
- List named queues directly under tray Start queue and Stop queue submenus, using the existing main-menu queue commands.
- Disable inactive Stop entries when populating a queue menu.
- Use an editable queue chooser beyond 1,000 entries instead of assigning an unbounded range of popup command IDs.
- Runtime and visual tests remain deferred.

## 1.17.0 / browser 0.65.62 (recorded 0.63.85)

### Media assembly track verification
- Verify selected audio and video tracks before exact-size confirmation and publication.
- Compare source/output codecs, video dimensions, audio channels/sample rate and available per-track durations; retain saved streams on verification failure.
- Keep the selected-height and AAC audio-only checks; runtime tests remain deferred.

## 1.16.1 / browser 0.65.61 (recorded 0.63.85)

### Caption assembly recovery compatibility
- Rebuild retained live-DASH caption outputs created before the current timing and mux rules.
- Bind both Period and joined-session assembly receipts to the caption implementation revision while preserving verified downloaded fragments and audio/video-only reuse.
- Runtime checks remain deferred.

## 1.16.0 / browser 0.65.60 (recorded 0.63.85)

### Forward MP4 WebVTT inline timing
- Map inline cue timestamps through ctim, positive playback edits and presentation offsets.
- Rebase mapped timestamps during recorded Period joining and live capture clipping.
- Reject missing timing metadata and unsupported reverse/dwell timed payloads explicitly; runtime checks remain deferred.

## 1.15.3 / browser 0.65.59 (recorded 0.63.85)

### MP4 WebVTT cue timing metadata
- Preserve and validate optional ctim metadata through sample/fragment decoding.
- Inline timestamp remapping remains unfinished; runtime checks remain deferred.

## 1.15.2 / browser 0.65.58 (recorded 0.63.85)

### Live-DASH caption output metadata
- Apply the selected caption label/language to final output, including single-Period sessions.
- Version caption assembly reuse records so older results cannot omit the metadata update.
- Runtime checks remain deferred.

## 1.15.1 / browser 0.65.57 (recorded 0.63.85)

### Live-DASH track metadata
- Validate selected audio/caption names, language tags and metadata-to-track consistency at native admission.
- Match the existing recorded-selection metadata bounds; runtime checks remain deferred.

## 1.15.0 / browser 0.65.56 (recorded 0.63.85)

### Browser live-DASH captions
- Discover and select supported live MP4 WebVTT caption tracks in Chromium and Firefox.
- Gate handoff on desktop support and include caption addressing/credential checks.
- Browser-to-file runtime verification remains deferred.

## 1.14.0 / browser 0.65.55 (recorded 0.63.84)

### Native live-DASH captions
- Admit a selected MP4 WebVTT track alongside video and optional audio, with explicit initialization and media-kind checks.
- Preserve three-track selection through recording-session recovery and refresh.
- Advertise desktop capability for forthcoming browser integration; runtime acceptance remains deferred.

## 1.13.1 / browser 0.65.54 (recorded 0.63.84)

### DASH recording caption muxing
- Decode retained MP4 WebVTT into an explicitly mapped subtitle stream with language metadata.
- Validate caption counts and preserve compatible subtitles across joined Period outputs.
- Live capture admission remains unfinished; runtime checks remain deferred.

## 1.13.0 / browser 0.65.53 (recorded 0.63.84)

### Retained DASH caption assembly
- Add native verified-file assembly of retained MP4 WebVTT fragments into clipped, rebased subtitle text.
- Preserve byte-range offsets and reject changed or incomplete retained resources.
- End-to-end live caption selection and muxing remain unfinished; runtime tests remain deferred.

## 1.12.3 / browser 0.65.52 (recorded 0.63.84)

### DASH media-kind preflight
- Verify requested live track kinds against inherited manifest metadata before recording.
- Add explicit MP4 WebVTT classification for the forthcoming caption contract; live captions remain unfinished.
- Runtime checks remain deferred.

## 1.12.2 / browser 0.65.51 (recorded 0.63.84)

### Shared MP4 WebVTT decoding
- Route recorded caption assembly through a retained-file decoder with explicit byte offsets and presentation clock bounds.
- Preserve existing fragment continuation and edit-list mapping for future live recording integration.
- Live captions remain unfinished; runtime tests remain deferred.

## 1.12.1 / browser 0.65.50 (recorded 0.63.84)

### Completed-file controls during relocation
- Disable conflicting completed-file actions and skip active records in Delete Completed.
- Keep download-progress closure independent of file-move cancellation; read destination text from a consistent snapshot.
- Runtime and visual checks remain deferred.

## 1.12.0 / browser 0.65.49 (recorded 0.63.84)

### File-move speed and timing
- Show measured processing speed, total elapsed time and current-phase remaining time during file moves.
- Reset samples between phases and decay speed during stalls; retain destination and byte-count displays.
- Runtime and visual checks remain deferred.

## 1.11.3 / browser 0.65.48 (recorded 0.63.84)

### Move recovery before file actions
- Reconcile the selected download's pending move before Remove, Recycle or Redownload.
- Preserve unresolved history and leave unrelated move journals alone.
- Runtime validation remains deferred.

## 1.11.2 / browser 0.65.47 (recorded 0.63.84)

### File moves and transfer scheduling
- Exclude relocation from transfer slot accounting and automatic queue-stop cancellation.
- Preserve file protection, shutdown cancellation and completion-action waiting while a move runs.
- Runtime validation remains deferred.

## 1.11.1 / browser 0.65.46 (recorded 0.63.84)

### Relocation verification progress
- Show byte progress during destination/original verification and cross-volume rollback.
- Keep the destination visible alongside phase byte counts; protect verification reads against concurrent writers.
- Recovery remains non-cancellable after a successful move. Runtime and visual checks remain deferred.

## 1.11.0 / browser 0.65.45 (recorded 0.63.84)

### Background Move/Rename
- Connect the desktop Move/Rename and Properties workflows to a background progress dialog with phase-aware cancellation.
- Release the manager lock during long file operations, reserve the destination, and wait for relocation during shutdown.
- Preserve unrelated metadata during location commit and recovery. Runtime and visual acceptance remain deferred.

## 1.10.0 / browser 0.65.44 (recorded 0.63.84)

### Move/Rename progress and cancellation backend
- Add optional phase/byte progress and cancellation to native relocation; existing callers keep their default behavior.
- Allow cancellation during initial hashing and Windows cross-volume copying, verifying original-file recovery before reporting a clean cancellation.
- Finish verification/history or rollback after a successful move despite late cancellation.
- Desktop progress/cancel UI and asynchronous locking remain unfinished; runtime tests remain deferred.

## 1.9.2 / browser 0.65.43 (recorded 0.63.84)

### Same-volume Move/Rename performance
- Avoid whole-file checksum reads for supported same-volume renames by locking the source against writes and verifying its Windows file identity, size and timestamps.
- Use the recorded identity for case-only renaming, rollback and recovery; retain compatibility with older checksum journals.
- Keep fresh checksum verification for cross-volume moves and unsupported filesystem identities. Runtime latency and background progress/cancellation remain unverified or unfinished.

## 1.9.1 / browser 0.65.42 (recorded 0.63.84)

### Move/Rename current-file recovery
- Journal and verify the actual file being moved, including files edited since download, without replacing historical download checksums.
- Verify original-file contents during recovery and verify rollback before clearing its journal.
- Preserve uncertain move outcomes and report Windows copy-without-source-removal failures instead of reporting a completed move.
- Runtime acceptance and large-file responsiveness remain outstanding.

## 1.9.0 / browser 0.65.41 (recorded 0.63.84)

### Completed-download filename case
- Allow Move/Rename to change only filename letter case, retaining destination collision protection.
- Record actual spelling and fresh content identity for recovery; verify stored spelling after rename and rollback.
- Preserve ambiguous recovery journals and prohibit file moves during deferred catalog saves. Runtime acceptance remains deferred.

## 1.8.0 / browser 0.65.40 (recorded 0.63.84)

### DASH per-segment index templates
- Accept Number/Time index templates alongside independently timed complete media segments, instead of misclassifying them as whole-representation indexes.
- Validate optional index locators without fetching their resources or sharing credentials with their origins; preserve the existing representation-wide SIDX path.
- Gate live handoff on desktop support. Runtime acceptance and index-based subsegment selection remain deferred.

## 1.7.0 / browser 0.65.39 (recorded 0.63.83)

### DASH duration-boundary overlaps
- Accept bounded non-negative pdDelta declarations in native DASH addressing and both browser catalogs instead of rejecting every duration-boundary declaration.
- Route recorded declarations through finite Period assembly, retaining boundary clipping without changing segment URLs or extending the output duration.
- Allow supported live template, list and indexed selections with duration overlaps; require the updated desktop capability before handoff.
- Negative duration gaps, malformed values and values beyond exact integer bounds remain unsupported. Runtime acceptance remains deferred.

## 1.6.8 / browser 0.65.38 (recorded 0.63.82)

### Catalog duplicate identity
- Distinguish imports by usable browser session, proxy route, form request, capture requirements, offline project and expected hash as well as URL and destination.
- Preserve distinct contexts as separate numbered entries rather than silently skipping them.
- Remember import-assigned filenames so importing the same numbered entry again does not create another copy; later filename changes invalidate that name match.
- An unreadable existing credential record no longer blocks importing a separate usable record. Incoming credentials still require validation.

## 1.6.7 / browser 0.65.37 (recorded 0.63.82)

### Imported download request context
- Preserve required form and media recapture flags when attaching verified completed files from a catalog; the saved files remain completed and usable.
- Preserve media recapture requirements through subsequent catalog exports.
- Download Again rejects missing form/media context and validates the saved browser session before creating a new entry, instead of treating an imported contextual download as an ordinary URL.

## 1.6.6 / browser 0.65.36 (recorded 0.63.82)

### Site Grabber start recovery
- Recover selected existing downloads before Site Grabber begins its start-batch transaction, so recovered history is durable before journals are removed.
- Preserve publication and replacement journals if recovery is requested inside a transaction that defers catalog saves.
- Preserve already committed recovery if a later project-start step fails; unselected downloads remain outside the recovery pass.

## 1.6.5 / browser 0.65.35 (recorded 0.63.82)

### Offline website admission
- Save a standalone offline archive entry only after its project settings, captured HTML and browser login are prepared.
- Restore the prior job list when archive preparation or persistence fails, avoiding incomplete entries after a restart.
- Preserve enclosing Site Grabber and Download Again transactions, so archive creation cannot commit their unfinished work.

## 1.6.4 / browser 0.65.34 (recorded 0.63.82)

### Download Again catalog transaction
- Commit new Download Again entries only after their inherited properties, browser session and offline-page data are ready.
- Roll back the new entry if preparation or the final catalog save fails, preventing partially configured entries from surviving a restart.
- Reject stale selections and overlapping catalog transactions before creating the copy.

## 1.6.3 / browser 0.65.33 (recorded 0.63.82)

### Move/Rename recovery
- Resolve pending publication and replacement recovery for the selected download before changing its file location, then recheck completion.
- Keep files at their existing location while a pending replacement still depends on them; reject stale catalog selections.
- Restore the in-memory record if saving recovered move history fails, retaining the recovery journal for retry.

## 1.6.2 / browser 0.65.32 (recorded 0.63.82)

### Removal recovery
- Reconcile only the selected download's publication and replacement records, including private audio/video tracks, before removing its catalog entry.
- Reject active or stale selections before removal, and preserve the record if recovery cannot complete.
- Recover pending saves before recycling a completed file and recheck its completion status before invoking Windows.

## 1.6.1 / browser 0.65.31 (recorded 0.63.82)

### WebVTT text timestamp synchronization
- Shift timestamps embedded in plain WebVTT cue text together with cue boundaries during HLS timestamp mapping and recorded/live discontinuity placement.
- Apply the same mapping to plain WebVTT DASH Period placement and clipping.
- Preserve text and formatting, remove timestamp markers outside the clipped cue, and reject recognized non-increasing or out-of-cue timestamps.
- MP4 WebVTT payload timestamp remapping remains deferred.

## 1.6.0 / browser 0.65.30 (recorded 0.63.82)

### Reverse MP4 caption playback edits
- Support finite reverse MP4 WebVTT edits at fixed-point speeds down to -16x, alongside forward and dwell edits.
- Reverse cue interval endpoints onto the movie clock, preserve text/settings, and retain movie-duration clipping and presentation sorting.
- Reject open-ended reverse edits and protect signed media-distance subtraction from overflow.

## 1.5.0 / browser 0.65.29 (recorded 0.63.82)

### Fractional MP4 caption playback edits
- Preserve the full 16.16 forward playback rate for recorded MP4 WebVTT edits, including slow motion and fractional acceleration through 16x.
- Convert caption clocks using bounded integer quotient/remainder scaling, avoiding direct multiplication of large timestamps.
- Retain finite dwell edits, movie-duration clipping, DASH offsets and output budgets; reverse playback remains unsupported.

## 1.4.0 / browser 0.65.28 (recorded 0.63.82)

### MP4 caption dwell edits
- Support finite zero-rate edits in recorded MP4 WebVTT captions, holding cues active at the selected media timestamp for the specified movie duration.
- Respect inclusive cue starts and exclusive cue ends, movie placement, DASH offsets, empty samples and existing output limits.
- Reject open-ended dwell edits; retain whole-number forward playback support from 1x through 16x.

## 1.3.0 / browser 0.65.27 (recorded 0.63.82)

### MP4 caption playback edits
- Support positive whole-number playback rates from 1x through 16x when assembling recorded MP4 WebVTT captions.
- Apply each edit's speed to media timing before movie placement and DASH presentation offsets; retain movie-duration clipping for finite edits.
- Preserve repeated/reordered edits, empty gaps and existing output limits. Fractional, zero and reverse rates remain unsupported.

## 1.2.15 / browser 0.65.26 (recorded 0.63.82)

### Replacement recovery
- Prepare both replacement-history records before changing the live catalog during recovery.
- Restore both in-memory records if saving the recovered catalog fails; retain the journal for another recovery attempt.
- Read required recovery snapshots before moving the preserved original back into place.

## 1.2.14 / browser 0.65.25 (recorded 0.63.82)

### Resume recovery
- Recover interrupted publication and replacement saves for the selected download and its private audio/video tracks when Resume is clicked, without requiring a full application restart.
- Leave unrelated downloads' recovery journals untouched and refuse recovery while the selected transfer or replacement original is active.
- Validate replacement-journal filenames against their recorded download IDs before applying recovery.

## 1.2.13 / browser 0.65.24 (recorded 0.63.82)

### Media recovery
- Find saved audio/video child tracks when recovering interrupted file publication, instead of requiring every track to be a top-level download.
- Keep path/hash validation and durable parent-catalog saving; reject ambiguous duplicate identifiers without deleting recovery data.
- Keep replacement-history recovery restricted to top-level download records.

## 1.2.12 / browser 0.65.23 (recorded 0.63.82)

### Media publication
- Allow internal audio/video track jobs to publish while preserving their initial catalog-membership state. The 1.2.9 membership guard incorrectly required these private child jobs to appear in the main download list.
- Continue rejecting changes to membership, destination and replacement metadata during verification.

## 1.2.11 / browser 0.65.22 (recorded 0.63.82)

### Interrupted-save recovery
- Preserve direct-download staging files while publication or replacement recovery is pending, including when journal state cannot be read.
- Prevent direct, recorded-media, live DASH and offline-archive assembly from overwriting pending recovery output on retry.
- Use the shared conservative journal check in recorded-media cleanup without replacing the original failure with a filesystem-check exception.

## 1.2.10 / browser 0.65.21 (recorded 0.63.82)

### Live recording recovery
- Preserve assembled live HLS output after a publication failure when its recovery journal exists or cannot be inspected.
- Block another assembly pass from overwriting that output until publication recovery is resolved.
- Share the conservative recovery-journal check with media publication cleanup.

## 1.2.9 / browser 0.65.20 (recorded 0.63.82)

### Responsiveness
- Move final replacement-file verification outside the catalog lock and pass Pause/cancellation through direct, media, live HLS and offline publication paths.
- Hash held file handles and revalidate file identity and replacement metadata before committing. Preserve the existing publication journal, previous-version backup and history rollback.

## 1.2.8 / browser 0.65.19 (recorded 0.63.82)

### Responsiveness
- Prepare synchronization replacement hashes outside the catalog lock, with cancellation between blocks, so large existing files do not block catalog access during that hash pass.
- Reuse the verified digest for replacement admission while holding a read-only sharing handle, and check file identity before consuming it.

## 1.2.7 / browser 0.65.18 (recorded 0.63.82)

### Scheduler
- Stop unscheduled periodic synchronization at local midnight when no explicit stop time is selected, matching IDM's documented end-of-day limit.
- Preserve the deadline across ordinary settings saves; an explicit Start or configured startup start arms the next deadline. Explicit stop times keep priority.

## 1.2.6 / browser 0.65.17 (recorded 0.63.82)

### Synchronization
- Verify the saved SHA-256 before declaring an unchanged remote file up to date, detecting local edits or corruption even when the file length is unchanged.
- Keep verification cancellable and prevent concurrent file writes during hashing. Legacy records without a valid saved hash retain their existing metadata check.

## 1.2.5 / browser 0.65.16 (recorded 0.63.82)

### Installation
- Register Download with UDM for .url files through SystemFileAssociations rather than modifying the InternetShortcut class, fixing the access-denied installation failure.
- Notify Explorer of association changes during installation and removal without changing the default browser.


## 1.2.4 / browser 0.65.15 (recorded 0.63.82)

### Fixes
- Stop automatic queue retries when a fresh browser proxy or browser session capture is required.
- Keep those entries from being treated as exhausted retry work that permits queue completion actions.


## 1.2.3 / browser 0.65.14 (recorded 0.63.82)

### Fixes
- Preserve restored-file synchronization status after resume, retry, and publication recovery.
- Stop automatic synchronization retries for webpage responses and content-prefix errors that require user review.


## 1.2.2 / browser 0.65.13 (recorded 0.63.82)

### Fixes
- Refine synchronization restoration enrollment, queue membership, and recovery state for files that need to be restored locally.

## 1.2.1 / browser 0.65.12 (recorded 0.63.82)

### Fixes
- Let synchronization restore a missing local destination instead of failing before it can download a replacement.
- Keep duplicate replacement state and synchronization status accurate when a file is restored in place.

## 1.2.0 / browser 0.65.11 (recorded 0.63.82)

### DASH and media capture
- Add recorded and live DASH handling for BaseURL, SegmentList, SegmentTemplate, and index-derived segment addressing.
- Improve DASH period windowing, timing offsets, finalized-segment draining, and recovery across changing manifests.
- Add MP4 WebVTT subtitle fragment parsing and sidecar caption assembly for supported adaptive downloads.
- Preserve media request identity and revalidate browser receipts before handing a capture to the desktop app.

### Desktop experience
- Refine adaptive-download metadata, admission limits, queue and scheduler roles, toolbar presentation, and local-size synchronization.

## 1.1.0 / browser 0.64.92 (recorded 0.63.63)

- DASH address refresh now treats declared duration as display metadata, allowing older captures without that field to refresh. Accepted refreshes replace or clear the displayed length transactionally while retaining segment identity checks. Runtime checks deferred.

- Recorded DASH captures now retain a valid declared presentation duration for the download list when byte size is unknown. The value remains media length, not a file-size estimate. Chromium/Firefox 0.63.56; runtime checks deferred.

- Advanced per-protocol proxy settings now expose Automatic, Negotiate, NTLM, Digest and Basic for explicit HTTP/HTTPS logins. Forced choices do not fall back to another method; existing profiles remain Automatic and browser routes inherit only matching proxy settings. Runtime/domain checks deferred.

- HTTPS form downloads can now negotiate explicit proxy login after a confirmed CONNECT rejection before any upload. Plain HTTP POST and ambiguous failures remain excluded from this retry; domain/runtime checks are deferred.

- IDM export now rejects separately stored form bodies instead of silently emitting an empty POST, and rejects request text that cannot round-trip as UTF-8. Export format descriptions now explain URL-only output and large-form catalog limitations. Runtime checks deferred.

- Combined total and per-file speed limits now reserve both allowances before waiting for the later deadline, instead of adding both delays. Short reservations and live-setting refresh remain; runtime throughput checks are deferred.

- Regular download pacing now re-reads live speed settings between short bandwidth reservations, reducing stale waits after raising or disabling a limiter. Shared total/per-file accounting and quota charging are retained; runtime timing checks are deferred.

- Explicit proxy logins now attempt Negotiate for GET/HEAD after a terminal Negotiate-only proxy challenge. Existing Basic/Digest/NTLM preference and POST behavior are preserved. Domain authentication and runtime checks remain deferred.

- Browser import now preserves explicit choices per row during metadata lookup, keeps unrelated rows eligible for automatic file selection, and submits in displayed order. Late lookup failures no longer overwrite submission progress. Extension 0.63.55; runtime tests deferred.

- Browser import properties now support per-file Referer overrides, including omission, while retaining the original source-page record. A desktop capability gate prevents silent fallback; custom request settings suppress stale browser size metadata. Extension 0.63.54. Runtime tests deferred.

- Browser handoff now shares one fresh desktop capability request across proxy, import properties, POST and session checks, removing up to three redundant bridge round trips per link. No capability cache survives into the next handoff. Extension 0.63.53; runtime timing tests deferred.

- Browser batch review now reports acknowledged handoff progress and retains bounded submission receipts across page reloads or worker interruptions. Active batches cannot be evicted; interrupted lists remain single-use. Extension 0.63.52. Runtime tests deferred.

- Browser link review can now submit all 500 collected links instead of disabling submission above 100. Chromium and Firefox 0.63.51 retain sequential handoff and single-use submission guards. Runtime tests deferred.

- Native import File Properties now displays live type, lookup status and exact reported byte size. Unsaved address or request changes suppress stale metadata until saved and refreshed. Runtime tests deferred.

- Native import properties now offer per-file login and Referer overrides. Protected overrides follow each original row into metadata requests and download admission; shared credentials remain the default. Runtime tests deferred.

- Native import Edit now follows the reference single-highlight rule and updates immediately when highlighting changes, avoiding ambiguous edits to the first of several highlighted rows. Runtime tests deferred.

- Native import metadata now clears the default selection for HTML pages. Explicit checkbox and Check all/Uncheck all choices survive late metadata, filtering and authorization refreshes. Runtime tests remain deferred.

- Batch URLs now replace every asterisk with the same original number or letter, matching the retained IDM generator evidence. Previews and generated rows share this behavior; runtime tests are deferred.

- Native import review now shows sortable Folder override and Description columns, preserving per-file values through filtering and editing. Blank folder overrides inherit the shared destination policy. Runtime testing remains deferred.

- Delivery builds now offer a production-only target set, copy all native dependency notices, and package isolated release/browser directories. Installer version checks now include COM automation; the MIME-data notice is shipped with setup. Runtime qualification remains deferred.

- Native metadata previews now resolve final-response URL filenames and apply specific MIME suffixes to generic names or URL-only script endpoints, sharing the extension's licensed MIME data. Specific names and the extension-correction preference retain precedence. Runtime tests deferred.

- Native imports add Hide HTML files and Hide repeated files. Hidden selections are preserved and counted; metadata-driven filtering and per-file edits retain original row identity. Runtime tests deferred.

- Batch output patterns now use server filename extensions from metadata. The first non-HTML extension updates the shared pattern once; mixed files retain per-row extensions and explicit per-file edits remain authoritative. Runtime tests deferred.

- Native imports now preview server-reported size, MIME type and server filename suggestions in the background. Four bounded metadata workers keep the dialog responsive; URL/authorization changes cancel stale lookup generations. Size and Type columns sort with unknown values last. Runtime tests deferred.

- Generated number/letter batches now offer a live output filename pattern. Every asterisk uses the row's original sequence token, even after sorting; per-file names override the pattern. Selected names are validated before admission. Runtime tests deferred.

- Native batch generation now accepts descending numbers and letters, matching the retained IDM sequence-generation evidence. Zero padding, inclusive endpoints and the 1,000-URL numeric limit are preserved. Runtime tests deferred.

- Native batch/text imports now sort by File name and Address with direction arrows, preserving per-link properties, checkboxes and already-added tracking across reordering. Admission follows the displayed order. Runtime tests deferred.

- Explicit import categories now survive synchronization replacements and UDM catalog export/import. Missing categories fall back to the current profile's category rules, and malformed category-choice markers are rejected before catalog publication. Runtime tests deferred.

- Browser integration 0.63.50 sends the selected-links page popup through import review before desktop submission. Explicitly checked links remain checked during metadata inspection, with their source frame and link text retained. Context-menu selected-link imports now preserve their initial selection too. Runtime tests deferred.

- Native import now offers each file's category folder, one category for all files, or one folder for all files. Explicit category choices survive content-based filename correction, redownload and duplicate restart; per-link folder overrides remain available. Build/source checks only, runtime tests deferred.

- Browser integration 0.63.49 adds the import options Use link texts as descriptions and Hide images on this page, backed by the retained IDM dialog-228 controls. Collection includes page image resources and retains original source-frame context. Explicit descriptions take precedence over link text. Source checks only; runtime tests deferred.

- Native batch/text import adds an Edit command for each unimported link's address, output filename, destination folder and description. Per-link folders override the shared import folder; existing duplicates retain their saved metadata. Compilation/source checks only, with interactive and regression testing deferred.

- Browser integration 0.63.48 adds explicit per-link HTTP authorization to import properties. Credentials use the existing protected desktop-header path, are not saved with browser batch records, and are cleared from the page after submission finishes. Runtime tests deferred.

- Browser integration 0.63.47 adds sortable import columns, MIME-aware file selection, selected-byte totals and hidden-selection counts. Metadata updates preserve manual selection; sorted imports retain their displayed order. Unknown sizes remain separate from known-byte totals. Source checks only; runtime tests deferred.

- Browser integration 0.63.46 adds background size/type inspection and Hide HTML files to link import. It reuses observed response metadata or sends bounded HEAD requests in the source frame, honors cookie-sharing settings, and leaves unavailable or encoded sizes unknown. HEAD requests do not become download or media captures. Runtime tests remain deferred.

- Browser integration 0.63.45 adds per-link save folders and descriptions under More properties. The desktop validates custom destinations and advertises support; older installations reject these edits rather than silently ignoring them. Existing duplicate records retain their saved metadata. Build/source checks only, with runtime testing deferred.

- Browser integration 0.63.44 adds editable output filenames to the link-import page and defaults imported links to Download later. Empty names retain server naming, invalid Windows names are rejected before handoff, and submitting locks the selection against duplicate sends. Source checks only; not installed or browser-tested.

- Browser integration 0.63.43 shows an OFF toolbar badge and an explanatory tooltip when capture is disabled on a tab. Re-enabling restores the shared error badge; tab activation, navigation and worker startup refresh the indicator without delaying handoff. Source checks only; not installed or browser-tested.

- Unattended queue downloads no longer open automatic completion or login dialogs. Authentication failures remain available for manual recovery; individual resumes and explicitly opened completed files retain their interactive behavior.


- Scheduler now uses native time controls with seconds, long-date formatting, reference keyboard shortcuts and corrected tab order. Daily, one-time and independent stop boundaries retain second precision.
- Queue lists show Time left and refresh filename, size and progress while open. Membership and order updates preserve surviving selections and the top visible file; drag operations are not interrupted by a refresh.
- Queue menus show named Start/Stop actions. Idle queues disable Stop in the menu and Scheduler, while active or waiting runs remain stoppable.
- Sounds adds independently configurable Queue started and Queue stopped/finished events. Additional files and retries within a run do not repeat the start notification.
- Scheduler Apply reflects edited values. Unchanged close/navigation avoids writes; edited saves merge changed fields with current queue data, and new queue creation rechecks current names to avoid replacing a concurrently created queue.
- Media confirmation timing excludes the user's wait before Start, and interrupted media records clear stale completion phases and final-size markers while retaining reusable cached tracks.

The Scheduler save-state candidate includes the Scheduler, queue-menu, sound and
media timing changes above. Its installation was verified on October 27;
interactive acceptance is deferred. The newer unattended-dialog build is staged
but not installed. Extension 0.63.45 and its matching import-properties desktop build are a newer staged batch, also not installed. Build and source hashes for the installed candidate are recorded in
[the candidate record](docs/validation/scheduler-save-state-build-20261027.json).


- Updater hardening after review: the update prompt's button actions no longer call through a closure that had gone out of scope; Help > Check for Update answers while a startup check is still running; a failed check is not recorded as done; the update job is marked in its data, so a setup finished after a restart or offered again still meets the gate instead of downloading a second copy; the setup is copied to a private folder under `%LOCALAPPDATA%\UDM\updates` and verified there, off the UI thread; release asset paths must be exactly tag/file; the prompt shows the window when UDM started in the tray; the manifest tools resolve relative paths against the PowerShell location.
- Code signing: Azure Trusted Signing is a third credential for `package.ps1` and `installer/sign.ps1` (`-TrustedSigningMetadata`, optional `-TrustedSigningDlib`): signtool signs through Microsoft's dlib, the timestamp defaults to Microsoft's authority, and signatures are verified by a valid chain from the "Microsoft ID Verified" CAs since the certificate changes per signing. `docs/release-signing.md` lists the setup for this and for a CA-issued certificate step by step. `build-setup.ps1` no longer loses `-SignToolPath` and `-NoTimestamp` when it dot-sources `sign.ps1`.
- Updater: UDM checks a signed `latest.json` on the release host at startup (at most once per 20 hours, with an opt-out and a per-version skip) and from Help > Check for Update, verifies it with the ECDSA P-256 key built in, downloads the setup through its own transfer with the manifest's SHA-256 as the expected checksum, and starts it only when it also carries a trusted Authenticode signature. `tools/update-key.ps1` creates the key and writes the public half into the build; `tools/publish-manifest.ps1` signs the manifest for a release (`docs/release-signing.md`). Staged POST bodies no longer expire after 3.6 seconds (the hour was counted in milliseconds).
- Code signing is wired into packaging: `package.ps1 -SignThumbprint <sha1>` or `-SignPfxPath <file>` signs UDM's executables and the network helper with signtool (SHA-256, RFC 3161 timestamp) and has Inno Setup sign the setup and uninstaller; `-SignDryRun` reports the tool, certificate and files without changing anything. The vendor's WinDivert files are never re-signed. `installer/test-signing.ps1` proves the pipeline with a temporary self-signed certificate (`docs/release-signing.md`).
- Publishing a download to another volume copies it with buffered 1 MiB writes instead of CopyFileW, which copies large files unbuffered and waits on the disk: 384 MiB onto a hard disk now publishes in 2.6 s instead of 3.7 to 7 s, the same as IDM's assembly, and the copy is flushed after publication (`docs/speed-benchmark-2026-10-23.md`).
- The Size column shows a media download's stream length while its byte size is unknown, as IDM does ("2 hour(s) 3 min", "3 min 20 sec", "20 sec"); HLS/DASH captures record the plan's total length, SABR records already carried theirs, and live recordings stay blank.
- Speed-blind tail splits need a half of at least a 64th of the file per connection (256 KiB to 2 MiB) instead of always 256 KiB: on a shared 3 MB/s link a 140 MB file saw 36 tail splits, each a new TLS connection plus wasted in-flight bytes; with the change both public benchmark files download level with IDM within a second per run (`docs/speed-benchmark-2026-10-23.md`).
- Downloads planned with more than one connection negotiate HTTP/2 or HTTP/1.1 rather than HTTP/3: one QUIC connection carried every range and got a single connection's share of a shared link (Alpine over a 4 MB/s link: 143 to 156 s against IDM's 63 to 68 s; 64 s once HTTP/3 was left to single-connection transfers). The record now carries the negotiated protocol, and the benchmark harness takes `--udm-settings` to merge settings into its private catalog.
- View > Toolbar follows IDM's menu: Customize..., Large/Small Buttons, Look for new toolbars (opens the toolbar folder), Neon and a new 3D Style look that draws UDM's own icons on raised tiles; Tip of the Day gains IDM's picture panel and "Did you know..." heading; the Batch download dialog is verified control by control against IDM's capture at 125 % (`ToolbarUiTests`, `BatchDialogUiTests`).
- Each connection issues its next 256 KiB read before writing the previous buffer, so a single connection no longer alternates network and disk: the sequential loopback case went from 3.6 s to 1.4 to 2.3 s against IDM's 2.0 to 3.4 s (`docs/speed-benchmark-2026-10-23.md`).
- Downloads complete the moment the last byte is written: the trailing hash thread stops when the workers return, the sparse attribute is cleared after publication (2 to 12 s before on a full or mechanical disk), and when no expected checksum must be verified the file is published first and its SHA-256 recorded a moment later. A kept-alive connection the server closed is retried once at once instead of being treated as an outage, and files are planned as three 4 to 32 MiB chunks per connection instead of 8 MiB chunks. Against IDM 6.43 on the same loopback origin UDM now finishes within 0.2 s of its last byte on every case and is faster on five of seven (`docs/speed-benchmark-2026-10-23.md`).
- Content recognition grows by 20 signatures written from public specifications and checked against the libmagic test corpus (`docs/content-signatures-libmagic-2026-10-23.md`): PDF behind a UTF-8 byte order mark and SVG documents are no longer taken for plain text (which the web-page stop treats as an error page), plus animated PNG, Zstandard legacy frames, Hangul Word Processor (3.x, 5.x, HWPX), DSDIFF, FIT, GEDCOM, Group Policy .pol, UF2, binary G-code, Android vdex, xclbin, Xilinx .bit, Keyman .kmx, Tarantool logs, binary OpenPGP keys and ext2/3/4 images. `Udm.NativeTests.exe --sniff-corpus <folder>` checks the whole corpus (`tools/fetch-libmagic-corpus.py`).
- Explorer integration: the installer registers a "Download with UDM" verb on Internet Shortcut (.url) files; it runs `UDM.exe --shortcut "<file>"`, which reads the shortcut's address (web and FTP only) and adds it like `--add`. The development installer registers the same verb for the current user (`tests/explorer-verb.test.py`, launch checks in the native suite).
- Capture POST bodies above 4 MiB (up to 64 MiB) from the browser: the extension streams the bytes in ordered base64 chunks over the existing native host pipe (`post-body-begin` / `post-body-chunk` / `post-body-finish`), the desktop verifies size and SHA-256, seals the body DPAPI-protected under `requests/` outside the catalog, and the download record keeps a small reference. Bytes are never treated as text, so binary and non-UTF-8 producers round-trip exactly (loopback check with every byte value). Desktops without this support keep such downloads in the browser without pausing them.
- Draw UDM's own 40x40 page icons in the Configuration dialog (browser window, document, folder, tray, nodes, shield, key, modem, speaker) in place of the Windows stock icons; they follow the light and dark theme and scale with DPI (`OptionsTabsUiTests`).
- Rebuild the Site Grabber as IDM's five-step wizard: "UDM Site Grabber. Step N --- Set start page / Save files to / Set explorer filters / Set file filters / Explore and download" on IDM's 717x475 shell with a Project/Options/Actions/Help menu bar, the left picture panel and Back/Next/Close at IDM's positions; step 1 follows IDM's captured layout with the Advanced >> manual-login area, and the exploration result uses IDM's 905x639 result layout with its actions in the Actions menu (`GrabberWizardUiTests`). Dialogs with a menu bar now keep their full client size.
- Match IDM's About dialog (461x503 layout, build date, licensing group, legal notice, Home Site and Support Team lines), its two-line Download complete text with the file type icon, and a Download limits node in the Scheduler tree that edits the shared download quota. tools/idm-research/dump-dialogs.py dumps RT_DIALOG templates from a PE for such comparisons.

- Take the disk write of each received block out of the catalog lock (eight connections no longer queue behind one write, and the window no longer waits on them), probe every 500 ms for 20 s after a connection drops on a download that was receiving so a short outage ends the moment the network is back (94 ms in the new fixture check, against a 1, 2, 4 s backoff), size each read to at most a quarter second of the speed limit when one applies (so limited downloads pace smoothly instead of stalling after one read), and write the periodic catalog save outside the manager lock ([notes](docs/speed-benchmark-2026-10-23.md)).
- Benchmark UDM against IDM 6.43 on identical loopback and public downloads (`benchmarks/speed`, [results](docs/speed-benchmark-2026-10-18.md)). Fix the shortfalls it found: a connection that breaks after delivering data is reopened at once and a megabyte of progress restores the retry budget, so servers that reset connections or short outages no longer fail the download; a worker whose connection is refused while others are receiving hands its range to them instead of failing the download; parallel ranges use one HTTP/1.1 connection each instead of sharing one HTTP/2 connection (2.3 times faster on a CDN); 256 KiB reads; the assembled file is hashed while it is written.
- Write every HTTP range in place into one preallocated output file instead of one part file per range, hash the file while it downloads (ranges are claimed front to back in 4 to 8 MiB chunks) and publish it by rename. The finish phase after the last byte of a 384 MiB file drops from 2.6 to 3.8 s to 0.1 to 0.9 s; existing part files are folded into the single file on the next start. The resume offset after a crash is the last saved catalog (saved every 1.5 s while downloading).
- Negotiate HTTP/3 where Windows and the server support it (Options › Connection, "Use HTTP/3 when supported by Windows and the server", on by default). A transfer that fails during an HTTP/3 exchange retries once without HTTP/3 before the existing HTTP/2 to HTTP/1.1 step. Explicit proxy routes through the bundled curl stay on HTTP/2.
- Recognize many more file formats from their content: disk images, fonts, e-books, raw camera and texture images, lossless and tracker audio, broadcast video, data and capture files, further app packages inside ZIP, and the producer of compound documents (MSI, Word, Excel, PowerPoint, Outlook, Visio). A download without a usable extension gains the right one for these formats, and a web page or text notice served under an image, font, disk-image or other binary-only name now stops the download like one served under an archive name. See [backend gap closing](docs/idm-backend-gap-closing-2026-10-16.md).

- Save a captured rendered starting page in Site Grabber offline ZIP archives, preserving runtime-created HTML and rewriting downloaded assets and child-page links. Negotiate HTML capture with the running desktop, protect stored captures, and report oversized pages explicitly. Browser integration 0.63.30.

- Clear stale download-list tooltip text when its row disappears after filtering or a list update.

- Preserve the rendered-page capture identity in Site Grabber checkpoints when browser cookies change during exploration. Persist crawler-managed cookie updates with the capture association, restoring both on save failure; manual session and filter changes still require recapture.

- Preserve a verified receipt for fully received ordinary-file prefetch. Download Later can resume from saved parts without spending a one-use URL again; missing or modified saved bytes are rejected before publication.

- Keep ordinary-file prefetch running when Start is clicked with unchanged login details, including filename, folder and drive changes. Assemble verified parts beside the confirmed destination without another HTTP request; invalid destinations and failed confirmation saves keep publication gated.


- Show server-provided filenames in Download File Info with an explicit Use server name action. Preserve edited names and selected folders, keep changes as a draft until confirmation, and reject replacement or conflicting destinations. Metadata lookup uses HEAD without consuming a single-use download link; with prefetch enabled, the suggestion comes from the existing transfer response without another request.

- Decode legacy ISO-8859-1 server filenames in Site Grabber and original-response capture. Extended filenames take precedence over plain fallback names; malformed escapes, embedded NULs and unsupported encodings retain the fallback.
- Validate Scheduler numeric fields completely before saving, rejecting partial numbers, decimals, signs and out-of-range values while preserving blank connection retries as the Options default.

## 1.0.4 / browser 0.64.49 (recorded 0.63.20)

- [Release notes](docs/release-v1.0.4.md).
- Extend browser proxy handling to evaluate inline PAC scripts with compatible helper functions, deterministic route selection, cancellation, and bounded execution. Browser and desktop keep the selected proxy route consistent through native handoff.
- Harden browser/native handshakes and proxy diagnostics, including safe request context handling and improved validation for Chromium and Firefox integration.
- Improve desktop list behavior: clarify columns, preserve stream sorting, and refresh visible metadata predictably.
- Recognize Unicode and compound file-name patterns more reliably during content inspection.

## 1.0.3 / browser 0.64.46 (recorded 0.63.17)

- [Release notes](docs/release-v1.0.3.md).
- Allow a browser-captured media download to be confirmed while its tracks are still being fetched. A folder or file-name change is saved transactionally, keeps the existing requests alive, stages assembly in its original working location, and publishes to the confirmed destination.
- Show the verified final media size in the main download list while the confirmation dialog is open, including sorting by that value, without re-entering dialog or capture presentation handling.
- Preserve the first video or audio transfer error when the companion track is cancelled, so the user sees the meaningful HTTP or transport failure rather than a secondary cancellation message.

## 1.0.2 / browser 0.64.45 (recorded 0.63.16)

- [Release notes](docs/release-v1.0.2.md).
- Measure the actual final media container before the download confirmation dialog opens, including audio-only output. The dialog reports a verified size rather than an estimate.
- Let Start publish the already-verified staged media output without re-downloading or remuxing it. Folder, file name, category, description, and queue can still be changed at confirmation time; transfer-affecting changes remain protected.
- Make media publication recoverable: preserve the verified source and a small journal if saving completion state is interrupted, then reconcile the completed download safely on the next recovery pass. Cross-volume copies verify bytes before publication and honor cancellation.
- Improve browser handoff timing and make capture registration resilient when several downloads are captured at once.

## 1.0.1 / browser 0.64.43 (recorded 0.63.8)

- [Release notes](docs/release-v1.0.1.md).
- Build the bundled curl library with Windows SSPI and NTLM enabled, so configured proxy credentials can complete NTLM authentication and the existing Windows-login path remains available for intranet routes.
- Qualify proxy auto-configuration scripts with a loopback PAC fixture, including ordered alternatives, DIRECT and SOCKS results, fallback, and unreachable-route behavior.
- Show exact media sizes only when the selected stream or container reports a trusted content length. Browser and native handoff now preserve that reported size for video and audio selections without substituting a bitrate estimate.
- Improve media preview responsiveness and receipt error recovery while keeping stale, foreign, expired, or untrusted browser captures out of a download handoff.

## 1.0.0 / browser 0.64.37 (recorded 0.63.8)

- [Release notes](docs/release-v1.0.0.md).
- Promote the content-recognition and browser/native integration work from 0.92.0 to the first 1.x release.
- Check the downloaded content before publishing. A web page or text notice served in place of an archive, installer, document, or media file now stops the download with its data kept, and queues do not retry it automatically. Resuming such a download asks whether to save the received content anyway. When web page text is found in front of an otherwise recognized file, the download pauses and resuming asks whether to remove that text or keep the data as received.
- Add a missing or script-style extension (for example `download.php`) from the recognized content before publication. Explicit extensions are never changed. Both checks can be turned off in Options › File Types.
- Check the first bytes of a download while it is still running, so a web page served for a file stops after about 5 KB instead of after the whole body.
- Retry once with HTTP/1.1 when an HTTP/2 transfer fails mid-exchange (stream reset, GOAWAY, transport error); the download keeps HTTP/1.1 afterwards. See [backend gap closing](docs/idm-backend-gap-closing-2026-10-16.md).
- Support Windows login (Negotiate/NTLM) for servers and proxies. Saved site logins are offered for any challenge scheme, intranet servers can receive the current Windows login (Options › Logins, on by default), and a configured proxy login answers Basic, Digest and NTLM challenges (the vendored curl is now built with Windows SSPI), while a proxy asking for Windows login without a configured login receives the current user's login on intranet routes. A loopback authenticating proxy covers Basic, Digest and Negotiate in the native suite. A loopback proxy auto-configuration script qualifies PAC route order, DIRECT and SOCKS directives, failover to the next alternative and the absence of a direct fallback when scripts or proxies are unreachable.
- Reattach a new desktop process to a running network helper. The desktop records its helper; an active helper whose desktop exits waits up to two minutes for a same-user replacement desktop, and a new desktop adopts it instead of elevating another helper, keeping capture objects and response writers.
- Site capture rules with the D flag now also apply to plain addresses routed through an HTTPS proxy, matching the recovered IDM semantics (an https address or a TLS proxy route). Media captured from the browser video panel drops a detected web page prefix automatically, as IDM does for its video downloads; other downloads keep the prompt.
- Qualify FTP against a loopback server (EPSV/PASV, EPRT/PORT, REST resume, SIZE/MDTM, logins, HTTP CONNECT) and add FTP gateway proxy types (USER user@host, SITE host, OPEN host; Options › Proxy). Active FTP works through a gateway; CONNECT and SOCKS routes still require passive mode.
- Add Windows download automation: `UDM.LinkTransmitter` (COM, per-user registration by the installer or `Udm.Automation.exe --register`) with `SendLinkToUDM`, `SendLinkToUDM2` and `SendLinksArray`. Single links follow flags 1 (silent) and 2 (queue only); link arrays are stored encrypted and reviewed in a "Download all links with UDM" dialog. A running UDM hosts the class object itself, so callers reach it without any registry lookup; the installer registers `Udm.Automation.exe` machine-wide for cold starts, and that server starts the desktop when it is not running.
- Site capture rules now also act on responses: V/A/C stream rules place a matching reply on the video panel with the rule's media type, S rules hand matching background requests (XHR, media, objects) to UDM, and the diagnostics page lists the last 50 rule decisions (host, path shape and query names only) with a "Save report as file" button.
- Add site capture rules (Options › File Types › Site rules). One rule per line, `[FLAGS][#EXT]!PATTERN`, decides before the file-type list whether a browser download is ignored, captured outright, or captured through a file name or extension taken from the address. No rules ship by default.

## Unreleased

- Delivery builds now offer a production-only target set, copy all native dependency notices, and package isolated release/browser directories. Installer version checks now include COM automation; the MIME-data notice is shipped with setup. Runtime qualification remains deferred.

- Native metadata previews now resolve final-response URL filenames and apply specific MIME suffixes to generic names or URL-only script endpoints, sharing the extension's licensed MIME data. Specific names and the extension-correction preference retain precedence. Runtime tests deferred.

- Native imports add Hide HTML files and Hide repeated files. Hidden selections are preserved and counted; metadata-driven filtering and per-file edits retain original row identity. Runtime tests deferred.

- Batch output patterns now use server filename extensions from metadata. The first non-HTML extension updates the shared pattern once; mixed files retain per-row extensions and explicit per-file edits remain authoritative. Runtime tests deferred.

- Native imports now preview server-reported size, MIME type and server filename suggestions in the background. Four bounded metadata workers keep the dialog responsive; URL/authorization changes cancel stale lookup generations. Size and Type columns sort with unknown values last. Runtime tests deferred.

- Generated number/letter batches now offer a live output filename pattern. Every asterisk uses the row's original sequence token, even after sorting; per-file names override the pattern. Selected names are validated before admission. Runtime tests deferred.

- Native batch generation now accepts descending numbers and letters, matching the retained IDM sequence-generation evidence. Zero padding, inclusive endpoints and the 1,000-URL numeric limit are preserved. Runtime tests deferred.

- Native batch/text imports now sort by File name and Address with direction arrows, preserving per-link properties, checkboxes and already-added tracking across reordering. Admission follows the displayed order. Runtime tests deferred.

## Native 0.87.0 / browser 0.64.31 (recorded 0.63.8)

- [Detailed changes since 0.85.0 RC3](docs/changelog-rc3-to-087.md) and [release notes and limits](docs/release-v0.87.0.md).
- Capture rendered Site Grabber links through Edge and native messaging with document identity, protected snapshots, durable receipts, and lost-reply reconciliation.
- Preserve original browser responses through durable native admission and recover interrupted captures without creating a second request.
- Improve Chromium proxy bypasses, mapped-address native transport, media/caption recovery, and isolated installer repair/uninstall behavior.
- Advance desktop and installer versions to 0.87.0 and browser integration to 0.63.8, including safer same-document media navigation.

### Native 0.85.0 RC1 / browser 0.64.21 (recorded 0.63.0)

- [Changes since 0.84.0 RC3](docs/changelog-rc3-to-085-rc1.md) and [RC1 package verification](docs/release-v0.85.0-rc.1.md).

- Retain the first full HTTP response for single-use download links while preserving validator checks on recovery. Controlled live cases passed 8/8 and the native suite passed 2,589/2,589.
- Support Digest authentication for explicit HTTP proxies and keep proxy challenges separate from origin authentication.
- Advance native, browser, and installer version declarations for the RC1 package.

## Native 0.84.80 RC3 (recorded desktop 0.84.0) / browser 0.63.34 (recorded 0.62.13)

- [Changes since RC2](docs/changelog-rc2-to-rc3.md) and [RC3 package verification](docs/release-v0.84.0-rc.3.md).
- Keep HLS captions synchronized across distant timestamp anchors and preserve valid empty subtitle tracks.
- Support larger recorded HLS/DASH selections and durable per-segment adaptive checkpoints.
- Preserve pipelined and body-framed HTTP requests with their original client during network takeover decisions.
- Expose the Properties source link to Windows accessibility.

## Native 0.84.79 RC2 (recorded desktop 0.84.0) / browser 0.63.33 (recorded 0.62.13)

- [Changes since RC1](docs/changelog-rc1-to-20261008-push.md) and [RC2 package verification](docs/release-v0.84.0-rc.2.md). This prerelease is a source and installer publication; the installer has not been executed or installed on the personal machine.

- Honor Windows static-proxy bypasses and configured PAC URLs for FTP; unresolved WPAD and unsupported static proxy routes remain fail-closed.
- Expand captured browser POST bodies to 4 MiB with an 8 MiB native request frame; preserve exact-byte and over-limit rejection checks.
- Reject ranged HTTP responses that omit the validator established by the download probe.

- Wait for a saved completion before executing CLI /q exit or /h dial-up hang-up; a failed catalog save retains both pending actions for retry.

- License UDM's original code and documentation under Apache 2.0; retain third-party notices and include the project license/notices in future packages.
- Version progress-dialog reference metadata and sanitized validation/release records, retaining the full-suite failure and its passing focused rerun.

- Make individual Pause transactional: a failed catalog save preserves the job, capture prompt, scheduler state, and running transfer. Cancel the worker only after the pause decision is saved.
- Avoid catalog writes when stopping an idle completed download while retaining pending-synchronization cancellation and scanner-wait handling.

- Keep the completion dialog drag area icon-only, retain its Unicode filename as the control name, and support icon rendering in window previews.
- Add a completion-dialog test runner and protect its optional warning observer flag across threads.

## Native 0.84.71 RC1 (recorded desktop 0.84.0) / browser 0.63.21 (recorded 0.62.8)

- Consolidate queue retry, synchronization, recovery, backup and restore behavior with focused backend checks.
- Add `/a` queue-only downloads and `/s` queue start, including forwarding to an existing application instance.
- Extend browser capture recovery, multipart handling, file recognition and HLS/DASH media selection.
- Improve native menus, properties, progress controls, keyboard behavior and accessibility.
- Add installer migration, ownership and transaction safeguards, plus pinned HTTP/2 transport dependencies.
- See [candidate acceptance and delivery status](docs/current-staged-candidate.md) for the exact tested, packaged and installed revisions. This entry records source changes; it does not announce a qualified release.

## Native 0.78.4 (recorded desktop 0.78.0) / browser 0.53.40 (recorded 0.53.1) — video preparation deadlines

- Stop stalled cross-site video preparation before the panel timeout and prevent late native submission.
- Guard direct-media handoff after asynchronous settings/credential work; cancel HLS/DASH preparation when expired.
- Keep unknown native acceptance distinct from a request never sent, without automatic retry.
- [434 targeted and 35 actual Edge checks, decoded outputs and explicit limits](docs/parity-browser-0.53.1.md).

## Native 0.78.3 (recorded desktop 0.78.0) / browser 0.53.39 (recorded 0.53.0) — Firefox handoff reliability

- Keep Firefox's original response running until UDM has durably prepared the request.
- Preserve refused/unavailable downloads without pause/resume or a second request.
- Recover prepared/committed ownership after lost acknowledgements and actual background reloads.
- Preserve browser completion and user pause/cancel decisions during preparation.
- [386 targeted and 47 live checks, baseline failures and remaining limits](docs/parity-browser-0.53.0.md).
- Native 0.78 and earlier browser changes: [release](docs/parity-0.78.0.md), [manual Firefox recovery](docs/parity-browser-0.52.2.md), [current gap register](docs/idm-research-current-status-2026-09-29.md).

## 0.74.0 / browser 0.47.39 (recorded 0.47.1) — repeated-cancel exclusions

- Offer site or literal-address exclusions after two consecutive cancelled automatic captures.
- Add suppression/re-enable, File Types Add/Delete, replay protection and transactional settings updates.
- Preserve literal asterisks, long exact URLs and IPv6 in both extension bundles.
- Preserve multipart.js and ordered dependencies when regenerating Firefox.
- Correct network-helper signature wording.
- [2,821 checks and qualification limits](docs/parity-0.74.0.md).

## 0.73.0 / browser 0.47.38 (recorded 0.47.0) — queue startup and export scopes

- Persist per-queue Start download on UDM startup independently of the current Stop state.
- Prepare opted-in queues once at launch, with rollback if saving fails.
- Add all, selected and named/all queue export scopes with retained per-file checkboxes.
- Reject stale/duplicate export records and empty export sets before writing.
- Reject overlong instance tags instead of silently connecting to the main app.
- [1,938 checks and qualification limits](docs/parity-0.73.0.md).

## 0.72.0 / browser 0.47.37 (recorded 0.47.0) — native toolbar customization

- Replace individual toolbar buttons with a native toolbar and queue dropdowns.
- Add Available/Current command editing, repeated separators, Move Up/Down and Reset.
- Migrate existing preferences and save customized command layouts.
- Support bounded .tbi skin strips, normal/hot/disabled states and high-DPI image variants.
- Preserve UDM artwork and fall back safely if a saved skin is missing.
- [1,917 passing checks, development corrections and remaining GUI limits](docs/parity-0.72.0.md).

## 0.71.0 / browser 0.47.36 (recorded 0.47.0) — remote ZIP preview

- Add Preview to Download File Info for ordinary ZIP links before download.
- Read bounded HTTP/HTTPS/FTP ZIP and ZIP64 directories using existing login, browser sessions and proxy routes.
- Handle small no-range responses, cancellation, timeouts and changed archive responses.
- Show names, sizes and encryption flags in a virtual native list; retain local preview.
- Correct toolbar/dial-up scope in the comparison inventory.
- [1,887 passing checks, initial failures and limits](docs/parity-0.71.0.md).

## 0.70.0 / browser 0.47.35 (recorded 0.47.0) — offline website folders

- Enable original subfolders for offline ZIP projects with an explicit destination folder.
- Preserve nested pages and stylesheets, rewrite relative links, retain a root entry page, and separate colliding/query/external resource names.
- Reuse redirected resources, validate archive names and content, and retain verified cache reuse on pause/resume.
- Correct the About dialog's stale version label.
- [1,848 passing checks and remaining limitations](docs/parity-0.70.0.md).

## Browser 0.47.34 (recorded 0.47.0) / native 0.69.2 (recorded desktop 0.69.0) — multipart text-form downloads

- Preserve browser-generated multipart text fields, original order, repeated names, encoding and submitter overrides.
- Match the ephemeral submission snapshot to the observed request and retain verified same-origin POST redirects.
- Keep unsupported file/Blob or oversized bodies in the browser.
- [846 checks and actual Edge byte comparisons](docs/parity-browser-0.47.0.md).

## Browser 0.46.34 (recorded 0.46.0) / native 0.69.1 (recorded desktop 0.69.0) — responsive video formats

- Bind panel format reads to the browser-issued document identity, avoiding redundant tab lookups and document-idle waits.
- Preserve strict current-tab checks for download actions and legacy browser paths.
- Complete a public Edge 360p video pause, link refresh, resume and full decode on one history record.
- [807 current checks, measured results and limits](docs/parity-browser-0.46.0.md).

## 0.69.0 / browser 0.45.34 (recorded 0.45.0) — direct video recovery

- Refresh an unfinished direct video/audio pair from the browser panel on its existing record.
- Validate partial-track HTTP identity and completed-track integrity before reuse; retain saved data on rejection or cancellation.
- Open the original page automatically when address refresh begins.
- Check middle and final playback bytes as well as the first byte before accepting a native player pair.
- [1,785 passing checks, public-test failures and remaining limitations](docs/parity-0.69.0.md).

## v0.68.0 — combined 0.58 through 0.68 update (28 September 2026)

This published release includes the previously untagged 0.58.0 through 0.68.0 milestones. The v0.68.0 tag is the complete source snapshot; the entries below preserve the recorded detail for every included version.

- **0.58–0.64:** expanded Site Grabber destinations, filters, link handling, website workspaces, browser sign-in, login persistence, and session updates.
- **0.65–0.67:** signed-in browser downloads, Edge proxy handoff, encrypted proxy/session handling, and ordered proxy alternatives.
- **0.68:** refresh an interrupted recorded HLS or static DASH session from a browser capture while preserving the original download, history, and verified cache.

See [0.58](docs/parity-0.58.0.md), [0.59](docs/parity-0.59.0.md), [0.60](docs/parity-0.60.0.md), [0.61](docs/parity-0.61.0.md), [0.62](docs/parity-0.62.0.md), [0.63](docs/parity-0.63.0.md), [0.64](docs/parity-0.64.0.md), [0.65](docs/parity-0.65.0.md), [0.66](docs/parity-0.66.0.md), [0.67](docs/parity-0.67.0.md), and [0.68](docs/parity-0.68.0.md) for behavior and stated limits.

## 0.68.0 / browser 0.45.33 (recorded 0.45.0) — recorded-video link recovery

- Refresh interrupted HLS/static DASH sessions from the browser panel without creating another download.
- Preserve filename, history and cache; review fresh credentials before applying them.
- Verify retained segment bytes against the fresh source before fetching missing segments; preserve cache on failure, pause or restart.
- Reject changed selections, foreign-origin credentials and incompatible segment layouts.
- [1,739 passing checks and remaining limitations](docs/parity-0.68.0.md).

## 0.67.0 / browser 0.45.32 (recorded 0.45.0) — extended Edge proxy handoff

- Preserve encrypted HTTPS proxies and Edge SOCKS4 local DNS through browser capture and native download.
- Keep proxy credentials separated by encryption protocol; preserve existing native compatibility.
- Preserve ordered PAC alternatives for connection failures without replaying POST or bypassing TLS/authentication errors.
- [2,586 passing checks and remaining limitations](docs/parity-0.67.0.md).

## 0.66.0 / browser 0.44.32 (recorded 0.44.0) — Edge proxy handoff

- Captured fixed/direct Edge routes, per-protocol endpoints, bypass rules, and explicit-link proxy selection.
- Corrected explicit HTTP/SOCKS proxy routing for loopback downloads with a bounded native Schannel transport.
- Added native compatibility checks; retained browser ownership for unresolved PAC routes.
- [2,509 checks and remaining limitations](docs/parity-0.66.0.md).

## 0.65.0 / browser 0.42.32 (recorded 0.42.0) — Signed-in browser downloads

- Managed, exact-origin cookie sessions for ordinary browser file captures with consent, store/partition checks, encrypted updates, duplicate handling and link refresh.
- Removed default-profile cookie guessing and fixed UTF-8 cookie-header budget accounting.
- [Acceptance and remaining limitations](docs/parity-0.65.0.md).

## 0.64.0 / browser integration 0.41.32 (recorded 0.41.0) — Website session updates

- Apply website Set-Cookie responses to signed-in Site Grabber sessions, including redirects, expiry, deletion, domain/path scopes and separate partitioned identities.
- Share updates across a transfer's parallel requests and protect saved sessions with Windows account encryption.
- Retain updated cookies in exploration checkpoints, File Info lookups, queued file transfers, redownload, synchronization and offline ZIP requests. Reject late updates after browser recapture; roll back failed catalog saves.
- Reproduce the previous release's HTTP 401 failure with an actual Chrome sign-in and a server that rotates its session. Verify the replacement-cookie workflow in Chrome, Edge and Firefox.
- Browser source and signed network runtime remain unchanged. Public-site coverage, rendered exploration, cross-job session sharing and full IDM parity remain open.

## 0.63.0 / browser integration 0.41.31 (recorded 0.41.0) — Site Grabber browser sign-in

- Add manual browser sign-in, a login-page field, logout exclusions and an explicit project selector in the extension popup.
- Save scoped browser cookie snapshots with Windows protection; carry them into exploration, metadata, queued files, redownload, synchronization and offline ZIP requests.
- Respect tab stores/containers, matching partitions, cookie paths, Secure and expiration; recalculate scope through redirects.
- Handle stale wizard saves, corrupt sessions, expiring tickets, replay protection and catalog/template credential boundaries.
- 2,381 checks pass, including real Chrome, Edge and Firefox sign-in/popup/native workflows. Rendered crawling, cookie rotation and full IDM parity remain open.

## 0.62.0 / browser integration 0.40.31 (recorded 0.40.0) — Site Grabber login and descriptions

- Add project-specific HTTP Basic/Digest login with Windows-protected persistence, exact-origin request scoping, editable credential recovery and save rollback.
- Carry login details into exploration, parallel metadata checks, queued downloads and offline ZIP archives.
- Extract static link text and image alt text into collected-file properties, a Description column and new download records; retain bounded Unicode descriptions across scan continuation.
- Preserve edited metadata on existing downloads; templates exclude project credentials. Browser integration remains 0.40.0.
- 2,276 checks pass, including 45 new native checks, 40 real Chrome/Edge integration scenarios and 33 offline website checks. Native visual acceptance and full IDM parity remain open.

## Browser integration 0.40.30 (recorded 0.40.0) / native 0.61.0 — Capture lifecycle

- Fix a reproducible late-navigation cleanup race that erased newly detected playlists.
- Retire previous document/frame captures, authenticated request context and SABR sessions while preserving fresh captures; serialize menu-offer storage with cleanup.
- Persist bounded document identity across service-worker restarts and reject delayed work after tab closure.
- Add webNavigation permission; reload the extension and refresh video tabs to activate.
- 687 fresh checks pass, including 25 new lifecycle checks and 20 real extension/native scenarios in each of Chrome and Edge. Native engine and signed network runtime unchanged; full IDM parity remains unestablished.

## 0.61.0 / browser integration 0.39.30 (recorded 0.39.0) — Offline website links

- Add a Complete website preset and conversion of HTML/CSS references to actual downloaded destinations, including original subfolders, numbered filenames, redirects and CSS imports.
- Retain original pages for repeat conversion; preserve external edits and unsupported encodings. Recover interrupted page/catalog publication with verified journals and rollback.
- Support conversion while queues are disabled, Stop/retry, and synchronization using original server lengths. Preserve project association on updated files.
- 2,198 checks pass, including 53 new native checks and 33 real offline-site checks in Chrome/Edge. Native visual acceptance, public-site coverage and full IDM parity remain open.

## 0.60.0 / browser integration 0.39.29 (recorded 0.39.0) — Live Site Grabber workspace

- Download matching files during exploration with bounded concurrency and incremental persistence. Start/stop checked downloads without enabling unrelated queues.
- Add reusable templates, folder/referring-page trees, editable collected filenames, file/folder actions, address copying and live statistics.
- Preserve edits across scan checkpoints, transactionally remap category/template destinations, and stop on persistence failures.
- 2,112 checks pass, including 56 new native checks and 32 Chrome/Edge integration scenarios. Native visual acceptance and full IDM parity remain open.

## 0.59.0 / browser integration 0.39.28 (recorded 0.39.0) — Advanced Site Grabber

- Add separate Explorer and File Filters pages with include/exclude patterns, independent site/path rules, depth, main-domain scope, size bounds and deterministic duplicate hiding.
- Add concurrent metadata lookup, server filenames, result properties, referring-page context, Stop with retained results and saved-scan continuation.
- Use an included MPL-2.0 Public Suffix List snapshot for domain boundaries; preserve legacy saved projects and category/destination behavior.
- 2,056 checks pass, including 69 new native checks and 32 Chrome/Edge integration scenarios. Native visual acceptance, broader video/driver equivalence and full IDM parity remain open.

## 0.58.0 / browser integration 0.39.27 (recorded 0.39.0) — Grabber destinations and reliable batches

- Add category-based, selected-category and explicit-folder destinations, with optional original URL subfolders for collected files.
- Add a dedicated Save to step and per-file destination/added-state previews; retain old project behavior and category choices across category renames/deletion.
- Save each selected batch and project atomically, with rollback on invalid paths or storage failure; reject traversal and redirected subfolders and preserve pause/restart/resume.
- 1,987 checks pass, including real HTTP folder downloads and 32 Chrome/Edge extension/native scenarios. Native visual acceptance and full IDM parity remain open.

## v0.57.0 — combined 0.41 through 0.57 update (28 September 2026)

This published release includes all previously untagged milestones from 0.41.0 through 0.57.0. The v0.57.0 tag is the complete source snapshot; the linked notes retain the recorded details for each included milestone.

- **0.41–0.43:** Firefox proxy-route handoff, indexed DASH resource binding, and selected YouTube audio-only streaming.
- **0.50–0.53:** media-session recovery, download and completion dialogs, configuration/queue controls, plus PAC and transport configuration.
- **0.54–0.56:** browser customization, quota workflows, and safer duplicate/overwrite behavior.
- **0.57:** exposed clear live HLS recording with Stop and save, pause/recovery, and native parallel capture.

See [0.41](docs/parity-0.41.0.md), [0.42](docs/parity-0.42.0.md), [0.43](docs/parity-0.43.0.md), [0.50](docs/parity-0.50.0.md), [0.51](docs/parity-0.51.0.md), [0.52](docs/parity-0.52.0.md), [0.53](docs/parity-0.53.0.md), [0.54](docs/parity-0.54.0.md), [0.55](docs/parity-0.55.0.md), [0.56](docs/parity-0.56.0.md), and [0.57](docs/parity-0.57.0.md) for detailed behavior and limits.

## 0.57.0 / browser integration 0.39.26 (recorded 0.39.0) — live HLS recording

- Capture exposed clear live HLS through native parallel workers and playlist refreshes; Stop and save the captured portion or pause separately.
- Persist encrypted segment receipts, verify resumed media, recover offline after expiration/publication conflicts, and clean only owned cache files after successful publication.
- Respect retry budgets and Retry-After; cancel stalled HTTP promptly; preserve separate-track timing, discontinuities and changed initialization sections.
- 1,461 checks passed, including generated TS/fMP4 media and 32 actual Chrome/Edge extension/native scenarios. Native visual acceptance, broad public-site support and full parity remain open.

## 0.56.0 / browser integration 0.38.26 (recorded 0.38.0) — duplicate overwrite workflows

- Remember numbered-copy, overwrite and Existing choices, with full dialog descriptions and all policies in Options > Downloads.
- Replace completed HTTP/FTP files after verifying new bytes; retain prior versions with accurate metadata and invalidate obsolete scan results for edited copies.
- Restart unfinished duplicates at the original filename with a fresh parts generation, preserved preferences and correct browser/CLI confirmation routing.
- Commit history and preferences together; retain old parts on failure and recover cleanup after a crash without deleting unrelated or changed files.
- 1,185 checks passed: 1,135 native, 16 protocol/app-host, 30 Chrome/Edge and 4 real active/passive FTP scenarios. Eight selected resource bounds match. Native visual acceptance and full parity remain open.

## 0.55.0 / browser integration 0.38.25 (recorded 0.38.0) — quota and download workflows

- Add the download-limit warning preference, native warning dialog, quota countdown and waiting status.
- Handle simultaneous quota reservations, large chunks, cancellation, rollover, restart and malformed/future saved timestamps.
- Open Download complete for existing completed links and resume unfinished duplicates directly; keep automatic completion actions separate from viewing history.
- Preserve new-download confirmation, saved segments, paused records on persistence failure and existing double-click behavior.
- 1,139 checks passed, including 1,095 native checks and real isolated Chrome/Edge downloads. Six selected reference rectangles match. Native visual acceptance and full parity remain open.

## 0.54.0 / browser integration 0.38.24 (recorded 0.38.0) — browser customization update

- Add separate native Keys, Browser Menus and Video Panels dialogs with functional preferences.
- Support Insert/Delete combinations, request-bound held-key capture, Ctrl-force/Alt-bypass defaults and forced web-resource filtering.
- Apply browser-family context-menu switches and video type/minimum-size/site-exception rules; reject stale disabled offers.
- Add optional verified direct-player capture with opt-in, protected/ad/fragment checks and duplicate prevention.
- Fix Edge menu-rebuild and held-key/download-popup races found in real browser testing.
- 1,720 behavioral checks passed, including real isolated Chrome/Edge downloads and 1,040 native checks. 45 static control rectangles match the reference. Native visual acceptance, public-site coverage, full parity and speed equivalence remain unverified.

## 0.53.0 / browser integration 0.37.24 (recorded 0.37.0) — transport configuration update

- Add asynchronous custom PAC scripts, per-protocol configuration, redirect reevaluation and ordered proxy alternatives without POST replay.
- Route passive FTP control/data through PAC-selected HTTP CONNECT and SOCKS; retain precise connection failure messages.
- Add a working TLS 1.3 toggle and an opt-in modification-date override. Preserve certificate, ETag, byte-range, length and refreshed-link identity checks.
- Add reference-position transport controls and clear stale PAC settings from captured browser routes.
- Verified: 1,024 native checks, 24 new transport scenarios, 127 existing live transport regressions, 8 native-host checks, 6 isolated app/host checks and 112 static control bounds. Visual acceptance, full parity and speed equivalence remain unestablished.

## 0.52.0 / browser integration 0.37.23 (recorded 0.37.0) — configuration and queue update

- Rebuild configuration around nine tabs with font-based dialog units, browser lists, category settings, connection exceptions, downloads, proxy routes, logins, dial-up/VPN and sounds.
- Add browser executable enable/disable preferences; the native host derives its actual parent identity. Restart reconnects only UDM native hosts from the active install.
- Add queue selection, optional queue creation/start and persistent prompt preferences for Download Later and batch imports. Queue-only browser capture applies to files and HLS/DASH media.
- Make category extension edits affect routing; preserve folder-memory and type settings across rename/delete and roll back failed persistence.
- Replace the duplicate dropdown with radio choices and compact Add URL with authorization controls. Existing replacement safeguards and HTTPS-only saved logins remain.
- Merge Options drafts without overwriting unrelated live settings and refresh controls after nested editors. Restore global SOCKS selection through Advanced and import Windows proxy routing.
- Verification: 1,008 native checks, 8 host protocol checks, 6 isolated app/host checks and 107 selected resource-bound comparisons. Native visual/click acceptance remains open.

## 0.51.0 / browser integration 0.37.22 (recorded 0.37.0) — download dialog update

- Rebuild File Info, File Properties, Progress, Completion and progress customization with Windows dialog units and Tahoma 8-point measurements. Use native Windows buttons in the light theme.
- Add category creation and recent destination folders to File Info; keep authentication, queue and preview options under More.
- Match completion button order and add the address field. Keep saved-file drag, Open with, folder and suppression actions connected.
- Apply speed-limit edits immediately; preserve remembered limits and reset temporary overrides on resume. Add persistent Hide tab controls.
- Add separate disconnect, exit, power and force controls on the completion tab. Combined actions run once after a cancellable countdown, with force off by default and explicit confirmation when selected.
- See [verification and remaining gaps](docs/parity-0.51.0.md). Native visual acceptance and full parity remain unverified.

## 0.50.0 / browser integration 0.37.21 (recorded 0.37.0) — media recovery update

- Retain completed captured-stream segments across pause, failure and process restart. Verify hashes and exact content/track identity before reuse, including after changing the connection count.
- Add **Refresh media session** for paused captured YouTube streaming jobs: open the original video, recapture the same selection, then save the matching session or save and resume on the existing record.
- Reuse verified complete tracks when retrying assembly, even after the browser session expires. Preserve input tracks on output hash failure or destination conflict.
- Show retained/reused bytes, media phases, **Retry assembly**, and an ETA that excludes previously retained media. Browser panels acknowledge session recovery with a review message.
- See [test evidence and remaining limitations](docs/parity-0.50.0.md). This update does not establish current YouTube compatibility or complete IDM parity.

## v0.40.0 — combined 0.37 through 0.40 update (27 September 2026)

This published release includes the previously untagged 0.37.0, 0.38.0, and 0.39.0 milestones as well as the final 0.40.0 changes. The tag provides one honest source snapshot and file comparison from v0.36.0; the linked notes preserve the recorded detail for each intervening milestone.

- **0.40.0:** separate HTTP/HTTPS/FTP proxy routes, passive FTP through HTTP CONNECT, and Microsoft Defender/ClamAV scanner presets. See [0.40 verification and limits](docs/parity-0.40.0.md).
- **0.39.0:** durable browser-handoff ownership receipts and recovery after an extension worker restart. See [0.39 verification and limits](docs/parity-0.39.0.md).
- **0.38.0:** safer browser form-download capture up to 1 MiB, plus Firefox handoff and saved-history reliability fixes. See [0.38 verification and activation](docs/parity-0.38.0.md).
- **0.37.0:** audio-only M4A output, explicit recorded-media audio language selection, and optional WebVTT subtitles in supported MP4 downloads. See [0.37 verification and limits](docs/parity-0.37.0.md).

## Included milestone: 0.39.0 / browser integration 0.29.20 (recorded 0.29.0)

Adds persistent browser-handoff ownership receipts and recovery after the extension worker stops. Accepted downloads continue in UDM; unsubmitted downloads resume in the browser. A durable release receipt prevents a delayed Add from creating a second download. Ambiguous ownership remains for review, and recovery never replays form bodies. Adds the popup recovery button. Activated locally in UDM, Chrome and Edge; Firefox was verified in an isolated profile. Passed 761 native checks, 366 browser/protocol/preparation checks and 61 live browser checks. The setup package was rebuilt. See [verification and limits](docs/parity-0.39.0.md).

## Included milestone: browser integration 0.28.20 (recorded 0.28.0)

Adds recorded clear DASH SegmentBase/SIDX support: bounded index parsing, exact range validation, parallel native media downloads, selected audio language and audio-only M4A. Compatible with native 0.38.0. Verified with 345 automated checks and 29 isolated Chrome, Edge and Firefox checks. See [implementation and limits](docs/browser-parity-0.28.0.md).

## Included milestone: 0.38.0

Supports browser form downloads up to 1 MiB with desktop negotiation and bounded memory. Fixes incomplete multipart capture, redirected-body fallback and unreloadable history writes. Extension 0.27.1 fixes Firefox request-event ordering and pause states, validates captured upload lengths, and preserves browser downloads when complete request data is unavailable. Native 0.38.0 and Chrome/Edge integration are activated. Removed the unrelated mouse utility and restored UDM documentation. See [verification and activation status](docs/parity-0.38.0.md).

## Included milestone: 0.37.0

Recorded HLS/DASH now supports audio-only M4A output, explicit audio language selection, and optional WebVTT subtitles embedded in MP4. The simple video handoff stays one click. Native, Chrome, Edge and Firefox checks are recorded in [the verification report](docs/parity-0.37.0.md).

Each version is represented by a source snapshot tag and its original portable bundle on the [GitHub Releases page](https://github.com/usamakkhan/UDM/releases). Select **Compare** to see the exact file and code changes introduced by that version.

| Version | Historical release date | Changes from previous version |
| --- | --- | --- |
| [v0.7.0](https://github.com/usamakkhan/UDM/releases/tag/v0.7.0) | 26 Aug 2026 | First archived source snapshot |
| [v0.8.0](https://github.com/usamakkhan/UDM/releases/tag/v0.8.0) | 28 Aug 2026 | [Compare v0.7.0...v0.8.0](https://github.com/usamakkhan/UDM/compare/v0.7.0...v0.8.0) |
| [v0.9.0](https://github.com/usamakkhan/UDM/releases/tag/v0.9.0) | 31 Aug 2026 | [Compare v0.8.0...v0.9.0](https://github.com/usamakkhan/UDM/compare/v0.8.0...v0.9.0) |
| [v0.10.0](https://github.com/usamakkhan/UDM/releases/tag/v0.10.0) | 2 Sep 2026 | [Compare v0.9.0...v0.10.0](https://github.com/usamakkhan/UDM/compare/v0.9.0...v0.10.0) |
| [v0.10.1](https://github.com/usamakkhan/UDM/releases/tag/v0.10.1) | 4 Sep 2026 | [Compare v0.10.0...v0.10.1](https://github.com/usamakkhan/UDM/compare/v0.10.0...v0.10.1) |
| [v0.11.0](https://github.com/usamakkhan/UDM/releases/tag/v0.11.0) | 6 Sep 2026 | [Compare v0.10.1...v0.11.0](https://github.com/usamakkhan/UDM/compare/v0.10.1...v0.11.0) |
| [v0.12.0](https://github.com/usamakkhan/UDM/releases/tag/v0.12.0) | 8 Sep 2026 | [Compare v0.11.0...v0.12.0](https://github.com/usamakkhan/UDM/compare/v0.11.0...v0.12.0) |
| [v0.12.1](https://github.com/usamakkhan/UDM/releases/tag/v0.12.1) | 11 Sep 2026 | [Compare v0.12.0...v0.12.1](https://github.com/usamakkhan/UDM/compare/v0.12.0...v0.12.1) |
| [v0.12.2](https://github.com/usamakkhan/UDM/releases/tag/v0.12.2) | 13 Sep 2026 | [Compare v0.12.1...v0.12.2](https://github.com/usamakkhan/UDM/compare/v0.12.1...v0.12.2) |
| [v0.13.0](https://github.com/usamakkhan/UDM/releases/tag/v0.13.0) | 15 Sep 2026 | [Compare v0.12.2...v0.13.0](https://github.com/usamakkhan/UDM/compare/v0.12.2...v0.13.0) |
| [v0.14.0](https://github.com/usamakkhan/UDM/releases/tag/v0.14.0) | 17 Sep 2026 | [Compare v0.13.0...v0.14.0](https://github.com/usamakkhan/UDM/compare/v0.13.0...v0.14.0) |
| [v0.14.1](https://github.com/usamakkhan/UDM/releases/tag/v0.14.1) | 20 Sep 2026 | [Compare v0.14.0...v0.14.1](https://github.com/usamakkhan/UDM/compare/v0.14.0...v0.14.1) |
| [v0.15.0](https://github.com/usamakkhan/UDM/releases/tag/v0.15.0) | 22 Sep 2026 | [Compare v0.14.1...v0.15.0](https://github.com/usamakkhan/UDM/compare/v0.14.1...v0.15.0) |
| [v0.16.0](https://github.com/usamakkhan/UDM/releases/tag/v0.16.0) | 24 Sep 2026 | [Compare v0.15.0...v0.16.0](https://github.com/usamakkhan/UDM/compare/v0.15.0...v0.16.0) |
| [v0.16.1](https://github.com/usamakkhan/UDM/releases/tag/v0.16.1) | 26 Sep 2026 | [Compare v0.16.0...v0.16.1](https://github.com/usamakkhan/UDM/compare/v0.16.0...v0.16.1) |
| [v0.29.0](https://github.com/usamakkhan/UDM/releases/tag/v0.29.0) | 26 Sep 2026 | [Compare v0.16.1...v0.29.0](https://github.com/usamakkhan/UDM/compare/v0.16.1...v0.29.0) |
| [v0.30.0](https://github.com/usamakkhan/UDM/releases/tag/v0.30.0) | 26 Sep 2026 | [Compare v0.29.0...v0.30.0](https://github.com/usamakkhan/UDM/compare/v0.29.0...v0.30.0) |
| [v0.31.0](https://github.com/usamakkhan/UDM/releases/tag/v0.31.0) | 27 Sep 2026 | [Compare v0.30.0...v0.31.0](https://github.com/usamakkhan/UDM/compare/v0.30.0...v0.31.0) |
| [v0.33.0](https://github.com/usamakkhan/UDM/releases/tag/v0.33.0) | 27 Sep 2026 | Includes [0.32 FTP-through-SOCKS](docs/parity-0.32.0.md) and [0.33 scheduler wake timers](docs/parity-0.33.0.md); [compare v0.31.0...v0.33.0](https://github.com/usamakkhan/UDM/compare/v0.31.0...v0.33.0) |
| [v0.34.0](https://github.com/usamakkhan/UDM/releases/tag/v0.34.0) | 27 Sep 2026 | [Compare v0.33.0...v0.34.0](https://github.com/usamakkhan/UDM/compare/v0.33.0...v0.34.0) |
| [v0.36.0](https://github.com/usamakkhan/UDM/releases/tag/v0.36.0) | 27 Sep 2026 | Includes [0.35 alternate audio-track selection](docs/parity-0.35.0.md) and [0.36 signed network backend](docs/parity-0.36.0.md); [compare v0.34.0...v0.36.0](https://github.com/usamakkhan/UDM/compare/v0.34.0...v0.36.0) |
| [v0.40.0](https://github.com/usamakkhan/UDM/releases/tag/v0.40.0) | 27 Sep 2026 | Includes [0.37 recorded-media outputs](docs/parity-0.37.0.md), [0.38 form-download reliability](docs/parity-0.38.0.md), [0.39 handoff recovery](docs/parity-0.39.0.md), and [0.40 proxy/FTP/scanner updates](docs/parity-0.40.0.md); [compare v0.36.0...v0.40.0](https://github.com/usamakkhan/UDM/compare/v0.36.0...v0.40.0) |
| [v0.57.0](https://github.com/usamakkhan/UDM/releases/tag/v0.57.0) | 28 Sep 2026 | Includes milestones 0.41–0.43 and 0.50–0.57; [compare v0.40.0...v0.57.0](https://github.com/usamakkhan/UDM/compare/v0.40.0...v0.57.0) |
| [v0.68.0](https://github.com/usamakkhan/UDM/releases/tag/v0.68.0) | 28 Sep 2026 | Includes milestones 0.58–0.68; [compare v0.57.0...v0.68.0](https://github.com/usamakkhan/UDM/compare/v0.57.0...v0.68.0) |

The commits were restored from the original archived source trees. This makes GitHub's Files changed view useful for every consecutive release pair.

## Archived tag version ledger

These tags have no paired milestone heading above. The recorded number comes from each tag’s Chromium manifest. The accounted number shows its place in the reconciled sequence.

| Desktop tag | Recorded browser | Accounted browser |
| --- | --- | --- |
| `v0.92.0` | 0.63.8 | 0.64.36 |
| `v0.91.0` | 0.63.8 | 0.64.35 |
| `v0.90.0` | 0.63.8 | 0.64.34 |
| `v0.89.0` | 0.63.8 | 0.64.33 |
| `v0.88.0` | 0.63.8 | 0.64.32 |
| `v0.85.0-rc.3` | 0.63.0 | 0.64.23 |
| `v0.85.0-rc.2` | 0.63.0 | 0.64.22 |
| `v0.40.0` | 0.29.0 | 0.29.21 |
| `v0.36.0` | 0.25.0 | 0.25.20 |
| `v0.34.0` | 0.24.0 | 0.24.19 |
| `v0.33.0` | 0.24.0 | 0.24.18 |
| `v0.31.0` | 0.24.0 | 0.24.16 |
| `v0.30.0` | 0.24.0 | 0.24.15 |
| `v0.29.0` | 0.24.0 | 0.24.14 |
| `v0.16.1` | 0.16.1 | 0.16.8 |
| `v0.16.0` | 0.16.0 | 0.16.7 |
| `v0.15.0` | 0.15.0 | 0.15.7 |
| `v0.14.1` | 0.14.1 | 0.14.8 |
| `v0.14.0` | 0.13.0 | 0.13.7 |
| `v0.13.0` | 0.13.0 | 0.13.6 |
| `v0.12.2` | 0.11.0 | 0.11.6 |
| `v0.12.1` | 0.11.0 | 0.11.5 |
| `v0.12.0` | 0.11.0 | 0.11.4 |
| `v0.11.0` | 0.11.0 | 0.11.3 |
| `v0.10.1` | 0.8.0 | 0.8.3 |
| `v0.10.0` | 0.8.0 | 0.8.2 |
| `v0.9.0` | 0.8.0 | 0.8.1 |
| `v0.8.0` | 0.8.0 | 0.8.0 |
| `v0.7.0` | 0.5.0 | 0.5.0 |

The `v1.0.1` manifest records 0.63.14, while its earlier changelog heading recorded 0.63.8. The sequence uses the tag value for that milestone. The 1.0.4 release commit and its notes record 0.63.20. The separate `v1.0.4` snapshot contains 0.63.61, within the browser advances accounted for by 1.1.0.

Earlier repeated desktop 0.41, 0.42, 0.65, 0.69 and 0.78 deliveries also receive distinct patch labels below. Their browser increments were already counted, so relabeling them adds no new increments.

### Earlier GUI delivery accounting

These 39 functional candidate deliveries were omitted from the previous audits. Each linked note records its own changed behavior and build or delivered candidate. Allocate a block before the already-accounted Properties candidate: desktop 0.84.1–0.84.39 and browser 0.55.45–0.55.83. Rows use filename order as stable accounting identifiers; the notes do not establish a complete delivery chronology. The former Properties label 0.84.1 becomes 0.84.40, and later 0.84 labels and browser accounting positions move forward by 39. Recorded versions in binaries, receipts and tags remain unchanged.

| Historical note | Accounted desktop | Accounted browser |
| --- | --- | --- |
| [Arrange files menu — staged candidate](docs/gui-arrange-menu-candidate-20261001.md) | 0.84.1 | 0.55.45 |
| [Category dialog controls — staged candidate](docs/gui-category-controls-candidate-20261001.md) | 0.84.2 | 0.55.46 |
| [Predefined category lifecycle — staged candidate](docs/gui-category-lifecycle-candidate-20261001.md) | 0.84.3 | 0.55.47 |
| [Category site restriction routing — staged candidate](docs/gui-category-restrictions-candidate-20261001.md) | 0.84.4 | 0.55.48 |
| [Category icons refresh after editing](docs/gui-category-row-refresh-20261001.md) | 0.84.5 | 0.55.49 |
| [Category file types — staged candidate](docs/gui-category-types-candidate-20261001.md) | 0.84.6 | 0.55.50 |
| [Column customization: save failure and recovery](docs/gui-column-save-recovery-20261001.md) | 0.84.7 | 0.55.51 |
| [Completed-file synchronization enrollment — staged candidate](docs/gui-completed-enrollment-candidate-20261001.md) | 0.84.8 | 0.55.52 |
| [Completed synchronization queue members — candidate acceptance](docs/gui-completed-membership-candidate-20261001.md) | 0.84.9 | 0.55.53 |
| [Default synchronization queue — staged candidate, 1 October 2026](docs/gui-default-sync-queue-candidate-20261001.md) | 0.84.10 | 0.55.54 |
| [Display scaling candidate](docs/gui-display-scaling-20261001.md) | 0.84.11 | 0.55.55 |
| [Downloads menu placement — staged candidate](docs/gui-downloads-layout-candidate-20261001.md) | 0.84.12 | 0.55.56 |
| [File menu alignment — staged candidate](docs/gui-file-menu-candidate-20261001.md) | 0.84.13 | 0.55.57 |
| [Find dialog alignment — staged candidate](docs/gui-find-dialog-candidate-20261001.md) | 0.84.14 | 0.55.58 |
| [Font menu and reset — staged candidate](docs/gui-font-reset-candidate-20261001.md) | 0.84.15 | 0.55.59 |
| [Column-header customization menu](docs/gui-header-columns-20261001.md) | 0.84.16 | 0.55.60 |
| [Help product actions — staged candidate](docs/gui-help-product-candidate-20261001.md) | 0.84.17 | 0.55.61 |
| [Offline Help and stable column widths — candidate acceptance](docs/gui-help-topics-candidate-20261001.md) | 0.84.18 | 0.55.62 |
| [Main-window keyboard navigation](docs/gui-keyboard-navigation-20261001.md) | 0.84.19 | 0.55.63 |
| [Speed Limiter menu — staged candidate](docs/gui-limiter-menu-candidate-20261001.md) | 0.84.20 | 0.55.64 |
| [Shared speed-limit retention — staged correction](docs/gui-limiter-shared-save-candidate-20261001.md) | 0.84.21 | 0.55.65 |
| [Atomic multi-file queue changes — staged candidate](docs/gui-membership-batch-candidate-20261001.md) | 0.84.22 | 0.55.66 |
| [Native menu-state correction — staged candidate](docs/gui-menu-state-candidate-20261001.md) | 0.84.23 | 0.55.67 |
| [Mixed-selection Stop/Resume correction — staged candidate](docs/gui-mixed-selection-candidate-20261001.md) | 0.84.24 | 0.55.68 |
| [Progress limiter control recovery](docs/gui-progress-limiter-recovery-20261001.md) | 0.84.25 | 0.55.69 |
| [Queue drag-and-drop — staged candidate](docs/gui-queue-drop-candidate-20261001.md) | 0.84.26 | 0.55.70 |
| [Queue state indicators — staged candidate, 1 October 2026](docs/gui-queue-indicators-candidate-20261001.md) | 0.84.27 | 0.55.71 |
| [Queue membership display — staged candidate, 1 October 2026](docs/gui-queue-membership-view-candidate-20261001.md) | 0.84.28 | 0.55.72 |
| [Named queue submenus — staged candidate](docs/gui-queue-submenus-candidate-20261001.md) | 0.84.29 | 0.55.73 |
| [Expired-link recovery and stale metadata](docs/gui-refresh-link-recovery-20261001.md) | 0.84.30 | 0.55.74 |
| [Scheduler queue membership controls — staged candidate](docs/gui-scheduler-membership-candidate-20261001.md) | 0.84.31 | 0.55.75 |
| [Selected-file Stop dialog lifecycle — staged correction](docs/gui-selected-stop-candidate-20261001.md) | 0.84.32 | 0.55.76 |
| [Separate Pause all and Stop all — staged candidate](docs/gui-stop-all-candidate-20261001.md) | 0.84.33 | 0.55.77 |
| [Tasks menu alignment — staged candidate](docs/gui-tasks-menu-candidate-20261001.md) | 0.84.34 | 0.55.78 |
| [Tip of the Day — staged candidate](docs/gui-tip-day-candidate-20261001.md) | 0.84.35 | 0.55.79 |
| [UDM toolbar action completion — staged candidate, 1 October 2026](docs/gui-toolbar-share-candidate-20261001.md) | 0.84.36 | 0.55.80 |
| [Tree context menu targeting — candidate acceptance](docs/gui-tree-context-candidate-20261001.md) | 0.84.37 | 0.55.81 |
| [Help: Check for updates — staged candidate](docs/gui-update-check-candidate-20261001.md) | 0.84.38 | 0.55.82 |
| [View menu grouping — staged candidate](docs/gui-view-menu-candidate-20261001.md) | 0.84.39 | 0.55.83 |

Excluded: the whole-menu gap audit, active-menu acceptance, limiter rollback qualification, actual-transfer progress qualification and concurrent Stop all qualification introduce no separate functional delivery. Completed-file Properties is already counted by the Properties candidate and receives no additional increment.

### Additional historical builds

The 0.79–0.82 installers reused browser 0.53.1. The remaining rows reused desktop 0.84.0 and the browser version shown. Accounted desktop numbers for the repeated 0.84 builds are assigned in delivery order, including RC1–RC3 above. These labels identify historical milestones. Build receipts and tags retain the recorded versions.

| Historical build | Accounted desktop | Recorded browser | Accounted browser | Delivered change |
| --- | --- | --- | --- | --- |
| [Recorded-caption package](docs/recorded-caption-release-20261008.md) | 0.84.78 | 0.62.13 | 0.63.32 | Recorded WebVTT initialization headers before RC2 |
| [Properties 0.62.11 package](docs/properties-release-20261008.md) | 0.84.77 | 0.62.11 | 0.63.30 | Corrected subtitle link labels after the live WebVTT build |
| [Saved completion gate](docs/cli-saved-completion-20261006.md) | 0.84.76 | 0.62.8 | 0.63.26 | Wait for durable completion before CLI actions |
| [Scanner diagnostics](docs/completion-scanner-20261006.md) | 0.84.75 | 0.62.8 | 0.63.25 | Readable completion scanner diagnostics |
| [CLI hang-up](docs/cli-hangup-20261006.md) | 0.84.74 | 0.62.8 | 0.63.24 | Disconnect after successful completion |
| [CLI completion exit](docs/cli-completion-exit-20261006.md) | 0.84.73 | 0.62.8 | 0.63.23 | Exit after a successful download |
| [Transactional Pause](docs/validation/pause-package-20261006.json) | 0.84.72 | 0.62.8 | 0.63.22 | Durable pause before stopping a worker |
| [R212](docs/current-staged-candidate.md#r212-completion-dialog-controls-and-verification) | 0.84.70 | 0.62.8 | 0.63.20 | Completion dialog icon and drag area |
| [R210](docs/current-staged-candidate.md#r210-progress-options-gui-and-duplicate-text-fix) | 0.84.69 | 0.62.8 | 0.63.19 | Completion-option checkbox captions |
| [R209](docs/current-staged-candidate.md#r209-progress-status-control-alignment) | 0.84.68 | 0.62.8 | 0.63.18 | Progress URL and status controls |
| [R207](docs/current-staged-candidate.md#r207-scheduler-command-line-start) | 0.84.67 | 0.62.8 | 0.63.17 | CLI queue start |
| [R206](docs/current-staged-candidate.md#r206-queue-only-command-line-compatibility) | 0.84.66 | 0.62.8 | 0.63.16 | Queue-only CLI admission |
| [R201](docs/current-staged-candidate.md#r201-properties-source-link-control-difference-removed) | 0.84.65 | 0.62.8 | 0.63.15 | Accessible native Properties source link |
| [R174](docs/current-staged-candidate.md#r174-find-download-address-coverage) | 0.84.64 | 0.62.5 | 0.63.13 | Find matches additional download addresses |
| [R172](docs/current-staged-candidate.md#r172-protected-column-checkbox-consistency) | 0.84.63 | 0.62.5 | 0.63.12 | Protect the File Name checkbox |
| [R171](docs/current-staged-candidate.md#r171-columns-showhide-controls) | 0.84.62 | 0.62.5 | 0.63.11 | Separate Show and Hide column actions |
| [R159](docs/current-staged-candidate.md#r159-columns-dialog-controls-staged-2026-10-05) | 0.84.61 | 0.62.1 | 0.63.6 | Column width and ordering controls |
| [R158](docs/current-staged-candidate.md#r158-category-panel-close-control-staged-2026-10-05) | 0.84.60 | 0.62.1 | 0.63.5 | Category-panel close control |
| [R157](docs/current-staged-candidate.md#r157-main-tray-recovery-staged-2026-10-05) | 0.84.59 | 0.62.1 | 0.63.4 | Main tray recovery |
| [R155](docs/current-staged-candidate.md#r155-tray-progress-caption-staged-2026-10-05) | 0.84.58 | 0.62.1 | 0.63.3 | Live tray progress captions |
| [R154](docs/current-staged-candidate.md#r154-dark-download-list-gridlines-staged-2026-10-05) | 0.84.57 | 0.62.1 | 0.63.2 | Dark download-list gridlines |
| [R153](docs/current-staged-candidate.md#r153-toolbar-layout-tested-not-installed-2026-10-05) | 0.84.56 | 0.62.1 | 0.63.1 | Single-row toolbar |
| [R151](docs/current-staged-candidate.md#r151-staged--saved-http-and-ftp-logins-2026-10-05) | 0.84.55 | 0.62.1 | 0.63.0 | Saved HTTP and FTP logins |
| [R149](docs/current-staged-candidate.md#r149-installed--saved-login-controls-and-settings-lists-2026-10-05) | 0.84.54 | 0.62.1 | 0.62.99 | Saved-login editing controls |
| [R148](docs/current-staged-candidate.md#r148-installed--download-quota-preferences-2026-10-05) | 0.84.53 | 0.62.1 | 0.62.98 | Download quotas |
| [R147](docs/current-staged-candidate.md#r147-server-exception-selection-controls-staged) | 0.84.52 | 0.62.1 | 0.62.97 | Server exception selection controls |
| [R146](docs/current-staged-candidate.md#r146-scoped-wildcard-server-connection-rules-staged) | 0.84.51 | 0.62.1 | 0.62.96 | Scoped server connection rules |
| [R145](docs/current-staged-candidate.md#r145-tray-cleanup-regression-under-test) | 0.84.50 | 0.62.1 | 0.62.95 | Tray icon cleanup |
| [R144](docs/current-staged-candidate.md#r144-per-download-tray-mode-installed) | 0.84.49 | 0.62.1 | 0.62.94 | Per-download tray mode |
| [R143](docs/current-staged-candidate.md#r143-progress-start-view-behavior-staged) | 0.84.48 | 0.62.1 | 0.62.93 | Progress dialog start-view preferences |
| [R139](docs/current-staged-candidate.md) | 0.84.47 | 0.62.1 | 0.62.92 | Dark group-box captions |
| [R138](docs/current-staged-candidate.md) | 0.84.46 | 0.62.1 | 0.62.91 | Server retry and synchronization probe recovery |
| [R137](docs/current-staged-candidate.md) | 0.84.45 | 0.62.1 | 0.62.90 | Download dialog and queue-only preference dependency |
| [R136](docs/current-staged-candidate.md) | 0.84.44 | 0.62.1 | 0.62.89 | Configuration tab order |
| [R132](docs/current-staged-candidate.md#r132-idle-checkpoint-correction-in-r131-clean-build-workspace) | 0.84.43 | 0.61.0 | 0.61.87 | Idle queue checkpoint correction |
| [R126](docs/current-staged-candidate.md) | 0.84.42 | 0.61.0 | 0.61.86 | Resume status across video and audio |
| [R125](docs/current-staged-candidate.md#r125-installed-native-fix-and-browser-061-files) | 0.84.41 | 0.60.0 | 0.60.85 | Live media addresses and destination field |
| [0.84.0 Properties candidate](docs/properties-release-assembly-20261001.md) | 0.84.40 | 0.55.0 | 0.55.84 | Completed-file Properties queue-membership correction |
| [0.82.0](docs/candidate-0.82.0.md) | 0.82.0 | 0.53.1 | 0.53.44 | Dial-up/VPN credentials |
| [0.81.0](docs/candidate-0.81.0.md) | 0.81.0 | 0.53.1 | 0.53.43 | Shared HTTP connections |
| [0.80.0](docs/candidate-0.80.0.md) | 0.80.0 | 0.53.1 | 0.53.42 | HTTP/2 negotiation |
| [0.79.0](docs/candidate-0.79.0.md) | 0.79.0 | 0.53.1 | 0.53.41 | Import/export and explicit Resume |

### Deliveries already covered by a browser increment

| Delivery | Recorded browser | Accounted browser |
| --- | --- | --- |
| [0.83.0 nested HLS audio installer](docs/browser-055-nested-audio-candidate.md) | 0.55.0 | 0.55.44 |
| [0.83.0 media receipt installer](docs/candidate-0.83.0.md) | 0.54.0 | 0.54.44 |
| [0.41.1 (recorded 0.41.0) hierarchical recorded DASH](docs/browser-parity-0.31.0.md) | 0.31.0 | 0.31.21 |

### Earlier paired development records

| Desktop record | Recorded browser | Accounted browser |
| --- | --- | --- |
| [0.78.2 (recorded 0.78.0) / Firefox and resume](docs/parity-browser-0.52.2.md) | 0.52.2 | 0.52.41 |
| [0.78.1 (recorded 0.78.0) / Firefox recovery](docs/parity-browser-0.52.1.md) | 0.52.1 | 0.52.40 |
| [0.78.0](docs/parity-0.78.0.md) | 0.52.0 | 0.52.39 |
| [0.77.0](docs/parity-0.77.0.md) | 0.51.0 | 0.51.39 |
| [0.76.0](docs/parity-0.76.0.md) | 0.50.0 | 0.50.39 |
| [File-form capture candidate](docs/candidate-browser-0.49.0.md) | 0.49.0 | 0.49.39 |
| [File recognition candidate](docs/research-recognition-followup-2026-09-29.md) | 0.48.0 | 0.48.39 |
| [0.75.0 automation candidate](docs/candidate-native-0.75.0.md) | Unspecified | 0.47.40 |
| [0.65.1 (recorded 0.65.0) / Edge video selection](docs/parity-browser-0.43.0.md) | 0.43.0 | 0.43.32 |
| [0.43.0](docs/parity-0.43.0.md) | 0.36.0 | 0.36.21 |
| [0.42.3 (recorded 0.42.0) / captured YouTube audio](docs/parity-browser-0.35.0.md) | 0.35.0 | 0.35.21 |
| [0.42.2 (recorded 0.42.0) / hierarchical DASH indexes](docs/parity-browser-0.34.0.md) | 0.34.0 | 0.34.21 |
| [0.42.1 (recorded 0.42.0) / separate DASH indexes](docs/parity-browser-0.33.0.md) | 0.33.0 | 0.33.21 |
| [0.42.0](docs/parity-0.42.0.md) | 0.32.0 | 0.32.21 |
| [0.41.0](docs/parity-0.41.0.md) | 0.30.0 | 0.30.21 |
| [0.38.0](docs/parity-0.38.0.md) | 0.27.1 | 0.27.21 |
| [0.37.0](docs/parity-0.37.0.md) | 0.26.0 | 0.26.20 |
| [0.35.0](docs/parity-0.35.0.md) | 0.25.0 | 0.25.19 |
| [0.32.0](docs/parity-0.32.0.md) | 0.24.0 | 0.24.17 |
| [0.28.0](docs/parity-0.28.0.md) | Unspecified | 0.24.13 |
| [0.27.0](docs/parity-0.27.0.md) | 0.24.0 | 0.24.12 |
| [0.26.0](docs/parity-0.26.0.md) | 0.23.3 | 0.23.15 |
| [0.25.0 / browser audit fixes](docs/audit-0.23.2.md) | 0.23.2 | 0.23.14 |
| [0.25.0 / reload recovery](docs/browser-reload-0.23.1.md) | 0.23.1 | 0.23.13 |
| [0.25.0](docs/browser-parity-0.25.0.md) | 0.23.0 | 0.23.12 |
| [0.23.0](docs/player-retrieval-0.23.0.md) | 0.21.0 | 0.21.12 |
| [0.22.0](docs/links-login-0.22.0.md) | 0.20.3 | 0.20.15 |
| [0.21.2](docs/parallel-video-0.21.2.md) | 0.20.3 | 0.20.14 |
| [0.21.1](docs/video-comparison-0.21.1.md) | 0.20.3 | 0.20.13 |
| [0.21.0](docs/video-comparison-0.21.0.md) | 0.20.3 | 0.20.12 |
| [0.20.0 / panel recovery](docs/browser-0.20.2.md) | 0.20.2 | 0.20.11 |
| [0.20.0 / capture bypass](docs/browser-0.20.1.md) | 0.20.1 | 0.20.10 |
| [0.20.0](docs/gui-0.20.0.md) | 0.20.0 | 0.20.9 |
| [0.19.0](docs/gui-0.19.0.md) | 0.17.0 | 0.17.9 |
| [0.18.0](docs/gui-0.18.0.md) | 0.17.0 | 0.17.8 |
| [0.17.0](docs/speed-menu-0.17.0.md) | 0.17.0 | 0.17.7 |

## Documented development milestones

The project also retains the following development notes. They are committed with the current source so their work is visible in Git, but they are **not** backfilled as source tags or GitHub Releases: separate source snapshots and portable builds for these points were not available. Consequently, GitHub cannot show an honest file-by-file comparison for them. The notes describe the recorded work; they do not claim to recreate the exact code at that time.

| Documented version | Recorded change note |
| --- | --- |
| 0.17.0 | [Transfer measurements, video formats and download dialogs](docs/speed-menu-0.17.0.md) |
| 0.18.0 | [Desktop GUI expansion](docs/gui-0.18.0.md) |
| 0.19.0 | [Desktop workflow completion pass](docs/gui-0.19.0.md) |
| 0.20.0 | [YouTube panel repair and browser controls](docs/gui-0.20.0.md) |
| 0.20.1 | [Browser integration](docs/browser-0.20.1.md) |
| 0.20.2 | [Panel recovery and verified Edge download](docs/browser-0.20.2.md) |
| 0.21.0 | [Video comparison and implementation notes](docs/video-comparison-0.21.0.md) |
| 0.21.1 | [Live video comparison and storage repair](docs/video-comparison-0.21.1.md) |
| 0.21.2 | [Parallel YouTube downloads](docs/parallel-video-0.21.2.md) |
| 0.22.0 | [Media addresses and server logins](docs/links-login-0.22.0.md) |
| 0.23.0 | [Native player retrieval and browser integration](docs/player-retrieval-0.23.0.md) |
| 0.23.1 | [Extension reload recovery](docs/browser-reload-0.23.1.md) |
| 0.23.2 | [Audit](docs/audit-0.23.2.md) |
| 0.25.0 | [Browser integration update](docs/browser-parity-0.25.0.md) |
| 0.26.0 | [Native and browser comparison](docs/parity-0.26.0.md) |
| 0.27.0 | [Browser integration 0.24.0](docs/parity-0.27.0.md) |
| 0.28.0 | [Offline websites and SOCKS5](docs/parity-0.28.0.md) |
