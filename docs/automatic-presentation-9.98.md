# Automatic presentation respects dialog ownership — 9.98

Native source **9.98**, Chromium/Firefox source manifests **0.73.62**. This candidate extends the existing modal ownership fence to automatic presentation. All five normal native programs build; all 116 backend and 1077 current UI assertions pass, plus 20 separate frozen 9.97 App-source assertions. No installer, installation or release is included. Source/build status is separate from installed UDM and Chrome activation.

## Behavior

Browser offer reconciliation and immutable File Info preparation can complete while another owned dialog is open. Their presentation now waits behind that dialog. Authentication, legacy confirmations, capture reviews, browser and automation batches, update results, clipboard capture, recovery messages, quota warnings and browser-settings requests use the same admission check before consuming their presentation state.

The existing weak, generation-checked maintenance wake resumes deferred presentation after modal return. Ordinary release does not depend on the main timer. Current ownership and presentation context are checked again at dispatch; a stale enable notification cannot bypass an active reservation. The main frame stays hidden when the workflow started hidden.

Prepared media views retain their shared immutable operation until successful dispatch. Retirement checks the current map entry by identity, so a newer prepared operation arriving during a nested message loop survives the older dialog. Presentation failure retains the operation and waits for the existing timer fallback rather than scheduling a rapid wake/retry loop. Clipboard replay revalidates current monitoring preferences before offering a download; deferred update results retain ownership until dispatch succeeds.

No networking policy, prefetch bytes, size calculation, publication barrier or storage flush policy changes in this candidate. Both extension manifests advance for release accounting; no browser script changes are implied.

## Qualification scope

The new fixture exercises explicit pre-window reservations, an actual native message box, an actual MFC modal, stale-enable notifications, hidden main frames, reentrant prepared-view replacement and a deliberately thrown presentation failure, in both themes. Presenter callbacks observe dispatch and retained state without contacting a website or opening a personal catalog. The prepared-media operation is a typed component fixture, not actual browser admission or a completed media download. Browser-settings dispatch is intercepted; no real settings window is launched. Ordinary modal-release cases run with the main timer disabled; only the explicit failure case invokes its existing timer retry.

The frozen control uses the exact qualified 9.97 App source and the same fixture source with current shared headers/core objects. It independently reproduces automatic presenter dispatch during an owned modal. It is not a normal-product 9.97 timing comparison and makes no claim about IDM.

The first attempt passed its reserved-modal case, then failed to observe the next native dialog because real fixture shutdown left a quit message on that same fixture thread. The final fixture clears only that owned thread's quit message between scenarios. The earlier failure, input hashes and pre-retry source snapshots remain distinct from final qualification.

## Remaining gates

Actual installed browser capture, complete media and exact-size acceptance, real shell dispatch latency, foreign-thread/process window brokers and the storage finishing/IDM timing gate remain open. Current source evidence does not establish complete visual or backend IDM parity. There is no installer, installation or release in this candidate. Temporary data remains preserved because automatic approval review rejected the earlier cleanup action; no alternate deletion route is used.

[Sanitized qualification records and exact source snapshots](validation/automatic-presentation-9.98/). The verifier checks 731 source inputs, 42 core objects and 21 binaries. Only the version-bearing Bridge core object changes; all 9 protected installed files retain their recorded hashes.
