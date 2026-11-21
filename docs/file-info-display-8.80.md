# File Info advisory snapshots: 8.80 / browser 0.72.44

## Behavior and ownership

Regular File Info size, server-filename, progress and preview-visibility refreshes use an owned background snapshot reader. A busy catalog no longer makes these repeated reads wait on the window thread. Controls retain the last coherent size and filename while a fresh view is pending. A snapshot includes its job and login-draft context; a result from another context is not rendered. A removed job disables the editable draft and explains that the download is unavailable.

The worker projects display fields and track URLs/sizes instead of copying transfer journals, capture plans or protected request bodies. ZIP visibility receives only required fields and presence markers for protected/offline inputs. Its explicit action still captures and validates current authoritative state. The snapshot also computes download-preview and address-refresh eligibility away from the UI thread.

One lazy joined worker coalesces requests. Timer/input pulses request reads; task-completion pulses consume results without requesting another read. This distinction retains the existing 250 ms normal and 80 ms assembly cadences without introducing a completion-driven busy loop. Read failures retain the last good identified snapshot. Stop joins the worker before the model can be destroyed. Nested File Info owners guard main-window shutdown while a dialog owns reader work; closing an inner dialog does not release an outer owner's guard.

## Scope still open

This increment does not remove every File Info wait. Initial view capture/equality checking, prefetch admission and capacity retry, some fallback metadata initialization, explicit actions and `endPrefetch` still have synchronous paths. Start/Download Later retain their existing authoritative owned decision tasks. These remaining dependencies need their own admission, identity, cancellation and lifetime design; display snapshots cannot authorize a download.

The candidate is separate from installed UDM 8.68 / extension files 0.72.32. No installation, personal-browser automation, fresh IDM timing comparison, installer, release, commit or push is part of this increment. The 8.68 timing result is not evidence of 8.80 latency or complete IDM parity.

## Qualification

All five native products carry verified **8.80** resources. Both source manifests are **0.72.44**. The normal production configuration builds without private feature switches. Bridge recompiles for the version increment. Transfer and Streaming also recompile because the expanded dependency inventory newly accounts for local zlib headers; this is not a new performance comparison or proof that their binary hashes stayed unchanged. [Build and source identities](validation/file-info-display-8.80/build.json).

**2,648 backend assertions pass across 52 C:/D: runs.** The new display suite contributes 35 assertions per drive: coherent fields and authoritative eligibility, minimal protected/offline ZIP projections, held-model nonblocking requests/observation, immutable previous results, coalescing, failed-read retention, decision deferral, login-context refresh, removed-job handling, stop rejection, joined teardown and nested presentation ownership. The retained suites cover command decisions, network/owned Monitor peers, authentication, history, maintenance, signal ownership, writeback and publication/recovery. These are owned fixtures, not live browser or signed-driver validation. [Every backend assertion](validation/file-info-display-8.80/backend-checks.json).

The actual main/File Info/network/login/progress window fixture passes **183 assertions**. Thirteen new assertions enter the production File Info timer, hold the Manager mutex on another thread and verify that it returns while the writer remains held. Real static controls retain the old size and browser-reported filename, the dialog paints, and main Close is refused before acquiring the model. After release the controls converge to the matching new size and name. With the cadence timer stopped, completion wakes cause no additional reads. Actual Cancel closes the modal dialog, joins its reader, releases the ownership guard and leaves current metadata unchanged. A separate progress-menu fixture passes **88 assertions**, for **271 actual UI assertions** in total. Both fixtures create and close separate owned desktops without switching desktops or injecting input into the personal session. [Main and File Info UI evidence](validation/file-info-display-8.80/ui-checks.json), [progress-menu evidence](validation/file-info-display-8.80/progress-menu-ui-checks.json).

## Input provenance

The build freezes **574 repository-local input hashes before compilation**, including the quoted-include closure across repository directories, resource payloads and linked local libraries. The four dependencies recorded only after 8.79 compilation are now included before 8.80 compilation. The expanded traversal additionally finds the local zlib headers. This declared inventory is not a claim to hash the entire Microsoft compiler/SDK distribution. [Dependency traversal and recompilation decisions](validation/file-info-display-8.80/dependency-inventory.json).

Two generated COM inputs live outside the repository. They are separately recorded before product/UI compilation, after core compilation had begun, and verified unchanged afterward. The unresolved generated names in the repository traversal are accounted for by that separate receipt; they are not falsely labeled repository inputs or pre-core hashes. [Generated-input identities and timing](validation/file-info-display-8.80/generated-com-inputs.json).

Before the first UI compilation, inspection corrected a fixture-only input: an unconfirmed dialog uses `BrowserReportedFileName`; a GET-response filename requires the matching confirmation/response receipt. The fixture now supplies the browser-reported field for both old/new names. The original broad pre-build inventory hash of this unused test source is preserved. A recorded recursive dependency check proves no compiled product or backend entry depends on it; both UI builds capture its corrected hash before compilation. No failed UI run is counted or hidden. [Before/after fixture identities and dependency proof](validation/file-info-display-8.80/ui-fixture-correction.json).

## Cleanup and next work

After preserving compact redacted evidence, remove **54 owned fixture roots** containing **45,615,801 logical bytes**, **56 temporary test executable/object files** totaling **283,719,147 bytes**, and the exact **85,485-byte** temporary fixture-source copy. These are logical file lengths, not a physical-space or drive-wear measurement. Verify product/core hashes, all seven installed-file hashes and both personal catalogs before and after cleanup. [Cleanup](validation/file-info-display-8.80/cleanup.json), [final preservation](validation/file-info-display-8.80/final-preservation.json).

Next isolate prefetch admission/capacity retries and initial File Info preparation, preserving durable acceptance, displayed-request identity and joined cancellation. Then requalify the full browser-to-dialog/prefetch/exact-size workflow and compare real dialog presentation and complete transfer/background-work timings with IDM when personal-session control is authorized again. The current owned-desktop checks prove a repeated-display responsiveness improvement, not initial dialog latency or universal parity.

## Later qualification finding

The 8.82 investigation finds that the private per-object include scanner used here did not consume UTF-8 BOMs before matching a first-line quoted include. The recorded input hashes and actual passing checks remain their original evidence, but this traversal is not proof of exhaustive per-object invalidation. 8.82 corrects the scanner and rebuilds the affected units. The same investigation also exposes a stopped-queue/publication race with a deliberately held manual finisher. [8.82 findings and qualification status](independent-finishing-8.82.md).
