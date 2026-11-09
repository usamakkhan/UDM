# IDM comparison follow-up

This pass implements two reproduced changes from the [detailed comparison plan](../idm-parity-execution-plan-20261107.md). The remaining inventory contains current UDM behavior, known differences, intentional scope decisions and comparisons that still need a licensed IDM session. A passing local fixture qualifies its behavior and version.

## 5.26 and browser 0.68.90

### E03 and E04. Reconnect pressure

Range workers previously retried failed opens on independent timers. On an origin with two accepted connections, competing attempts continued even while two responses were delivering the file. The 5.26 transfer engine shares admission of retries after a refusal or connection reset before response headers. The coordinator is local to the transfer. Already productive workers keep their range ownership and can keep reusing connections. Failed workers release ranges before waiting. A shorter probe interval applies when no peer has an accepted response. HTTP response retries, validators and the ordinary retry budget retain their existing paths.

All five production executables and both browser trees were staged. The browser version advances with the desktop according to the project version policy. [5.26 native build](range-reconnect-pacing-5.26-build.json)

Two interleaved runs per version completed each owned transfer with the expected SHA-256. On the two-connection cap, 5.25 recorded 58 and 56 refused attempts. 5.26 recorded 13 in each run. Both 5.26 outages completed with fewer refused attempts, but took 21.872 and 20.771 seconds versus 16.077 and 16.771 seconds on 5.25. The 5.26 pacing policy therefore needs the outage recovery follow-up before installed qualification.

### E06. Read the existing timing before changing finalization

The prior 5.25 benchmark recorded about 0.06–0.28 seconds from network completion through publication on these same-volume fixtures. Its recorded no-range receive phase was 2.72–4.28 seconds. Those measurements do not reproduce the historical multi-second assembly cost. Repeat a quiet sequential run and compare network reads, flush and cross-volume behavior before changing publication or hash verification.

## 5.27 and browser 0.68.91

### B01, B08 and B10. Keep the pending format menu after a same-player reload

The browser fixture reproduced a load event closing a pending video menu even when the player source identity stayed the same. Both browser bundles now retire the stale lookup while keeping the user's open and keyboard intent. A changed source still closes the old menu. The refreshed lookup can complete without a second click, and a delayed reply from the prior generation cannot repopulate it.

The controls fixture now expects Arrow Up to reuse a current catalog. Explicit Refresh still requests a new one. The source-race fixture checks stale replies, keyboard focus after a same-player reload, and that an actual source change closes the old menu. The Edge fixture executes both bundles in a controlled browser DOM. It does not establish installed Firefox behavior.

The browser improvement received its own source version. The next native build carries it forward with the recovery follow-up.

## Other gap checks

- The 5.26 native host passed 18 rendered Grabber handoff checks and the setup helper passed 12 catalog preservation and rollback checks. Eight Explorer registration source checks passed. These checks exercised fixture state and source only.
- Four startup queue checks passed on the 5.26 app. It started the opted-in queue, saved a byte-exact file, left the manual queue paused, and respected clearing the startup opt-in on relaunch.
- The public YouTube fixture now uses a unique native host registration and private browser profile. It verifies the expected staged app version and private catalog before visiting the page. An initial 5.26 run stopped at the missing media tools guard. With the pinned helpers attached, a fresh Edge profile delivered exactly one 1080p job to 5.26 in Awaiting confirmation. The temporary host registration was removed. This test did not measure exact size before Start.
- Both a 5.26 public completion attempt and a 5.28 attempt after warming browser playback reached the native queue. The streaming origin then rejected the request with a browser-attestation requirement, before any media bytes were retained. Both temporary registrations were removed. Native handling for a fresh, valid browser session and public exact-size acceptance remain open.

## 5.28 and browser 0.68.92

Once an outage has left no accepted response and no recent bytes, the first accepted response opens a brief window for faster parallel recovery. The slower probe interval still applies when healthy responses are already present. The serial build produced five 5.28 executables. All five hashes and 77 recorded input hashes matched the receipt. The two staged browser trees contain 95 verified files. [5.28 build receipt](range-recovery-and-panel-5.28-build.json)

| Loopback case | 5.25 wall seconds | 5.28 wall seconds | Refused attempts, 5.25 → 5.28 |
| --- | --- | --- | --- |
| Healthy ranged origin | 14.646 / 10.132 | 12.892 / 9.132 | 0 / 0 → 0 / 0 |
| Two accepted connections | 20.071 / 20.482 | 18.859 / 18.831 | 58 / 56 → 13 / 13 |
| Five-second outage | 16.077 / 16.771 | 14.908 / 16.475 | 70 / 72 → 23 / 24 |
| Origin ignores Range | 4.504 / 4.308 | 3.268 / 6.707 | 0 / 0 → 0 / 0 |

The capped case met the plan's 50% reduction target, with about 77% fewer refusals. The healthy and capped wall times were within the provisional guardrail in these trials. The outage ramp recovered the time lost in 5.26. All eight 5.28 outputs matched the expected SHA-256. Sequential timing remained variable. The coordinator applies only to parallel range transfers. A licensed paired run and fairness tests across multiple jobs sharing one origin are still open.

The 5.28 source passed 19 panel-control checks per browser bundle, 24 source-race checks across both, and five ad-transition checks. The panel cases use isolated Chromium-family browser processes. Chromium and Firefox content bundles ran under the same test environment. With staged native 5.28 and extension 0.68.92, isolated Edge passed 58 HLS checks and 10 indexed DASH checks, including full decoding, selected streams and exact indexed range requests.

## Q02. Make the Scheduler fixture report its first failure

The Scheduler test again stalled inside an owned modal. Desktop inspection found the Scheduler disabled behind a secondary dialog. The test now unwinds its own modal loops after a failed assertion or deadline. A simulated Close would invoke normal Scheduler validation again. The fixture refreshes Apply before its synthetic click, chooses a value distinct from the default, and compares only enabled controls in the reachable tab order.

The updated fixture reached 105 passing assertions on 5.28, including invalid-number rejection, catalog preservation, quota persistence, native tab navigation and applying exact seconds. It then hit its modal deadline after pressing Close in the run-once case. This is a partial result. Isolate that Close path and verify the saved run-once schedule after reopening before closing Q02. These are test-only changes made after the production build.

## Remaining acceptance

The previous installer attempt ended at Windows elevation, and IDM's local trial had expired. Program Files still reported UDM 4.74 during this pass. Installation, activated extensions, new paired IDM timings and comparisons, public video and exact pre-Start size, physical wake, actual Explorer/COM registration and release signing remain open.
