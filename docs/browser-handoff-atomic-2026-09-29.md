# Atomic browser admission and recovery — 29 September 2026

**The staged UDM handoff now saves the download record and its accepted receipt in one catalog replacement.** In the earlier candidate, a process failure between those writes could leave an uncertain admission. The new candidate recovered correctly through 41 forced process terminations, including duplicate and refreshed-link cases. It remains uninstalled and separate from the COM candidate.

Installed UDM remains native **0.74.0 / browser 0.47.1**. The prototype combines the previous prepared-handoff work with the latest uninstalled browser 0.49 source. Those version strings are base versions, not a new combined release.

## What changed

- Ordinary admission runs under the manager lock with intermediate catalog writes deferred. The final write publishes the job and accepted receipt together. A process exit before that write leaves the protected preparation retryable; an exit after it leaves an accepted receipt identifying the existing job.
- Storage or admission exceptions restore the previous catalog, job list, offers and mutable duplicate progress state. Cleanup of replaced partial files waits until the final catalog commit. Completed original files are retained.
- A prepared refresh stores its target identity, original URL and filename in the Windows-protected request envelope. Restart cannot silently convert that refresh into an unrelated new download. Changed or removed targets retain the preparation and report an error for review.
- A refresh opened after an ordinary request was prepared cannot redirect that request to a different download. The current UI refresh selection is preserved around admission.
- Earlier prototype uncertain/pending receipts stay fail-closed. The implementation does not guess whether an older interrupted admission succeeded.
- The native test build was repaired by allowing assertion helpers to accept generated string labels. No external message or environment flag can enable the C++ crash-observer callback.

[Source changes and diff](evidence-capture-atomic-20260929/source-review.diff) · [Admission implementation](evidence-capture-atomic-20260929/source/native/CaptureAdmission.cpp) · [Process/storage tests](evidence-capture-atomic-20260929/source/native/CaptureAtomicChecks.hpp)

## Verification

| Check | Result |
|---|---|
| Full native regression suite | **2,270 passed, 0 failed**, including the 311 focused atomic-admission checks. |
| Actual process termination | **41 cases:** new, numbered, existing, ask, replace-partial, replace-complete and refresh. Child processes terminate at deferred writes, before commit, after temporary-file flush or after commit. Tests reopen the catalog and retry the token. |
| Real storage rejection | **Seven scenarios** deny catalog replacement using a Windows file handle. State and original bytes remain intact; retry succeeds after the handle closes. These are storage-access failures, not physical disk-exhaustion tests. |
| Additional failure/intent checks | Three admission-exception boundaries, a post-commit lost-reply exception, late refresh selection and changed URL/filename/removed refresh targets. |
| Extension/component/protocol suite | **1,140 checks across 42 scripts passed.** Three scripts first failed to spawn required child processes inside the sandbox; those three passed when rerun outside it. Both attempts are retained. |
| Actual Edge forms | **13 scenarios, 28 assertions passed.** Ten native transfers preserve request bytes and output hashes; three unsupported cases remain in Edge. Cancellation is observed before each commit. |
| Actual Firefox forms | Same **13 scenarios, 28 assertions passed**, including cancellation ordering and payload protection. |
| Edge lost commit acknowledgement | Binary and redirect cases: **10 assertions passed**, no duplicate output. |
| Public HTTPS ZIP repetition | Six downloads: **nine assertions passed**. Each completed once in UDM with Edge canceled and matching output hashes. Normal TLS validation; no trust-store changes. |

Focused/repeated tests are not additional distinct feature counts. Earlier prototype successes are not substituted for these runs. The old failed build log is retained beside the successful rebuild log.

[Summary and source/binary hashes](evidence-capture-atomic-20260929/summary.json) · [Native results](evidence-capture-atomic-20260929/native-test-evidence.json) · [Focused crash log](evidence-capture-atomic-20260929/native-atomic-tests-1.log)

## Remaining release and parity work

The controlled HTTPS recognition matrix, combined packaging and activation in the user's Edge profile remain open. The older browser protocol still uses its previous handoff behavior; the new protocol requires a compatible native/browser pair. A complete UI for legacy uncertain receipts or rejected refresh preparations, restoration of every transient dialog choice after restart, and physical power/device-loss durability are not established by these tests.

The COM 0.75 candidate remains separate and its cold activation issue is unresolved. No new release version, installer, signature or app/extension deployment was produced here. Wider video/capture-rule/proxy/driver/GUI/performance gaps remain in the [research status](idm-research-current-status-2026-09-29.md). Full IDM parity is unproven.

## Isolation and cleanup

The deployed native application, browser and network receipt files were rechecked; only previously authorized research documentation differed from its original deployment receipt. The personal catalog remains unchanged with all 25 records. These tests used separate profiles, native instance tags and catalogs.

All four live fixture runs removed their temporary native-host registrations. A read-only check found all 12 inspected Edge/Firefox/Chrome key locations absent, no matching owned test processes, and both previously removed test certificates absent. Test profiles and fixture outputs remain as local evidence; they were closed, not deleted. No installed application, driver, browser setting, certificate or personal download was changed.

[Cleanup check](evidence-capture-atomic-20260929/cleanup.json) · [Collection/reproduction script](evidence-capture-atomic-20260929/collect.py)
