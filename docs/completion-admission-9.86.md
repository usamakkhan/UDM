# UDM 9.86: admit completion work without freezing the UI

UDM 9.86 removes blocking catalog acquisition from completion presentation and its ordinary transfer-notification handoff. All five native programs build as **9.86**; both extension source manifests advance to **0.73.50**, with JavaScript unchanged. This candidate is **uninstalled**. Overall IDM visual and finishing-speed parity remains open.

## Reproduced cause and architecture

The frozen 9.85 explicit completion request blocks for **547 ms** while another thread holds the catalog until a 500 ms watchdog expires. The real publication-followed-by-Finished callback path blocks for **563 ms**. Both eventually create the correct dialog. These are owned controlled contention cases, not fresh IDM timings. [Frozen controls](validation/completion-admission-9.86/baseline/results.json).

[App.cpp](../native/App.cpp) now retains ordered UI intents for Started, Published, Finished, explicit completion and ordinary completion handling. Each relevant metadata read uses `try_lock`. If the model is busy, that intent stays at the front and later work cannot bypass it. Exact duplicate pending intents coalesce. The ordinary uncontended path still executes immediately; no worker is created for an already-readable model.

[CompletionReadinessTask.hpp](../native/CompletionReadinessTask.hpp) owns one cancellable availability reader per main window. It waits off the UI thread with the established 10 ms condition-variable retry interval and posts an integer-only weak window wake after releasing the model. It consumes no completion action, changes no catalog and owns no window/control. The UI retries its ordered work against current eligibility, path, size and preferences. A failed, suppressed or removed publication cannot create a stale completion window. Cancellation joins without acquiring the catalog.

Published-before-Finished ownership is preserved, so final handling does not duplicate the already-shown dialog. A stale Started event cannot create a progress window after a completed download that should close progress. Update setup metadata is captured under the nonblocking notification read before its existing separate verification worker starts. Accepted owner shutdown cancels the readiness reader and clears remaining presentation intents only after Manager stops; a failed stop retains those intents for recovery. [Complete isolated delta](validation/completion-admission-9.86/completion-admission.patch).

## Readiness wake and CLI correction

Initial qualification passed 410 UI assertions. Review then found a finishing risk: after a deferred completion, the availability wake drained UI work but did not immediately retry `/q` Exit. The old exit-readiness check also took a blocking model lock. This could leave CLI shutdown dependent on the main timer.

The corrected App retries CLI completion from the actual readiness wake, uses a nonblocking exit check, and rearms the same cancellable availability reader if that check finds the model busy. It still requires the real inactive download, scanner readiness, final saved catalog and an enabled owner before posting Exit. Pending completion work prevents premature Exit. Four new light/dark cases qualify deferred Finished and a direct busy exit check **without any main-timer tick**. [Correction record](validation/completion-admission-9.86/cli-correction/correction.json), [isolated correction](validation/completion-admission-9.86/cli-correction/cli-correction.patch).

## Actual UI qualification and timing limits

The maintained [admission fixture](../native/CompletionAdmissionUiTests.cpp) passes **94 final assertions**. It uses actual production methods and consumes the owned worker's real integer wake through the main-window message handler. It does not manually poll admission to simulate the wake. It checks explicit and event dispatch, duplicate coalescing, current Unicode filename/size, actual Started → Published → Finished order, changed publication eligibility, cancellation, no replay and all four CLI wake cases.

| Theme | Request | Dispatch while writer held (ms) | Release to owned wake-handler return (ms) |
| --- | --- | ---: | ---: |
| Light | Explicit completion | 0 | 47 |
| Light | Published → Finished | 0 | 63 |
| Dark | Explicit completion | 0 | 63 |
| Dark | Published → Finished | 0 | 125 |

The writer is still held and its watchdog has not expired when dispatch returns. The second timing includes actual message dispatch, completion-window creation and subsequent work in that owned handler; it is not a measurement of natural first paint, nor a paired IDM comparison. The clock is coarse. A zero dispatch observation does not mean zero-latency dialog display or satisfy a one-millisecond parity guarantee. All observations are retained. [Final checks and observations](validation/completion-admission-9.86/ui/admission-results.json).

