# Published validation records

Exported on 6 October 2026 from local candidate records. These are historical observations with the scopes below, not a new release or deployment.

| Build or run | Outcome | Evidence |
|---|---|---|
| Post-RC1 Pause development candidate | 55 focused checks; 12 app checks; 50 completion checks | [Acceptance](pause-transaction/acceptance.json), [app smoke](pause-transaction/app-smoke/results.json), [completion fixture](pause-transaction/completion-portable/results.json) |
| Pause candidate full native suite | **2,546 passed, one failed** | [Result](pause-transaction/full-suite-result.json), [stdout](pause-transaction/full-build-tests.log), [stderr](pause-transaction/full-build-tests.stderr.log) |
| Same executable, scanner-focused rerun | 293 passed, zero failed; includes prerequisite checks | [stdout](pause-transaction/scanner-rerun.log), [stderr](pause-transaction/scanner-rerun.stderr.log) |
| R213 local installer | R212 app / R207 companions / browser 0.62.8; 143 package inputs verified | [Installer acceptance](r213/release-acceptance.json) |
| R213 native file deployment | Four native files replaced; catalog preserved; launch verified | [Sanitized deployment summary](r213/deployment-summary.json) |
| Current progress fixture, rebuilt 6 October | 174 passed using the tracked default reference | [Progress result](progress-portable/results.json) |

The full-suite failure was `Scanner result survives app restart`. Its scanner subprocess returned `0xC0000409` during manual rescan. The passing rerun does not erase that failure or establish a clean full-suite pass; its root cause remains unconfirmed. See [Pause investigation](../../backend-pause-transaction-20261005.md).

The Pause candidate is newer, uninstalled development work. R213 records refer to the separately identified R212 application and R207 companions, not deployment or installer acceptance of the Pause build. R213's installer was built but not executed or signed. The GitHub RC1 installer is a separate published artifact. No record here certifies full IDM parity or a clean-machine installer lifecycle.

Completion results exclude the unresolved opt-in missing-file modal diagnostic. Simulated DPI/control checks do not establish physical mixed-monitor or external shell/drag-and-drop behavior. The 174-check progress run tests the newly portable reference path; the backend and application suites were not rerun for this documentation/license change.

## Provenance and redaction

[manifest.json](manifest.json) maps each source record to its published copy and records SHA-256 hashes for both. Local workspace paths in logs become `<workspace>`; fixture-data roots become `<isolated-fixture-data>`; application paths become repository-relative and installer paths become filenames. The deployment summary omits the personal catalog count, local backup path, and runtime-evidence location. Synthetic localhost request paths and verification hashes are retained. Empty stderr files mean no stderr output was recorded.

Published evidence uses LF line endings for stable hashes. Source hashes identify the original local bytes; hashes inside the copied acceptance record describe that earlier workspace, including its original line endings, rather than the current documentation tree. `sourceBase` identifies the pre-change base and does not claim that the candidate was an unchanged build of that commit. Raw local catalogs, credentials, backups, binaries and screenshots are not included.
