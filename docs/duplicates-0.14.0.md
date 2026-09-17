# UDM 0.14.0 — duplicate downloads and recoverable replacement

## Behavior

Adding a regular file address already in history now offers three choices:

- **Show existing / resume saved parts** opens the completed file's Properties or lets the existing incomplete job continue through File Info. Active downloads reuse their existing task.
- **Numbered copy** creates another file without changing the existing download.
- **Replace completed file** downloads and verifies the new bytes first, then publishes them at the original path. The original remains available in history as a numbered previous version in the same folder. Replacement is explicit for each file and cannot be selected as an automatic default.

The dialog applies to Add URL, clipboard/drop entry, batch/import and regular browser handoff. Options > Duplicates offers Ask, numbered copy and existing download policies; the dialog can remember the two non-replacement choices. Repeated browser handoffs reuse a pending choice instead of generating duplicate dialogs. Pending duplicate decisions cannot be bypassed by prefetch, Resume or Start queue.

Matching normalizes the URL host, default port and fragment while retaining path case and the exact query string. Cookies and Authorization must match. Different signed URLs and account identities remain separate. Expired-link recovery continues to use Refresh download address.

## Replacement integrity

The engine stages new bytes separately and checks the original's SHA-256 again before publication. A changed, missing, locked or relocated original blocks replacement. Previous-version paths are reserved against other UDM jobs. HTTP rejection, cancellation and checksum failure do not overwrite the completed file. A failed history save rolls the file operation back while keeping the downloaded staging file. A persisted journal recovers interrupted states.

Publication uses Microsoft's [ReplaceFileW API](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-replacefilew) with a same-volume backup. UDM preserves that backup as another completed record. Tests exercise Windows file replacement and filesystem failures on disposable files.

## Validation

- **194 native checks passed, 0 failed**, including 39 new duplicate/replacement checks. These cover signed-query and credential separation, active/pending handoff reuse, queue/prefetch gating, policy validation, saved-prefix HTTP resume with exact SHA-256, original edits, existing backup files, locked files, state-write rollback, HTTP rejection, checksum mismatch, segmented replacement, and three recovery states.
- **30 browser integration checks passed**, including acknowledgment-before-cancel, failed-handoff fallback, source validation, media identity/ad rejection and selected-link capture.
- Live native UI checks confirmed all three choices, a numbered copy's File Info destination, Download Later, and the remembered preference in Options. These used isolated state and disposable files.
- The initial native test run exposed two fixture snapshot assertions that included an unrelated scheduler metrics update. Capturing the baseline immediately before the tested operation corrected both; the complete suite then passed.

See [machine-readable evidence](reference/duplicates-0.14.0.json). The complete native log is in benchmarks/duplicates-0.14.0/native-tests.log.

## Deployment and remaining work

The native app, host and monitor are version 0.14.0. Browser JavaScript remains 0.13.0 with the same native-message protocol; this desktop duplicate workflow requires no extension update. Chrome's previously noted separate Windows session still limits live user-browser review.

The release does not establish Internet speed parity with IDM. Periodic synchronization queues, complete toolbar customization, universal live-platform capture and a production driver remain unfinished. Replacement currently applies to regular HTTP/HTTPS files, not captured adaptive media or FTP. Historical 0.12.2 audit counts are not a current feature-completion total.

Source and binary backups are under backups/before-0.14.0. A predeployment user-history snapshot is under benchmarks/duplicates-0.14.0/history-before.json.

Final deployment verification: all 10 user download records are exactly unchanged (7 complete, 3 failed). The deployed app opens with 0 active transfers, and its native messaging host returned a valid length-prefixed ping acknowledgment.