The retained-reference completion fixture passes **274 final assertions** against the corrected App, including actual suppression-failure rollback, shared preferences, file actions, Enter behavior, early publication/CLI gates, reference geometry, DPI and scanner diagnostics. Thus **368 UI assertions** use the final App source. The earlier 24 suppression and 42 file-action contention checks are preserved as **66 pre-CLI-correction controls**, not counted as fresh final-source executions. Their controlled code paths were not changed by the CLI correction. [Final reference checks](validation/completion-admission-9.86/ui/reference-results.json), [preceding controls](validation/completion-admission-9.86/pre-correction-ui/results.json).

The new readiness task suite passes **13 assertions** for contention, accepted-decision ownership, cancellation, duplicate rejection, model unlock, notification failure, retry and no writes/action consumption. The existing suppression task (17), completed checkpoints (145) and publication availability (52) also pass: **227 backend assertions in four suites**. Both task suites are registered before this build. [Backend result](validation/completion-admission-9.86/build/suite-results.json).

## Normal-product download and full application exit

The unchanged normal final App downloads **4,194,304 bytes** from an owned loopback origin with an isolated catalog. The file's SHA-256 matches the generated body and the saved Complete record. The App exits normally with code 0 and all its threads terminated, then explicit output and catalog flushes succeed. There is no force-stop. One observed request transfers exactly the expected body. [Normal run and complete metrics](validation/completion-admission-9.86/smoke/results.json), [origin records](validation/completion-admission-9.86/smoke/origin.json).

This is a lifecycle smoke, not a speed ranking. Process-exit tail is 610.213 ms in this single run; the record separately includes the final explicit file/catalog flushes. It does not measure global system services or storage-controller caches. IDM is not launched and its settings are not read or changed. The temporary output is deleted after evidence export.

## Build provenance and cleanup

The final inventory has **710 entries**, 42 verified core objects, five verified product hashes/PE versions and the resource. Bridge is rebuilt for the version increment; 41 other core objects are reused. The first UI-fixture amendment upgrades the not-yet-compiled test to exercise the actual wake. The later CLI correction changes App and that fixture; only the affected App product is rebuilt, and admission/reference UI fixtures are requalified. The four unchanged helpers and backend results are retained. Earlier observations remain explicitly separated.

An initial correction-driver hash guard rejected normalized LF text against the original App's CRLF bytes before any correction compilation. The old raw bytes were reconstructed and verified against their original recorded SHA-256; the final inputs were captured before the corrected compiles. The full inventory is not claimed to have been frozen before every compilation. [Capture history](validation/completion-admission-9.86/build/source-inputs.json), [current build](validation/completion-admission-9.86/build/build-result.json), [final audit](validation/completion-admission-9.86/final-audit.json).

After sanitized export, **200 individually path/type/hash-checked files totaling 747,037,298 logical bytes** are removed. This includes the 4 MiB output, private catalogs, fixture programs, source snapshots, rendered images and disposable objects. Five superseded helper/object artifacts are retired. Current five products, resource, 42 objects and the frozen 9.85 App remain. Installed programs/manifests, both personal catalogs and shared FFmpeg/FFprobe retain all eleven protected hashes. Physical space reclaimed is not measured. [Cleanup receipt](validation/completion-admission-9.86/cleanup-receipt.json), [export hashes](validation/completion-admission-9.86/export-manifest.json).

## Remaining gates

This removes model-acquisition waits from the covered completion path. Address decryption, icons, filesystem/shell operations and armed completion-action persistence can still consume UI-thread time. Active progress-window construction is a separate route. Next, qualify a main-window close arriving during deferred completion and keep existing close/layout decisions ahead of presentation retries. A fresh installed/browser acceptance and responsive IDM reference are still needed; no new IDM timing comparison is claimed from the earlier unresponsive process. Previous sequential/split speed losses and non-aligned live DASH ends remain open. No installer, installation, administrator prompt, commit, push or release occurs in this increment.
