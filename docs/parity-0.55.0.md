# UDM 0.55.0 / browser 0.38.0

This update fixes download-limit feedback and the Existing duplicate workflow. It builds on the browser customization and transport updates in 0.54 and 0.53. It does not establish complete IDM parity.

## Download limits

- Options > Connection now has the warning checkbox at the reference position. It defaults to enabled when no value has been saved; disabling it keeps quota enforcement active.
- A native Download limits exceeded dialog explains the configured limit and the countdown to automatic resumption. It warns once per quota period and closes if the wait ends or warnings are disabled. Dismissing it does not override the limit.
- Progress shows the quota countdown and the main list shows Waiting for quota. Pause still stops the workers; raising or disabling the limit releases them.
- Concurrent reservations share one account. A chunk larger than the available allowance consumes that allowance and waits for the remainder, so a large chunk cannot wait forever for an impossible amount. Reservations never exceed the configured application-data allowance in a period.
- Restart preserves consumed quota. Invalid saved timestamps and a clock moving backwards recover by beginning a new quota period. A test exposed the invalid-date exception during development; the final build passes that recovery case.

Quota reservations gate processing of transfer chunks. A waiting chunk and HTTP/socket buffers can already contain additional network data; this is not an exact packet-level ISP usage meter. Test rollovers age only isolated saved timestamps, without changing the Windows clock.

## Existing downloads

- Choosing Existing for a completed link opens Download complete, with Open, Open with, Open folder and Close. This explicit view works even if automatic completion dialogs are suppressed, and does not replay sounds, file-opening actions or shutdown plans.
- Choosing Existing for an unfinished link schedules resume directly. The remembered Existing preference and browser handoff use the same path; saved segments are retained.
- A genuinely new download still opens File Info. Repeated capture of an unconfirmed download does not silently approve it. Completed-file double-click remains Properties.
- A failed history write leaves the original paused record and retry intent intact. Removed records cannot be restarted from an old offer.

Reference resources 141, 194 and 298 informed the controls and routing. Six selected bounds match the installed reference. Code is independently implemented. Primary documentation: [IDM Connection options](https://www.internetdownloadmanager.com/support/using_idm/options.html) and [Download complete](https://www3.internetdownloadmanager.com/support/using_idm/completeD.html).

## Verification

**1,139 checks passed:** 1,095 native checks, 8 native-protocol checks, 8 real isolated app/host checks and 28 live browser/native scenarios across Chrome and Edge. The native suite includes concurrent quota reservations, cancellation, settings changes, restart, rollover, malformed timestamps, real segmented transfers and byte-exact resumed output. Windows font measurement confirms the new captions and warning text fit at 96, 120, 144 and 192 DPI.

Chrome and Edge each exercised real MP4 download, recorded HLS with audio, ordinary file capture, force/bypass gestures and delayed script downloads using the rebuilt app and host. The browser source remains 0.38.0. These are local functional tests, not an internet speed comparison or proof of every video site's compatibility.

Receipts: [summary](evidence-0.55.0/summary.json), [native log](evidence-0.55.0/build-final.log), [selected layout audit](evidence-0.55.0/layout-audit.json), [workflow inventory](idm-parity-0.55.0.md).

## Remaining acceptance

Native dialog screenshots, clicks, accessibility and mixed-monitor DPI remain unverified because the Windows control tool fails before initialization with a missing kernel-assets path. Remembering replacement and replacing unfinished files remain different from IDM; retained-version replacement behavior is preserved. Public video-site coverage, driver equivalence, signing, clean-machine installation and a new controlled speed comparison remain open. No parity percentage is claimed.

## Current-PC deployment

[UDM 0.55.0 installer](../installer-out/UDM-0.55.0-Browser-0.38.0-Setup-x64.exe) is built. 39 files were deployed with SHA-256 verification and original-file backups. Installer SHA-256: `D1CB65E23FB8061C4D71D49AFF3AE7A7ED4F1CE84BC77873648A6148DF8D2930`.

The running app/native bridge reports 0.55.0 and the correct D:/UDM/user-data catalog. All 23 records, queues and saved preferences are unchanged. Browser 0.38.0 sources and the signed network runtime are unchanged. This desktop update requires no extension-source reload; activation in existing personal browser sessions remains unverified.
