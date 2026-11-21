# Browser presentation workflow qualification — UDM 9.9

UDM 9.9 / source extension manifests 0.72.73 migrates the older default CapturePresentationUiTests workflow to the owned asynchronous recovery, acknowledgement, Resume and login interfaces. This increment updates the test driver and version metadata. Production behavior outside version identification is unchanged from 9.2.

## Window-driven state checks

Each owned window stores an integer fixture stage as a Win32 property, which disappears with that window. The driver ignores invisible/disabled windows, waits for prepared controls and submits an action only once. Ordinary and media Save races require a distinct asynchronous stale-review result before a second explicit Save. Cancel still stages a newer capture immediately before the click, then requires that undisplayed replacement to reopen in the next explicit presentation wave.

Displayed receipt acknowledgements drain through the real owned queue before each subsequent wave; this wait does not present later captures prematurely. Resume and authentication checks pump the actual fixture message loop until their owners have settled, rather than treating a returning click handler as completed work. A check immediately before the login decision rejects any early retry HTTP request; the completed file still requires exactly one authenticated retry and the original expected payload.

The network restart fixture binds the actual NetworkCaptureBackend through the current operations interface, constrains process roots to its own process, and checks failed restart observation, released monitor/capture ownership and durable partial-byte recovery without an elevated helper.

An independent ordinary record supplies the synthetic media-size states. The fixture verifies a visible viewport and permits bounded retries of the existing nonblocking snapshot. It also pumps and acknowledges its disabled Resume command before later history checks, rather than leaving the global decision guard held in an unpumped test thread. The old Start assertion is replaced with accepted individual-mode evidence plus queued/dispatched state, retaining the original identity and unconfirmed-state checks.

## Qualification

Build all **five native products as 9.9**, with source extension manifests **0.72.73**. Freeze 595 repository inputs and generated COM inputs. Recompile version-dependent core units and verify reused objects. The source audit identifies only the migrated test driver, native version header and two source manifests as changed from the frozen 9.2 build. [Build](validation/presentation-workflow-9.9/build.json), [product versions](validation/presentation-workflow-9.9/product-versions.json), [source audit](validation/presentation-workflow-9.9/source-change-audit.json).

Pass **327 optimized presentation-workflow checks** in the default test mode. This exercises real native dialogs, restart/review/cancellation, protected saved requests, Main and progress actions, locale-sensitive category controls, asynchronous authentication and complete authenticated loopback output. The native File Info show timestamp is a synthetic fixture observation, not an installed browser or IDM timing comparison. The driver runs on an owned isolated Win32 desktop, closes it after exit, never switches the personal desktop and injects no personal input. [Workflow assertions](validation/presentation-workflow-9.9/presentation-checks.json).

Pass **122 focused backend assertions across four C:/D: runs**: download-address recovery and network-session ownership. The unchanged production behavior retains the separately recorded [9.2 baseline](file-info-refresh-9.2.md) of 3,334 backend / 698 selected UI assertions; those counts are not relabeled as current runs. The 327-check /Od diagnostic preflight is excluded from optimized counts. [Current backend assertions](validation/presentation-workflow-9.9/backend-checks.json), [all diagnostic drafts and failures](validation/presentation-workflow-9.9/draft-preflights.json).

Remove **11 owned fixture directories and 32 temporary compiler/test files** after exporting evidence, including the failed preflights and displaced prior core objects. Logical bytes removed: 726,765,015; this is not a measurement of physical free space or drive wear. Keep current products/required core cache, prior 9.2 products, installed files, personal catalogs and media helpers. [Cleanup](validation/presentation-workflow-9.9/cleanup.json), [preservation](validation/presentation-workflow-9.9/final-preservation.json).

The private build record initially retained an older extension-version label from its driver template. Correct that label against both already-frozen manifest hashes, preserve the original record hash and export the correction; source files, binaries and test results are unchanged. [Metadata correction](validation/presentation-workflow-9.9/build-metadata-correction.json).

## Draft observations

Preserve every failed diagnostic attempt separately from qualification: 9.3 fails to compile against removed network-session members; 9.4 reaches the obsolete Queued-only Start assertion; 9.5 reaches the old modal-size assertion; 9.6 and 9.7 stop at list membership. The 9.8 diagnostic records decisionPending=true while Manager is unlocked and the visible, enabled main list is idle. Reading the caller identifies the unconsumed asynchronous disabled-Resume command. 9.9 acknowledges that command through its message loop and passes the 327-check diagnostic preflight. These preflights use /Od with verified 9.2 core objects and are not optimized-product qualification or performance benchmarks.

## Delivery and remaining work

The qualification covers this file's default presentation workflow; optional first-paint/live-browser fixture modes are separate and are not claimed here. Other backend/UI suites retain their previously recorded 9.2 baseline rather than being relabeled as newly run. Live installed browser capture, IDM timing/visual comparison, category/scheduling/caller waits and exceptional teardown remain open. Full inside/out IDM parity remains unproven.

Installed UDM remains 8.68 / extension files 0.72.32. No installer, installation, personal-desktop control, commit, push or release is included. Runtime browser JavaScript is unchanged.
