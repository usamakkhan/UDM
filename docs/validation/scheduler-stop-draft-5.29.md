# Scheduler run-once stop draft in UDM 5.29

## Reproduction and fix

In the 5.28 native fixture, a run-once queue saved a start at 10:25:45 and a stop at 18:30:15. Clearing **Stop on** in Advanced disabled the stop on the Schedule tab as well. **Apply** correctly saved an empty `StopOnceUtc`, then initialized the disabled picker with the current local date and time. The test ran around midnight. Re-enabling the stop produced a deadline earlier than the start, and validation opened a warning when the fixture tried to close Scheduler. The retained results show 105 successful checks before the dialog deadline. [5.28 results](idm-gap-work-5.26-5.28.md#q02-make-the-scheduler-fixture-report-its-first-failure)

After a successful Apply, Scheduler keeps the stop date and time controls for the current run-once queue if its stop is disabled. The stored deadline stays empty while disabled. Re-enabling the stop in the same dialog can restore the previously selected instant. Opening a fresh dialog or selecting a different queue still initializes its controls from that queue's saved state. The catalog schema is unchanged.

The regression exercises both tabs, checks the cleared deadline and the preserved clock, compares the exact restored UTC deadline, closes the dialog, and reopens the private catalog to check the saved seconds.

## Qualification on November 8, 2026

- The production build produced five UDM 5.29 executables. The recorded 78 input hashes and the staged executable hashes matched. Chromium and Firefox 0.68.93 and the extension setup page were staged as 95 verified files. [Build receipt](scheduler-stop-draft-5.29-build.json)
- The Scheduler fixture from the same source passed all 692 assertions. It reached both catalog-reopen checks after Close, then completed the Files geometry, mnemonic, radio-group navigation, and light and dark theme checks at 96, 120, 144 and 192 DPI. These are assertions on the owned dialog. A first launch with a pre-created result directory exited before running any checks. The fixture requires a fresh path and created it on the successful run.
- The isolated pass closes the Q02 fixture failure. The installed comparison against IDM, actual wake from suspend, and public media completion still need their own evidence. This candidate is staged as loose native and browser files.
