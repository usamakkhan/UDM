# UDM 0.34.0: monitored virus checking

Previously UDM launched an external scanner and immediately forgot the process. It could announce completion or run a queue action before scanning finished; it never recorded the exit code. A blank argument field passed no downloaded filename. Synchronized replacement files bypassed this hook.

## Using it

Open **Options → Downloads → Virus checking settings**. Browse to your scanner's executable, enter the parameters documented by its vendor, and select a maximum wait of 1–3600 seconds (default 300). A blank program turns automatic checking off. Existing scanner program/arguments are retained; this release does not install, enable, elevate, reconfigure, or disable antivirus software.

Use `{file}` or IDM-style `[File]` in arguments. UDM expands all occurrences and quotes each argument, including paths with spaces or Unicode characters. If there is no placeholder, it appends the full path. It starts the exact .exe without a command shell, from the executable's directory, without inheriting UDM's handles.

**File Properties → Virus check** shows the recorded result and offers **Scan file**, **Stop waiting**, and scanner settings. Manual scanning reads the existing downloaded file; it does not download again or replay completion actions. Progress shows Scanning file and its Information tab includes the result. The completion dialog also displays the scanner result.

## Result handling

- Automatic checking runs on the final published file, once after a completed transfer, including synchronized replacements. Video/audio intermediate files are not individually scanned by this hook.
- The download retains its active queue slot until scanner monitoring finishes. Completion events, per-file actions and queue actions wait. A recorded exit zero allows the existing completion workflow; a nonzero exit, launch error, timeout or interrupted wait cancels automatic completion actions.
- Scanner status is separate from download status. UDM stores the scanner filename, timestamps, exit code and monitoring message. It does not store the full command or capture scanner output.
- Missing executables and process launch errors are visible. The timeout and Stop waiting stop UDM's monitoring without killing the antivirus. Closing UDM also stops waiting promptly. A crash/restart marks any saved Running result Interrupted; UDM does not automatically launch another scanner.
- Generic exit codes are **not malware verdicts**. Even code 0 can have different meanings between scanners. Consult the vendor's documentation and antivirus history. A launcher that returns before its scanning service completes cannot be treated as a completed scan by this generic process monitor.
- This is **not a quarantine or publication gate**. The downloaded file already exists and manual opening is available. UDM does not delete or quarantine it. Real-time antivirus remains independent.

The reference's documented configuration uses an executable and parameters with a filename placeholder, with automatic filename appending when parameters are blank: [IDM Options](https://support.internetdownloadmanager.com/support/options.html). UDM implements this independently, with additional process-result tracking. Exit-code caution is concrete: [Microsoft Defender's command-line documentation](https://learn.microsoft.com/en-us/defender-endpoint/command-line-arguments-microsoft-defender-antivirus) distinguishes several meanings for its success and failure codes; those meanings cannot safely be assigned to every scanner.

## Validation

| Suite | Passed | Evidence |
|---|---:|---|
| Full native regression | 698 | Includes 41 scanner checks: quoting, Unicode, repeated placeholders, configuration validation, real process exit codes, launch errors, timeout/cancel, state reload, queue/event ordering, manual rescan and synchronized replacement |
| Real UDM scanner workflows | 10 | Isolated app histories, byte-exact local HTTP downloads, actual child-process handoff, nonzero/missing scanner, timeout without termination, app crash/restart without redownload |
| Native launch workflows | 7 | Cold/warm command-line handoff, File Info/metadata confirmation, download completion while a dialog is open |
| Native messaging | 7 | Persistent and fragmented messages, invalid messages and bounded framing |

**722 checks passed before installation.** The 41 targeted scanner checks are included in 698, not counted twice. The process fixtures are benign test programs, not a substitute for qualification against real antivirus products. Initial sandbox runs encountered Windows access-denied saving test state / app startup; final passing runs used the same fixtures outside that restriction. All four native binaries build; existing deprecated `inet_addr` and test-variable shadow warnings remain.

## Installation

Installed and running as **UDM 0.34.0**. All **31 installed files** match their deployment hashes. All **23 download records, queue settings and existing preferences are unchanged**, including after browser testing. The only new preference is the default scanner wait timeout; automatic checking remains off because no scanner was configured. The native host confirms 0.34.0 and the normal user-history directory. Backups are in `parity-scanner-20260927/backup-before-0.34.0`.

Installed Edge integration passed **13 additional checks** with a separate profile and history: persistent messaging, Download Later, authenticated range transfer, byte-identical MP4, recorded HLS/audio, modifier gestures and stale iframe exclusion. **Final verification: 735 checks passed**, without counting overlapping runs twice.

## Remaining acceptance

The [103-workflow audit](idm-parity-0.34.0.md) remains **87 implemented, 12 partial, 2 unverified and 2 missing**. Scanner integration has improved, but real vendor adapters/presets, verdict/quarantine behavior and visual acceptance are not claimed. This release makes no Internet speed or complete IDM parity claim.

The Computer Use helper still fails before initialization with `failed to write kernel assets: The system cannot find the path specified. (os error 3)`. Native screenshots, keyboard traversal and DPI layout of the new dialogs remain unverified. Wider video coverage, live/subtitles/alternate tracks, localization, remaining proxy modes, signed distribution, physical wake testing and matched-route performance testing also remain open.

Browser extension source remains 0.24.0. Evidence and backups are under `parity-scanner-20260927` in the writable visualization workspace.
