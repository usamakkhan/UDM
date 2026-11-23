# Manual Exit respects dialog ownership — 9.97

Native source **9.97**, Chromium/Firefox source manifests **0.73.61**. All five normal native programs are built. All **85 backend and 721 current UI assertions pass**, plus **23 separate frozen 9.96 App-source counterexample assertions**. The extended manual-close fixture itself passes 250 UI assertions, including a positively injected layout-preparation failure. No installer, installation or release is included.

## Defect and behavior

A dialog owned by a progress window can leave the main frame enabled. Window Close and Tasks → Exit previously entered close preparation while that interaction was still open. A previously prepared close-to-tray decision could also hide the main frame during an actual main-owned MFC modal. These are independently reproduced in the frozen source control without allowing it to destroy its active dialog: the fixture cancels the owned close/layout task before another UI message can advance shutdown.

`closeBlockedByDialog` now participates in window-ownership admission. Close-policy result consumption uses the existing strict disabled-owner check. The actual close path checks active modal ownership while retaining the distinction between an ordinary disabled main frame and the main-frame disable owned by an accepted layout-save operation. A blocked explicit close returns without closing; the user can choose Exit again after finishing the interaction. Completion-related CLI Exit retains its existing pending transaction.

## Ownership before creation and through teardown

Visible-window state alone is insufficient. A ready close can run while a native dialog-opening function is pumping messages before its HWND becomes visible. `CompletionModalFence::ModalScope` reserves an owned invocation on the UI thread before that call and retains it until the function returns. The shared Form modal entry, native themed-message entry, folder picker and Open with chooser participate. Nested scopes cannot release an outer invocation; exception unwinding releases the reservation. Final scope release posts the existing generation-checked wake to a waiting owning frame.

No new system-wide hook, input interception, credential, service or administrator permission is involved. Root ownership stays separate for multiple frames. Actual window-state checks remain available for dialogs outside those shared invocation wrappers; no blanket claim about foreign-thread/process modal brokers is made.

## Accepted layout shutdown and cancellation

The failed integration sample recorded both main and progress windows disabled during layout preparation, with no modal dialog open. Treating that disabled progress window alone as an active modal stranded normal shutdown. The modal query now combines an explicit invocation reservation with enabled visible popups in disabled owner chains; the original stricter query still protects completion presentation.

When an accepted layout close is canceled by a modal, main-owner restoration waits until that modal invocation ends. Restoring the main frame immediately could re-enable its progress windows while a native dialog still owned them. `exitLayoutRestorePending` retains this UI-owned release; the existing maintenance wake and timer fallback consume it when the modal is gone. Layout preparation failure uses the same release path. Captured prior enable state, current ownership and the existing final canonical checkpoint remain authoritative.

The fixture disables the main timer, exercises an already-ready layout result during native dialog creation, and verifies actual release-wake restoration afterward. It then forces ordinary asynchronous layout contention and follows the real shutdown to window destruction, one integration stop and the saved canonical layout/concurrent setting. No successful exit is inferred from a predicate alone.

## Qualification and provenance

The focused UI cases cover Window Close, actual Tasks → Exit command dispatch, close to tray, a hidden main frame, stale enable notifications, a ready policy decision, a ready layout decision and a real main-owned MFC modal. Both themes are used. A test-only validation extension adds the failed-layout/modal-release workflow and moves its expected-exception assertion outside the catch block; it preserves all five product binaries and all core objects. Native-message dark-mode coverage uses the shared helper's supported Help-flag fallback; this does not assert that all dark messages use the native branch.

The reference is the exact previously qualified 9.96 `App.cpp`, compiled with the retained same test source and shared current headers/core objects. It proves the three close-admission counterexamples, not normal-product 9.96 timing or IDM behavior. Existing modal, durable-claim, countdown, close-policy and layout-failure/retry fixtures are rebuilt against the current source.

Earlier rejected attempts remain distinct: a test-only protected-member compile error, a run against an already-existing output root, the disabled-progress false positive, and a native-dialog creation race. The fresh-root guard, explicit exit codes and final source/binary hashes prevent those attempts from being counted as successful qualification. Build outputs use D: and reuse existing shared core objects rather than copying the toolchain or media helpers to the nearly full C: volume.

The final receipt verifies **730 source inputs, 42 core objects and 19 binaries**. Only the version-bearing Bridge core object differs from 9.96. All nine protected installed file hashes are unchanged. Public JSON uses LF as required by Git; the two preceding unpublished evidence folders also have their metadata normalized and export digests recomputed without changing raw input digests or source snapshots. [Sanitized records, earlier rejections, exact source snapshots and final verification](validation/manual-exit-modal-9.97/).

## Remaining gates

Automatic folder/shell dispatch responsiveness, other automatic prompt families, window brokers outside this UI thread, normal installed browser capture and the existing storage-finishing/IDM timing gate remain open. This change does not remove durability barriers, shorten a measured flush, claim installed acceptance or establish overall IDM parity. Temporary fixture files remain subject to the previously rejected cleanup operation; no alternate deletion route is attempted.
