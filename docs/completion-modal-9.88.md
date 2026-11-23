# UDM 9.88: completion waits for modal ownership

UDM 9.88 retains completion requests while File Properties or another modal disables the main owner. Both source extension manifests advance to **0.73.52**; JavaScript is unchanged. Five native programs are built. The candidate is **uninstalled**.

## Reproduction and implementation

Frozen 9.87 creates one completion while the actual Properties or ordinary modal owns a disabled main window, consuming the queued intent. The corrected baseline passes eight assertions reproducing this behavior. An initial fixture attempt failed before any assertion with “Could not write file”: the fixture had not created its private destination folder. Only the fixture directory setup was corrected; the failed run, corrected source and frozen product evidence are retained. [Baseline](validation/completion-modal-9.88/baseline/results.json) · [Initial setup failure](validation/completion-modal-9.88/baseline/initial-setup-results.json).

`pollCompletionWork()` now yields while main is absent or disabled, preserving the queue and readiness result. `OnEnable` posts the existing `TRANSFER_EVENTS` wake; transfer inbox events run before CLI Exit, including inbox-only notifications whose earlier wake was consumed by a modal. Re-disabling before the wake still blocks presentation. Close/layout priority, scanner/path validation and shutdown/save barriers remain. [Isolated patch](validation/completion-modal-9.88/completion-modal.patch).

## Qualification and build

The new maintained modal fixture passes **136 assertions across twenty light/dark scenarios** using actual Properties, ordinary modal and isolated capture owners. It covers ready intents, fresh inbox events, CLI exit and stale re-enable wakes without a main-timer tick. Current close-priority (88), exit-layout (20), admission (94) and reference (274) fixtures also pass: **612 UI assertions**. Five backend suites pass **246 assertions**, for **858 current checks**. These are focused checks rather than a full-suite rerun. [UI](validation/completion-modal-9.88/ui/results.json) · [Backend](validation/completion-modal-9.88/build/suite-results.json).

All **712 source inventory entries** were captured before compilation. Forty-two current core objects verify; Bridge rebuilds and 41 are reused. Five product hashes and PE versions, resource and two generated COM inputs verify. The normal App downloads 4,194,304 correct bytes, saves Complete with matching SHA-256 and exits normally; explicit output/catalog flushes succeed afterward. Its single smoke has 176.457 ms last-body-to-exit and 206.506 ms through both later flushes. It is a lifecycle check, separate from the paired experiment. [Audit](validation/completion-modal-9.88/final-audit.json) · [Normal product](validation/completion-modal-9.88/smoke/results.json).

## Fresh IDM comparison

One pair per owned D: case compares normal uninstalled UDM 9.88 and installed IDM 6.43.10.2. All six files match their expected SHA-256; UDM catalogs report Complete; all six observed app processes exit. IDM launcher exit code is 1 in each run despite correct outputs; its success-code semantics are not established. Keep that observation rather than silently treating it as code 0. The harness restores IDM's two connection settings. Other-session processes are left alone.

| Case | UDM / IDM total process time, s | UDM / IDM first-availability tail, ms | UDM / IDM process-exit tail, ms | UDM / IDM tail through explicit output flush, ms |
| --- | --- | --- | --- | --- |
| single-4m | 1.164 / 4.254 | 119.958 / 55.535 | 256.780 / 511.655 | 293.851 / 605.412 |
| single-64m | 9.049 / 10.320 | 223.983 / 70.170 | 315.998 / 599.479 | 361.096 / 1265.863 |
| split-64m | 2.373 / 3.924 | 289.826 / 177.780 | 380.308 / 502.207 | 414.390 / 1191.513 |

**First availability remains slower in UDM in every case.** Full process time and exit tails improve relative to IDM in this observation; this does not overturn prior losing runs or establish a repeated no-regression gate. The split case sends 70,455,049 bytes in UDM versus 68,260,087 in IDM for a 67,108,864-byte output: **3,346,185 versus 1,151,223 extra bytes**, with 9 versus 16 requests. Request count alone is not efficiency. [Raw results](validation/completion-modal-9.88/comparison/results.json) · [Assessment and scope](validation/completion-modal-9.88/comparison/assessment.json).

The timing includes observed application process/thread exit and a symmetric explicit output flush afterward. It does not measure GUI paint, every system service, device-cache persistence or power-loss behavior. There is one pair per case; do not call these samples a distribution or promote their single-value medians to acceptance. Payloads and isolated catalogs are removed after sanitized export.

## Cleanup and limits

After sanitized export, **259 files totaling 684,505,669 logical bytes** are removed from the build/fixture workflow. Five obsolete helper/object artifacts retire; current five products, resource, 42 referenced objects and the prior frozen App remain. The comparison separately removes six verified payloads totaling **276,824,064 bytes** and 26 private records/catalog files totaling **71,497 bytes**. Physical reclaimed space and disk wear are not measured. [Fixture cleanup](validation/completion-modal-9.88/cleanup-receipt.json) · [Comparison cleanup](validation/completion-modal-9.88/comparison/cleanup-receipt.json).

Installed programs/manifests, both personal catalogs and shared FFmpeg/FFprobe match the eleven protected hashes after both qualification and the fresh paired run. GUI first-paint timing, secondary modals leaving main enabled, address/icon/shell/armed-action persistence, current installed/browser acceptance and non-aligned live DASH ends remain open. The single-pair first-availability losses and extra split bytes remain explicit priorities in the updated architecture and execution plans. No installer, installation, UAC, commit, push or release occurs in this increment.
