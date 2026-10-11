# Changes from UDM 0.85.0 RC3 to 1.00.0

UDM 1.00.0 contains desktop version 1.00.0 and browser integration 0.63.7. This is a change log for the [published RC3](https://github.com/usamakkhan/UDM/releases/tag/v0.85.0-rc.3) to [1.00.0](https://github.com/usamakkhan/UDM/releases/tag/v1.00.0), not a claim of complete IDM parity. The tested product source is `7023dcdeecb5b788a0cd368c372c2c0bbebe0e04`; later in-progress Grabber work is outside this release.

## Browser response capture and downloads

- Retain a browser's original HTTP response during native capture preparation, including modern Chromium downloads. A declined or failed handoff can leave the browser transfer running instead of discarding a one-use response.
- Admit an already-started response only after the native job and response ownership are durably recorded. Lost acknowledgements can be reconciled without creating a second job or replaying the source request.
- Resume an interrupted captured response only when its saved validator and remote byte identity still match. Stage recovered prefixes transactionally; a mismatched range cannot be mixed into the published file.
- Recover completed publications after catalog save failures and retry transient backup locks. Duplicate captured response bodies are released after durable publication.
- Apply the shared global and per-download speed limits to captured browser responses using negotiated byte credits. Credits and pending waits are released correctly when limits are disabled, a save is delayed, or capture is cancelled. The progress display clears stale speed when a capture completes or pauses.
- Protect captured authorization headers in stored state, preserve permitted request headers for authenticated resume, and reject unsafe or contradictory header names and values. [Response-resume evidence](validation/original-response-resume-20261010.json) and [credit negotiation](validation/response-credit-negotiation-20261011.json).

## Handoff, crash recovery, and user decisions

- Keep prepared browser requests available for explicit recovery after a browser or native-process crash, including non-resumable POST requests. Recovery screens show recognizable request metadata and provide restart, discard, and native-review actions.
- Reconcile prepared and committed ownership after lost native replies; a lost status acknowledgement prompts one bounded reconnect. Desktop pause and stop decisions remain in force while response admission is uncertain.
- Reconnect to a still-running authenticated network helper after a desktop restart, instead of replacing the helper and losing its active session. Framing failures invalidate the compromised command channel; broker frame deadlines stay bounded.
- Keep the network helper alive during normal desktop idle periods without requesting elevation merely for background polling. Stale or ambiguous process identifiers are rejected before attachment.
- Preserve Grabber cancellation and replacement decisions across stale wizard saves and browser sign-in attempts. [Recovery review evidence](validation/recovery-review-outcome-20261011.json), [helper reconnection](validation/reconnect-release-20261011.json), and [Grabber cancellation](validation/grabber-cancel-edge-20261011.json).

## Site Grabber and offline-site discovery

- Find literal links created by inline and external JavaScript, including escaped URLs and mixed-case JavaScript MIME declarations. Failed external script reads have a deadline and do not erase links already found on the page.
- Decode HTML entities before handling CSS escapes and rewrite discovered CSS assets for offline pages.
- Collect links created in the live DOM through the browser extension. The collector bounds its work and binds each result to the browser document identity, rejecting stale results after a same-URL reload, navigation, closed tab, private context, or wrong frame.
- Validate rendered snapshots at the native boundary, tie replies to a specific project request generation, and store protected snapshots with durable receipts. A lost save acknowledgement is reconciled from the receipt; removed projects have their saved rendered state pruned. [Rendered navigation](validation/rendered-navigation-20261011.json), [native validation](validation/rendered-native-validation-20261011.json), and [native messaging](validation/rendered-native-messaging-20261011.json).

## Browser integration versions

| Browser version | Main change |
| --- | --- |
| 0.63.1 | Explicit saved-request review and restart, bounded native-status reconnection, and video-menu keyboard entry. |
| 0.63.2 | Recognizable recovery summaries for prepared requests in Edge and Firefox. |
| 0.63.3 | Interrupted-capture reconciliation, native review reopening, and broker stability fixes. |
| 0.63.4 | Keep open video format menus stable while player metadata arrives; empty mapped-caption recovery. |
| 0.63.5 | Restart or discard failed saved requests after Edge/Firefox network and server failures; improved Grabber script discovery. |
| 0.63.6 | Keep Chromium's original response running until native capture preparation is durable. |
| 0.63.7 | Match Chromium proxy bypasses for mapped IP addresses and scheme-qualified CIDR rules, with matching native transport fixes. |

## Media and interface

- Accept valid empty WebVTT subtitle segments in both chunked and close-delimited HLS responses, while retaining mapped caption timing across live and recorded workflows.
- Keep video format menus open through asynchronous player initialization and refresh an open empty menu when metadata arrives. Public YouTube direct-capture and Dailymotion recorded-video paths received focused Edge checks; these do not establish general coverage of either site.
- Reject media recapture when refreshed bytes or segment identity differ, retaining retryable partials rather than publishing a mixed result.
- Show unknown resume support until a server actually establishes it. Progress controls respect disabled actions for both mouse and keyboard; percentage precision, completed size display, Properties actions, and the native Move picker were refined.
- A captured publication opens the native completion dialog; finished captures clear their live speed display. [Caption evidence](validation/empty-close-captions-20261011.json), [panel evidence](validation/panel-metadata-race-20261011.json), and [public-media scope](validation/dailymotion-public-download-20261011.json).

## Proxy routing and native transport

- Align browser bypass decisions with effective Edge proxy settings for IPv4-mapped IPv6 addresses and CIDR rules, including scheme-qualified rules and explicit proxy bypasses.
- Use the existing curl transport for explicitly direct mapped IPv6 downloads where WinHTTP could fail with error 12029. Concurrent ranges and resume were exercised on that path; untrusted mapped-address TLS targets are rejected.
- Preserve safe failure behavior for routes the helper cannot support. Trusted mapped-address HTTPS and broad live routing remain incompletely qualified. [Routing package evidence](validation/routing-release-0637-20261011.json), [mapped transport](validation/mapped-transport-regression-20261011.json), and [TLS rejection](validation/mapped-tls-rejection-20261011.json).

## Installer and release verification

- Keep installer migration and required installation steps transactional. Isolated install, repair, and uninstall checks confirm native-messaging registration, restored missing payload files, preserved custom download history, and retained partial download bytes.
- Built the exact 1.00.0 desktop binaries and 0.63.7 browser payload into `UDM-1.00.0-Browser-0.63.7-Setup-x64.exe` (61,559,710 bytes, SHA-256 `CDC8B2E33511FA7EFA52BA4C6453E192C12013356B51B22D569B9A5FA3BD09EE`).
- The exact native executable passed **2,599/2,599** checks with local socket access. Rendered-capture unit fixtures passed **18/18**; actual Edge extension and Edge-to-native fixtures passed **3/3** and **5/5**. The production-derived isolated installer passed **35/35** lifecycle checks. The signed network runtime verified successfully. [1.00.0 verification record](validation/release-100-20261011.json).

The first native run in a restricted environment stopped after 107 passes at Windows socket error 12029; the same executable passed in the unrestricted rerun. The production installer was not installed in the personal application location, and the isolated lifecycle fixture does not qualify a machine-wide administrative upgrade. Live elevated-driver capture, broad public-site compatibility, and exact IDM feature or visual parity remain unverified.
