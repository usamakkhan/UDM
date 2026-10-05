# Candidate native 0.75.0: Windows download automation

**Staged, not installed.** UDM-owned COM interfaces and a native batch-review workflow are implemented and compiled. **Cold COM activation still fails with `REGDB_E_CLASSNOTREG` (0x80040154), so research gap R06 is not closed.** Installed UDM remains native 0.74.0 / browser 0.47.1.

This follows the single-link and reviewed-batch workflows in [IDM's published API](https://www.internetdownloadmanager.com/support/idm_api.html), using independent UDM interfaces, registration identities and implementation. No IDM component or registration is replaced.

The candidate exposes `UDM.LinkTransmitter.1` / `UDM.LinkTransmitter`, with `SendLinkToUDM`, `SendLinkToUDM2` and `SendLinksArray`. The first two accept a URL, referring page, cookies, POST text, credentials, destination folder/name and flags. The second also accepts a user-agent override and an empty reserved argument. Flag 1 suppresses per-file confirmation/progress/completion dialogs; flag 2 leaves the new job paused in the queue. Silent calls use numbered destinations and preserve existing active work.

`SendLinksArray` accepts a two-dimensional string/variant array with URL, cookies, description and first-row user-agent columns. The desktop shows a real **Download all links with UDM** dialog with selection, Check all / Uncheck all, queue, destination, Download later, OK and Cancel. Pending batches are encrypted. Accepted row attribution is committed with each new job, allowing review continuation after restart without re-adding those rows. Cancelling the review discards unaccepted offers.

The local pipe reuses UDM's transfer, duplicate and protected-storage paths. The browser native host explicitly rejects automation-only commands. Validation rejects unsupported flags, reserved values, malformed arrays, unsafe file names, relative folders, header injection, embedded NULs and oversized POST data. The desktop startup path is exercised against the actual candidate UDM executable in a private catalog.

Completed passing runs contain **2,006 checks**, with the following scope. Failed/intermediate runs are retained and excluded from that count:

| Suite | Passing checks | What was exercised |
|---|---:|---|
| Full native regressions | 1,931 | Existing native functionality against the changed Core/Bridge; process exited successfully. |
| Actual COM calls | 32 | Typed and IDispatch calls to an explicitly started out-of-process server; authenticated POST, exact output bytes, one POST without ranges, protected persistence, batch continuation and invalid inputs. |
| Native batch dialog | 23 | Actual owned controls, selection, destination, later/cancel/close behavior; two rendered screenshots reviewed. |
| Native-host protocol | 12 | Framing regressions and rejection of all three local-only command types. |
| Actual desktop startup | 8 | Running COM server starts the actual desktop into a private catalog, creates one paused job, and cleans up its own processes/registration. |

Visual review caught a stale selection count before the periodic refresh. Checkbox changes now immediately update the count and OK button; five additional real-control checks pass. The actual desktop was rebuilt after this UI-only correction. The earlier native/COM/desktop suites were not redundantly rerun.

The first transfer fixture did not tick the scheduler and was corrected; the passing rerun verifies the real request. Sandbox runs of the UI/full native suites were denied atomic replacement of their private state files; authorized reruns passed. None of those failed attempts was counted as a passing result.

**Activation remains the blocking release defect for this candidate.** The registration is visible in HKCU and the merged HKCR view, and manually starting the registered server enables actual marshalled calls. Cold activation failed in the typed client, a bounded retry, and a fresh PowerShell client. The cause is not established; this is not attributed to Windows policy or the sandbox without evidence. The registration structure was checked against [Microsoft's LocalServer32 documentation](https://learn.microsoft.com/en-us/windows/win32/com/localserver32). These narrower successes do not establish automatic COM launch.

Follow-up work now adds machine-wide registration, installer setup/uninstall hooks and separate 32-bit/64-bit type libraries. The installer compiles; actual 32-bit and 64-bit dispatch to a running server passes. Cold activation remains unresolved, and the machine-wide acceptance attempt was canceled at the Windows administrator prompt. Further work includes real installation/repair/uninstall, wider 32-bit calls/batches, live IDM comparison and combined browser qualification. [Registration follow-up](automation-registration-2026-09-29.md). The 1 MiB POST limit and 1,000-row / 512 KiB batch limit remain explicit. Explorer shell workflows are still separate outstanding work. This is progress on R06, not full COM/shell parity.

The obsolete recognition HTTPS helper was stopped after its two-hour certificate expired while consent remained pending. The exact certificate was absent from the trust store, and independent cleanup verified that absence. Its tests did not complete. No replacement helper or certificate was created. Recognition acceptance still remains open for browser 0.48/0.49; certificate cleanup is no longer outstanding.

Installed files still match their receipts apart from the authorized research report. All 25 personal download records and queues remain identical. The state file itself changed only in the saved floating-basket X/Y position; the producer of that change was not established and the position was preserved. [State comparison](evidence-candidate-0.75.0/state-review.json). Temporary COM class, interface, type-library and ProgID registrations have been removed. The source changes, failed attempts, passing runs, compiled binary hashes and cleanup receipt are retained as evidence.

[Summary](evidence-candidate-0.75.0/summary.json) · [source diff](evidence-candidate-0.75.0/source-review.diff) · [COM results](evidence-candidate-0.75.0/acceptance-running-2/results.json) · [native dialog results](evidence-candidate-0.75.0/ui-3/results.json) · [desktop startup](evidence-candidate-0.75.0/desktop-start-1/results.json) · [certificate cleanup](evidence-candidate-0.75.0/trust-cleanup-1.json)
