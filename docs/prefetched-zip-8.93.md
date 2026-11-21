# Prefetched ZIP readiness — UDM 8.93

UDM 8.93 / extension manifests 0.72.57 moves the active-prefetch ZIP readiness wait out of the preview window's pulse. The reader waits for the completed-prefetch receipt, checks the saved file on its worker, and revalidates ownership before returning a path. The dialog polls an owned future and can repaint or cancel while another operation holds Manager.

## Ownership and validation

`PrefetchedZipReadTask` uses one joined thread per open preview. Model-lock and receipt waits use a cancellable 10 ms wait. It accepts only a current member with an active transfer/reserved response and ConfirmationPending, matching the selected URL and protected headers and current ZIP eligibility. It snapshots the full private File Info request selection and rejects later changes, including protected browser proxy routing.

The existing version-1 completed-prefetch receipt must match URL, headers, Size and Received; replacement validation must have finished. Resolve the private parts path under Manager, release Manager, and verify the file's exact length on the worker. Reacquire Manager and recheck membership, activity, confirmation, selection, receipt, path and size. A changed request or stale/missing/mismatched file ends the preview without creating a replacement download. An incomplete receipt stays pending.

The returned path is internal. No protected request data is logged or exposed in controls. The task performs no admission, transfer stop, network request or catalog write. The existing local ZIP reader remains responsible for parsing the archive and displaying its directory; the readiness check does not replace archive validation. When the modal callback releases its final reader owner, cancellation wakes the wait and joins the thread before Manager can disappear.

Cancellation is bounded for the model/receipt wait. Windows file-size inspection itself is synchronous on the worker and cannot be interrupted by this cancellation flag; closing may still wait for an in-flight filesystem operation. No detached Manager reader is introduced.

## Qualification

All five native products report **8.93**, and both source manifests report **0.72.57**. Freeze 587 repository inputs and generated COM inputs before compilation. Bridge recompiles; the other 41 core objects, including Transfer, retain their verified 8.90 hashes. [Build](validation/prefetched-zip-8.93/build.json), [dependencies](validation/prefetched-zip-8.93/dependency-inventory.json), [generated COM](validation/prefetched-zip-8.93/generated-com-inputs.json).

Pass **3,088 backend assertions across 66 C:/D: runs**. The new reader contributes 21 checks per drive: complete and pending receipts, immutable terminal result, model nonmutation, request/header/proxy/membership/activity/confirmation/capture changes, mismatched file and receipt sizes, changed parts location, and joined cancellation during real model and receipt waits. A separate 21-check preflight is preserved but excluded from these counts. [Backend](validation/prefetched-zip-8.93/backend-checks.json), [preflight](validation/prefetched-zip-8.93/preflight.json).

Pass **634 actual UI assertions** (289 Main/File Info, 257 pending admission, 88 progress menu). The new production ZIP dialog scene opens while a real writer holds Manager, paints, handles its normal pulse and Cancel, and releases its last readiness-reader owner while the writer is still held. [Main UI](validation/prefetched-zip-8.93/ui-checks.json), [pending UI](validation/prefetched-zip-8.93/pending-ui-checks.json), [menu UI](validation/prefetched-zip-8.93/progress-menu-ui-checks.json).

Fixtures use isolated state and separate owned desktops without switching the user's desktop or injecting personal-session input. The reserved-response state is an explicit fixture state, not a live network transfer. These checks do not establish browser or IDM timing parity.

## Superseded builds and version accounting

8.91 / extension 0.72.55 introduced the reader, but the version update incorrectly encoded a UTF-8 marker as text and compilation failed. Restore the original encoding, parse the manifests and advance to 8.92 / 0.72.56. The new tests then failed compilation because they attempted to access Manager's private activity map. Replace that setup with the existing public reserved-response activity state, without widening Manager visibility or changing production activity semantics. Advance to 8.93 / 0.72.57, freeze fresh inputs and rebuild. Earlier attempts are unqualified and excluded from the primary counts. [Diagnostics and source hashes](validation/prefetched-zip-8.93/compiler-corrections.json).

## Cleanup and delivery

Remove **70 owned fixture directories and 80 temporary executable/object/resource files**, including the two failed attempts and current product entry objects. Preserve redacted evidence, five current products and the reusable core cache. Verify seven installed-file hashes, both personal catalogs and installed media-helper hashes remain intact. [Cleanup](validation/prefetched-zip-8.93/cleanup.json), [preservation](validation/prefetched-zip-8.93/final-preservation.json).

The pending UI fixture continues reusing the existing 8.83 alias to installed media helpers; no binaries are copied and no new alias is created. Automatic approval review previously rejected removing the alias as “blocked by policy”; no retry or bypass is attempted.

Installed UDM remains 8.68 / extension files 0.72.32. Browser JavaScript is unchanged. No installer, installation, commit, push, release or live IDM comparison is included.

## Remaining work

ZIP readiness still uses the existing UI pulse to observe its future; a direct completion wake remains open. The initial ZIP button handler still reads Manager before opening the child window. Non-prefetched ZIP preparation and prefetch stop/restart also retain synchronous work. This increment addresses the repeated active-prefetch readiness callback and its file check. Move the entry and non-prefetched paths onto owned commands next, retaining confirmation, cancellation and cookie-persistence authority. Caller restoration/admission, category/scheduling and final prefetch teardown waits also remain open. Full inside/out, visual and timing parity is unproven.
