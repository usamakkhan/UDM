# Native conversion validation — 19 September 2026

UDM 0.7.0 replaces the desktop, download engine, browser native host and network monitor with original x64 C++17. The desktop uses Unicode MFC with static MFC/CRT linkage. PE inspection reports machine 8664 and a zero CLR/COM descriptor. No C# subprocess implements the native UI or transfers. Browser code remains JavaScript; the optional kernel component remains C.

## Automated evidence

The native C++ suite passed **64 checks with zero failures**. It exercised real local HTTP transfers, parallel ranges, no-range and unknown-length responses, empty files, truncation and invalid range rejection, pause/resume, hashes, destination protection, DPAPI, dates/dictionaries, atomic state, grabber filtering and browser confirmation.

Media checks covered fragmented synthetic UMP responses, complete stream identities, expired/foreign-video rejection, segment gaps, encrypted-media rejection and local generated H.264/AAC assembly. FFprobe confirmed the selected video height and audio; wrong-height output was rejected while inputs remained available. These are controlled fixtures, not evidence of a successful current YouTube capture.

The pipe checks include repeated replies, a client deliberately delaying its read, and cancellable idle shutdown. An actual native host process also passed stdio frame fragmentation and desktop pipe round trip. The initial UI test exposed premature pipe disconnection; the server now waits for client closure before teardown so its acknowledgment is not discarded.

The unchanged browser code passed **26 browser + 4 capture + 6 UMP checks**. No external URL extractor was invoked.

## Screen and file checks

The MFC main window loaded all ten records from a separate copy of existing history. Toolbar layout, category/file icons, Options save, Connection tab, Scheduler and File Info were inspected. A local browser handoff opened File Info, then real eight-worker progress and completion dialogs. The 33,555,163-byte result independently matched SHA-256 `13e38b5c3bd581857b66e94ac82b2c5c002bc9279f2f2cc8cc126f97a177d7f8`. The local server deliberately paced responses; its approximately 1.3 MB/s rate is not an Internet benchmark.

Private validation history, downloaded fixtures and toolchain caches are excluded from the portable archive. The previous C# source remains available separately from the active build path.

## Limits retained

Current live YouTube success, CDN/proxy matrices, live FTP, all DPI/accessibility states, large-file/disk-full/power-loss stress, store deployment and full IDM parity remain unverified or incomplete. Historical YouTube benchmarks used a different workflow and do not prove native capture performance.

The original WFP driver is separately development test-signed; it is not installed or kernel-tested. The latest diagnostics report Test Mode off and no running UDM driver. A prior elevated Test Mode attempt was canceled at the Windows elevation prompt. No reboot was performed.

The cached compiler's optional vctip helper caused repeated Windows errors. Its cached executable was disabled after stopping those helper processes; later builds launched no new helper processes. A previously queued Windows-owned error dialog may still need dismissal. No Windows system process was terminated and no system error policy was changed.

The corrected host subsequently acknowledged a second local browser handoff successfully. The final MFC progress view includes a file-piece map. The prior executables and pre-upgrade history were copied to a local rollback directory before activation.

Activation completed: release/UDM.exe is running with the original user history. All ten download IDs, names, statuses and received-byte counts match the pre-upgrade copy. The registered release/Udm.NativeHost.exe passed a real stdio/pipe round trip. The three active binaries match the tested staged binaries; hashes are recorded in native-release-sha256.json. The source-and-portable archive was inspected for required native sources/assets and excludes private validation state, downloaded fixtures, compiler caches, PDBs and certificate private keys.
