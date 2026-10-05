# UDM browser handoff: durable download dialogs

**The staged candidate now restores the appropriate download dialog after reopening its catalog.** Accepted browser captures retain their File Info, existing-download or refresh presentation together with the accepted receipt. The candidate is tested but uninstalled; installed UDM remains native 0.74.0 / browser 0.47.1. This follows the [atomic admission work](browser-handoff-atomic-2026-09-29.md) and remains separate from COM 0.75.

Previously, admission could be safely recorded while its dialog choice existed only in memory. Reopening normalized a waiting job to Paused and could leave it without the expected prompt. The new journal records only the target ID and presentation kind; it does not duplicate URLs or credentials. The main window dispatches these saved presentations when it is enabled and retains them while another modal dialog is open.

File Info, duplicate decisions, existing unfinished/completed downloads and captured replacement addresses now restore their appropriate workflows. Duplicate choices retarget the saved presentation to the actual surviving record. Start, Later, Cancel, removal and successful address replacement consume the corresponding presentation with the catalog change. Failed saves preserve the outstanding prompt. Download Later and Skip File Info retain their existing quiet behavior. Unhandled presentations survive receipt aging; the accepted receipt remains even after its presentation is handled.

## Refresh-dialog race found and repaired

The first actual MFC regression reproduced a defect: a replacement address arriving immediately before Cancel was acknowledged despite never appearing in the dialog. That failed run is retained. The corrected dialog acknowledges only the receipt tokens it actually displayed, so the newer address gets another prompt.

Save now checks the current captured address and receipt group under the manager lock. If either changed since display, it updates the dialog and requires another reviewed Save instead of applying the stale address. The actual native fixture injects a second capture between display and button click, verifies the first Save leaves the original job unchanged, then verifies the reviewed second Save applies the new address.

[Failed reproduction](evidence-capture-presentation-20260929/capture-presentation-ui-race-before/results.json) · [Final native UI checks](evidence-capture-presentation-20260929/capture-presentation-ui-final/results.json) · [Reviewed Save dialog](evidence-capture-presentation-20260929/capture-presentation-ui-final/refresh-save-review-new-link.png)

## Verification

| Test | Result and scope |
|---|---|
| Full native suite | **2,309 passed, zero failed.** Includes the prior admission-crash checks and 39 presentation model checks: restart, duplicate retargeting, stale snapshots, later-token preservation, failed persistence, receipt aging and refresh. |
| Extension/native-host suite | **1,140 passed across 42 scripts.** Uses the candidate host. |
| Actual MFC windows | **31 checks passed.** Uses production MainWindow and dialog code, private state, real controls and rendered snapshots. Exercises disabled-owner deferral, File Info Start/Later/Cancel, duplicate choice, refresh Save/Cancel races, progress and completion, and no repeat prompt after handling/reopening. |
| Actual isolated Edge forms | **13 scenarios / 28 assertions passed.** Ten accepted cases preserve original POST bytes and output hashes; three unsupported cases stay in Edge. |
| Lost native commit reply | **Two actual Edge form cases / 10 assertions passed.** Recovery creates no duplicate output. |
| Public HTTPS downloads | **Six repeated W3C ZIPs / nine assertions passed.** Each completed once in UDM with Edge canceled; normal certificate validation remained enabled. This is ownership evidence, not the outstanding controlled recognition matrix or a speed comparison. |

The full native and extension suites passed before the last UI-only race correction. Core/native-host binaries were unchanged by that correction; the actual application and UI harness were rebuilt, then the final native UI and Edge tests passed. Focused checks are included in the full native count; repeated runs do not add feature coverage. Actual Firefox was not rerun for this revision; its retained live results belong to the preceding atomic candidate.

The GUI harness controls only its own windows. It reopens the private catalog rather than testing physical power loss. Its progress screenshot is Queued with zero transferred bytes; it is not a throughput test. The actual Edge fixtures separately verify transfer bytes and output integrity. Screenshots were visually reviewed for legibility and overlap, without claiming a complete IDM visual comparison.

## Retained state and remaining gates

All installed product, browser and network files still match their deployment receipts, apart from previously authorized research documentation. The personal catalog remains byte-identical with 25 download records. Independent cleanup verified nine fixture native-host locations absent, no matching test processes, and both prior test certificates absent from CurrentUser and LocalMachine Root. No certificate, driver, installed app, extension or personal download was changed.

The candidate still needs controlled recognition acceptance, a versioned paired native/browser package, and activation verification in the personal Edge profile. Legacy uncertain handoffs and changed/deleted refresh targets still need a complete review workflow. Wider media refresh, physical device/power-loss recovery, COM integration, full GUI/site coverage and the other research gaps remain open. No release version bump, installer or deployment is claimed.

[Summary and hashes](evidence-capture-presentation-20260929/summary.json) · [Source changes](evidence-capture-presentation-20260929/source-review.diff) · [Native results](evidence-capture-presentation-20260929/native-test-evidence.json) · [Edge form results](evidence-capture-presentation-20260929/presentation-edge-forms-1/results.json) · [Cleanup](evidence-capture-presentation-20260929/cleanup.json)
