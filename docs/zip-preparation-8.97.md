# ZIP preview entry and preparation — UDM 8.97

UDM 8.97 / extension manifests 0.72.61 opens the ZIP preview window before reading Manager or waiting for a prefetched transfer to stop. Preparation, selected stop, request/login construction and remote-probe launch run on an owned task. The existing virtual ZIP list and archive readers remain in use. Ordinary ZIP Preview is explicitly enabled again after admission settles, using its coherent eligibility view; the preparation worker still revalidates the click.

## Entry and ownership

The File Info button copies only owned UI inputs: current address, filename, checked metadata MIME and any edited credentials. It invalidates obsolete metadata work and opens the child. The parent pauses its advisory pulse while the modal child is active, so it cannot admit competing prefetch work during ZIP preparation.

The child paints its list, status and Cancel before starting `FileInfoZipTask`. Its model-lock wait is cancellable. It validates membership, the displayed address, ZIP eligibility and replacement state, captures the full private request selection and snapshots settings. Active HTTP confirmation without edited credentials reuses `PrefetchedZipReadTask`, issuing no replacement ZIP request. Other eligible requests prepare the remote path.

## Stop and launch authority

If confirmation must stop first, the task calls `Manager::endPrefetch` on its worker with the selected request. The optional selection guard is checked under Manager's own stop lock before any operation and again after waiting for an active transfer. No outer recursive lock is held while that wait releases the inner lock. Existing callers without a selection retain their established behavior.

After a stop, capture the current paused record and revalidate selection. Build edited login headers only on the private probe copy; edited login previews have no persistence callback. For unchanged login, retain the existing browser-session callback lease and session-equality checks. Revalidate current membership, selection and eligibility under Manager before constructing the bounded `ZipPreviewTask`. If the job has become active again, decline the remote probe rather than issue a competing request.

An absent or empty resolved-response address is the same selection when no redirect occurred. Nonempty resolved addresses remain guarded.

Settings remain a point-in-time snapshot. The selected-stop guard protects request identity; it does not freeze unrelated metadata or settings throughout the child lifetime.

## Cancellation and restoration

Cancel signals preparation and archive readers immediately. If an owned preparation or saved-file readiness read has not settled, the window shows a closing state and keeps repainting until its result can be consumed and joined. An already-started stop or durable save is allowed to finish. Its restoration intent survives cancellation. The child does not vanish while leaving a detached Manager operation behind.

Once the child returns, File Info requests restoration asynchronously using the captured selection. `FileInfoPrefetchTask` retains that guard for the same job across decision or capacity retries; a default retry cannot silently drop it. An explicit address refresh clears it, and a different job resets it. A changed request or routing context cannot restart from an old ZIP restoration request.

The existing HTTP/local archive workers retain their established bounded nonjoining cancellation model. Normal prepared-child close drains its Manager preparation and readiness reader; this does not claim that all HTTP I/O finishes before the window closes. Exceptional owner teardown still joins preparation to preserve Manager lifetime and can wait for a stop/save already underway.

## Qualification

All five native products report **8.97**; source extension manifests report **0.72.61**. Freeze 589 repository inputs plus generated COM inputs. Compile 1 current core revisions and reuse 41 matching cache objects. The reused set includes 2 native-qualified 8.96 revisions, 21 core-only 8.95 compilation outputs and 18 earlier verified objects; all are requalified by the current runs. [Cache provenance](validation/zip-preparation-8.97/cache-provenance.json). Transfer source is unchanged; its reused 8.95 object was rebuilt for the Core API change. All cache objects are requalified by the current runs; 8.95 itself is unqualified. [Build](validation/zip-preparation-8.97/build.json), [dependency inventory](validation/zip-preparation-8.97/dependency-inventory.json), [generated COM](validation/zip-preparation-8.97/generated-com-inputs.json), [product versions](validation/zip-preparation-8.97/product-versions.json).

Pass **3,170 backend assertions across 68 C:/D: runs**. The new task contributes 41 checks per drive, including empty no-redirect address equivalence and nonempty resolved-address rejection. Real authenticated HTTP responses parse as complete 22-byte empty ZIP archives. Private login edits preserve saved credentials; active reserved confirmation reuses its existing reader without a replacement probe. Changed request, proxy, capture requirements and membership reject launch. The positive path performs a durable stop, reads the remote ZIP, restores an unchanged real HTTP prefetch, receives its complete 22 bytes and stops without publishing. A default retry after decision deferral retains the selection guard and rejects changed routing. Cancellation during a held model lock and after a real durable stop preserves its stated ownership/restoration outcomes. A separate 41-check preflight is excluded from these totals. [Backend](validation/zip-preparation-8.97/backend-checks.json), [preflight](validation/zip-preparation-8.97/preflight.json).

