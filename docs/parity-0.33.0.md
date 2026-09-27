# UDM 0.33.0: scheduler wake timers and date handling

UDM previously polled scheduled queues while Windows was awake but did not register a timer to request waking the computer. Scheduler now has an optional **Wake computer for this queue** checkbox, an applied-timer status, and **Wake details**. Existing queues remain opted out.

## Using it

Open **Downloads → Scheduler → Schedule**, enable the queue, choose its date or weekday window, check **Wake computer for this queue**, and click **Apply** or **OK**. UDM registers the earliest future start that has eligible queue work. Keep UDM running, including minimized to the notification area. Windows must permit wake timers; this feature does not change power settings, sign-in requirements or startup registration. It does not turn on a shut-down PC or restart UDM after exit.

The status describes the applied settings. Draft changes take effect on Apply/OK. With several queues, “Next app wake” shows the earliest requested wake across them; Wake details identifies its queue. Windows timer registration is a wake request, not proof that the machine will physically wake. Unsupported registration is reported without disabling normal awake scheduling.

## Scheduling and fixes

- One absolute UTC Windows timer covers enabled, opted-in queues with pending eligible downloads or supported completed-file synchronization. Empty queues, unresolved duplicates, browser-recapture jobs, already submitted forms and confirmation prompts do not cause a wake.
- Daily deadlines use the same local-time window predicate as the queue engine, including weekday masks and overnight windows. Repeating queues use their saved next-run time when it is inside an allowed window. The timer is recalculated as work or settings change, and periodically for clock/time-zone changes.
- Applying a new date replaces the timer. Stopping/disabling a queue or exiting UDM cancels it; restarting UDM restores a future saved schedule. A completed one-time queue does not rearm its old start time.
- **Stop now clears the scheduler's Queue enabled checkbox.** Previously pressing OK afterward could unintentionally enable the queue again.
- Date entry/display uses the selected date's Windows daylight-saving rules, rather than today's offset. Nonexistent spring times are rejected. Newly entered repeated autumn times choose the first occurrence; applying an unchanged saved time preserves its original UTC occurrence and hidden seconds.
- Daily windows follow existing queue semantics: a window entirely inside a skipped spring hour does not run that day; a repeated autumn window can open twice. A run-once schedule retains one exact UTC instant.
- Timer status is transient and available through local diagnostics. Download history does not store handles or timer state.

Reference behavior: [IDM's wake/scheduler guidance](https://www.internetdownloadmanager.com/register/new_faq/functions22.html) requires IDM to remain running and Windows to allow wake timers. Windows implementation references: [SetWaitableTimer](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-setwaitabletimer) and [date-aware local/UTC conversion](https://learn.microsoft.com/en-us/windows/win32/api/timezoneapi/nf-timezoneapi-tzspecificlocaltimetosystemtimeex). UDM's implementation is independent.

## Validation

| Suite | Passed | Evidence |
|---|---:|---|
| Full native regression | 657 | Includes 56 new wake/date checks: weekday/overnight/DST planning, real Windows timer signaling/cancellation, earliest queue selection, eligibility, persistence failure, state reload and exact date preservation |
| Real app scheduled downloads | 9 | Timer diagnostics, no early HTTP requests, byte-identical downloads at their deadlines, opt-out behavior, restart/rearming, disabled queues and timer removal after completion |
| Native launch workflows | 7 | File Info confirmation, metadata preview, command-line handoff and queued transfer while a dialog is open |
| Native messaging | 7 | Persistent/fragmented frames, invalid messages and bounded framing |

**680 checks passed before installation**, counting each final scenario once. Tests ran with local HTTP fixtures and isolated app histories. The corrected date tests use the installed Pacific time-zone definition without changing Windows' active zone. Only final passing runs are counted; earlier fixture setup, completion-timing and process-cleanup failures were corrected before acceptance.

All four native binaries build successfully. Logs retain existing compiler warnings for `inet_addr` and a shadowed test variable. This release does not claim a change in Internet download speed.

## Installation

Installed and running as **UDM 0.33.0**. All **28 installed files** match their deployment hashes. All **23 existing download records and all queue settings are unchanged**, including after the browser test. The installed native host reports 0.33.0, the correct history directory/count, and wake status Off for the existing queues. Replaced files and state are backed up under `parity-scheduler-wake-20260927/backup-before-0.33.0`.

Installed Edge integration passed **13 additional checks**, including persistent messaging, Download Later, authenticated range transfer, byte-identical MP4, recorded HLS with audio, modifier gestures and stale iframe exclusion. Final verification: **693 checks passed**, excluding overlapping runs. The browser test used a separate profile and test history.

## Remaining acceptance

The [103-workflow audit](idm-parity-0.33.0.md) has **87 implemented, 12 partial, 2 unverified and 2 missing rows**. Wake scheduling moved from unverified to partial: physical sleep/hibernate resume has not been exercised on this PC. The 16 open rows are not closed by an API timer firing while the system is awake.

Native screenshot/layout/keyboard/DPI inspection remains unverified. The Computer Use helper still fails before initialization with `failed to write kernel assets: The system cannot find the path specified. (os error 3)`. The scheduler UI compiles; its wake backend and real timed-download workflow are tested. Wider video coverage, live/subtitle/alternate-track workflows, localization, remaining proxy modes, signed distribution, and full performance/accessibility acceptance remain open.

Browser extension source remains 0.24.0. Test evidence, deployment hashes and backups are retained under `parity-scheduler-wake-20260927` in the writable visualization workspace.
