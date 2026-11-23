# Completion respects secondary-window modals — 9.96

Native source **9.96**, both Chromium/Firefox source manifests **0.73.60**. This is one desktop ownership improvement; the paired browser versions advance as requested. All five normal native programs are built. All **76 backend and 428 current UI assertions pass**, plus **six separate frozen-source counterexample assertions**. No installer, installation, push or release is included.

## Observed defect and resulting behavior

A native Windows message owned by a download's progress window disables that progress window while leaving the main frame enabled. The previous main-frame-only check can consequently present Download complete over the message, or allow a CLI completion exit before the secondary interaction finishes. Ordinary MFC dialogs often disable the main frame too; those are a separate, previously covered ownership path.

`CompletionModalFence` checks current owned top-level windows on the UI thread. A visible window's ownership chain must be enabled before completion work can proceed. This also protects a visible modal whose progress owner is hidden. Hidden disabled helper windows without a visible owned popup, such as Windows' IME helper, do not imply modality. Unrelated windows outside this frame's ownership chain cannot defer its completions.

The fence protects the transfer inbox, queued completion admission, completion-claim error review, file/queue completion actions and CLI completion exit. It retains pending work instead of consuming it while blocked. Repeated publication events still produce at most one eligible completion through the existing publication checks.

One scoped Windows message observer watches only the application's UI thread for owner re-enabling or destruction. It posts the existing generation-checked maintenance wake, carrying no job, control pointer or command. The receiver checks current ownership again; an enable immediately followed by disable cannot authorize presentation. Hook exceptions stay inside the callback. Successful shutdown detaches before frame destruction, with a destructor fallback. The observer remains attached across a failed shutdown so the rearmed signal can still wake deferred work. Multiple frame fences share the thread hook but retain separate owner roots. If notification installation/delivery fails, the live ownership check remains active and the existing main timer provides a retry.

This is neither a system-wide hook nor keyboard/mouse interception. It does not move catalog/storage work, alter completion durability, change external shell behavior or claim faster whole-download performance.

Microsoft documents the distinction between [thread-specific and global message hooks](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowshookexw) and the [ownership, visibility and disabled states of windows](https://learn.microsoft.com/en-us/windows/win32/winmsg/window-features). The implementation uses the current UI thread ID and a null module handle; no external-process observer is installed.

## Qualification

The owned UI fixture opens real UDM progress windows, real progress-owned MFC dialogs and File Properties, and the actual native-message branch of `themedMessage`. The native branch uses ordinary OK/Cancel in light mode and its supported Help-flag fallback in dark mode; the test does not claim that every dark-mode message uses Windows' native dialog. Both themes exercise pending inbox and already-ready intents, duplicate publication events, stale enable notifications, secondary release and deferred CLI Exit. The main timer is disabled, so successful resumption requires an actual release wake. An unrelated disabled top-level fixture is also checked.

The frozen counterexample uses the exact saved 9.95 `App.cpp` with shared headers/core objects, compiled as an isolated `/Od` UI fixture. It is an App-source control, not an installed 9.95 product or IDM performance comparison. It reproduces completion presentation while the native progress-owned message is still open. The current fixture defers that presentation.

Initial test failures are retained: the first ownership implementation incorrectly treated a hidden disabled IME helper as a modal; another fixture assumed ordinary MFC modality left the main frame enabled. Later fixture assertions incorrectly expected an absolute wake count after adding preceding cases, and expected the frame to be enabled while its own shutdown-error message was open. The corrected checks follow visible-window owner chains, compare a wake against its local baseline, and verify owner re-enabling after dismissing the error. No failed attempt is represented as a product pass.

Standalone native controls cover hidden helpers, hidden owners with visible modal descendants, nested release, owner destruction, unrelated roots, shared-hook routing, root disabling, detachment and throwing notifications. An injected UI-only shutdown failure verifies that the frame restores ownership and subsequently defers/resumes a secondary-modal completion. Existing completion ownership, durable-claim and file/queue countdown fixtures are rebuilt against the final source. The final build verifies 729 source inputs, 42 core objects and 15 binaries. Only the version-bearing Bridge core object differs from 9.95. All nine protected installed program/manifest/tool hashes remain unchanged. [Sanitized build, final suites, earlier failures, exact source snapshots and provenance](validation/completion-secondary-modal-9.96/).

## Remaining gates

Automatic folder/shell calls can still block their invoking thread; they are separate work. Manual Tasks → Exit and unrelated automatic prompt families need their own secondary-modal review; this increment gates completion-related CLI Exit. Native common dialogs or windows belonging to other processes/threads are not inferred to be modals from this UI-thread check. Whole-transfer finishing/writeback parity, installed browser capture, exact-size quality/container comparisons, visual timing against responsive IDM and the broader detail inventory remain open. Keep historical losing timing samples and unsupported-schema failures.

No temporary download payload is needed for this change. The owned fixtures create four-byte output files and small synthetic catalogs. Cleanup remains separate; no deletion route is attempted while the earlier automatic approval review's cleanup rejection remains in force.