Pass **651 actual UI assertions**: 306 Main/File Info, 257 pending admission and 88 progress menu. A new scene clicks the actual File Info ZIP button after a writer holds Manager, verifies child first paint, pulse, Cancel and return to the parent before releasing the writer, then joins the queued parent cancellation. Another scene holds an owned checkpoint after the real stop, verifies responsive pending close, releases the checkpoint and checks restoration intent and paused state. That checkpoint is a controlled operation barrier, not a measurement of real storage latency. [Main UI](validation/zip-preparation-8.97/ui-checks.json), [pending UI](validation/zip-preparation-8.97/pending-ui-checks.json), [menu UI](validation/zip-preparation-8.97/progress-menu-ui-checks.json).

Owned UI desktops are created and closed without switching the personal desktop or injecting personal-session input. These component and loopback checks do not prove live browser, exact final streaming size or IDM timing/visual parity.

## Superseded 8.94, 8.95 and 8.96

The initial 8.94 / extension 0.72.58 source audit found a restoration retry that would discard its selected-request guard. Stop its incomplete native build before products or suite qualification. Retain the guard per job, add a default-retry negative case and a complete positive stop/preview/restoration case, advance both versions and freeze fresh inputs for 8.95. The incomplete prototype is excluded from primary counts.

The 8.95 / extension 0.72.59 real-HTTP preflight completed 32 assertions, then its selected stop rejected a benign newly added empty resolved-address field. A direct response with no redirect writes `ProtectedResolvedUrl` as an empty string; it is equivalent to absence. Normalize only that empty response address, retain nonempty address selection, add both identity checks and a nonempty-address launch rejection, then advance to 8.96 and rebuild. The failed preflight is preserved and excluded from primary counts. Reuse only independently hashed core compilation outputs from the interrupted 8.95 build, with fresh dependency checks; no 8.95 products or suite qualification is claimed. [Failed preflight](validation/zip-preparation-8.97/superseded-preflight.json), [cache provenance](validation/zip-preparation-8.97/cache-provenance.json). [Source audit](validation/zip-preparation-8.97/prequalification-audit.json).

8.96 / extension 0.72.60 builds all products and passes 3,170 backend assertions, but does not qualify its Main UI. The first UI run stops at an existing duplicate-Resume fixture that expects its row after a single refresh. A fresh run of the same binary reaches the new ZIP workflow and finds that ordinary Preview remains disabled after admission. Preserve both failures. Restore the duplicate fixture's visible-owner precondition and allow its normal nonblocking history refresh to settle before issuing Resume; retain its real row assertion and the earlier busy/hidden-history tests. Explicitly enable ZIP Preview from current eligibility after admission, add setup/outcome diagnostics, advance to 8.97 / extension 0.72.61 and rebuild. [Superseded Main UI](validation/zip-preparation-8.97/superseded-ui-checks.json), [fresh same-binary observation](validation/zip-preparation-8.97/superseded-ui-rerun-checks.json), [superseded backend](validation/zip-preparation-8.97/superseded-backend-checks.json).

## Cleanup and delivery

Remove **146 owned fixture directories and 216 temporary executable/object/resource files**, including the interrupted prototype and current product entry objects. Preserve redacted evidence, all five current products and reusable core cache. Verify seven installed files, both personal catalogs and installed media-helper hashes remain intact. [Cleanup](validation/zip-preparation-8.97/cleanup.json), [preservation](validation/zip-preparation-8.97/final-preservation.json).

The pending fixture reuses the existing 8.83 alias to installed ffmpeg/ffprobe without copies or a new alias. Automatic approval review previously rejected removing that alias as “blocked by policy”; no retry or bypass is attempted.

Installed UDM remains 8.68 / extension files 0.72.32. Browser JavaScript is unchanged. No installer, installation, commit, push, release or personal browser/IDM comparison is included.

## Next work

Caller offer restoration/admission and duplicate handling can still wait before File Info opens. Address refresh, category/scheduling actions and final prefetch teardown also retain synchronous work. ZIP receipt and archive-result presentation still use the existing UI cadence after preparation; direct completion wake-ups remain open for those stages. Continue moving the remaining model operations onto owned commands, then compare cold/warm paint, field readiness, complete files and all background finishing work with IDM. Full inside/out parity remains unproven.
