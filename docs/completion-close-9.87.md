# UDM 9.87: finish close decisions before presenting queued completion

UDM 9.87 restores close/layout priority over the deferred completion queue introduced in 9.86. Five native programs build as **9.87** and both extension source manifests advance to **0.73.51**; JavaScript is unchanged. This candidate is **uninstalled**. Complete IDM behavior, appearance and speed parity remains unproven.

## Frozen reproduction and fix

With publication retained behind a held catalog, the actual main-window Close starts its owned close-policy reader. Once the catalog is released and both readers are ready, frozen 9.86 drains completion before applying the ready hide-to-tray decision. Both the real worker-wake route and the timer route create **one completion dialog in that first close dispatch**. The main window is eventually hidden, but close did not take priority. [Frozen controls and observations](validation/completion-close-9.87/baseline/results.json).

In [App.cpp](../native/App.cpp), both dispatchers now service existing close-policy/layout decisions before completion retries. The admission pump itself also yields while either decision or its prepared-layout handoff is pending; direct notification/admission calls cannot bypass the ordering. No completion intent is consumed merely because a close decision is pending.

Cancelled and failed close reads, failed layout preparation, blocked close restoration and hide-to-tray paths request an integer-only owned completion wake after restoring their state. Thus retained work can resume without a main-timer tick. A completed Exit still stops Manager and integration through the existing path, clears presentation intents after Manager stops and creates no pending completion window. No shutdown flush, save, join or publication barrier is removed. [Isolated implementation and maintained test delta](validation/completion-close-9.87/completion-close.patch).

## Runtime qualification

The new [close-priority UI fixture](../native/CompletionClosePriorityUiTests.cpp) passes **88 assertions** across twelve light/dark cases. It uses actual production Close/timer/wake/admission methods and its own catalog/windows. First close-decision dispatches create **zero** dialogs. The fixture separately calls admission directly while close preparation owns the request, so dispatcher order alone is insufficient to pass.

| Case | Current observed result |
| --- | --- |
| Actual Close, ready timer decision | Main list hides first; a subsequent owned wake shows one retained completion |
| Actual Close, real worker wake | Same priority and one completion; the main list stays hidden |
| Cancelled close read while writer held | Cancellation is acknowledged without waiting for the writer; publication remains retained |
| Failed close-policy read | Owner remains usable, no completion in the failure dispatch, and retained work resumes on a wake |
| Failed exit-layout merge | Input/ownership are restored without stopping integration; completion can resume |
| Actual accepted Exit with pending publication | Integration stops exactly once, no completion is created, concurrent settings and the catalog entry remain saved |

No replay is observed on subsequent polls. These are UDM ordering tests, not an observed IDM comparison. [All new checks](validation/completion-close-9.87/ui/close-priority-results.json).

The existing **20 exit-layout UI**, **94 completion-admission/CLI wake**, and **274 completion-reference UI** assertions pass against the same current App source, for **476 current UI checks**. They retain layout failure/retry and direct-destruction ownership, actual wake-driven CLI Exit without timer ticks, saved-catalog/scanner gates, current paths, suppression rollback, shared preferences, file actions, DPI geometry and scanner diagnostics. [All UI runs](validation/completion-close-9.87/ui/results.json).

Five focused backend suites pass **246 assertions**: exit-layout task (19), completion readiness (13), suppression settings (17), completed checkpoints (145) and publication availability (52). [Backend results](validation/completion-close-9.87/build/suite-results.json).

## Normal download, reference availability and timing limits

The normal App downloads **4,194,304 bytes** from an owned origin into a private catalog. File and saved-catalog SHA-256 agree with the generated body, the saved record is Complete, the App exits normally with code 0, and explicit output/catalog flushes succeed afterward. One request transfers the expected body. The observed last-body-to-process-exit tail is **248.327 ms** in this single smoke; all measurements, including the later flushes, remain in the record. This is lifecycle acceptance, not speed promotion or a claim about global OS/device-cache work. [Normal-product result](validation/completion-close-9.87/smoke/results.json), [origin](validation/completion-close-9.87/smoke/origin.json).

A fresh supported desktop attempt lists windows and opens/captures IDM 6.43 in the confirmed current Usama session. Windows initially reported the background process responsive. After main-window inspection and a Tasks-menu click, the menu does not react and Windows reports the main window unresponsive. Its cause is not established. No process is forced to stop, the other Windows-session process is left alone, and no paired transfer is launched. Private history/screenshots are not exported. [Reference observation](validation/completion-close-9.87/desktop-observation.json).

## Provenance and cleanup

The current inventory verifies **711 entries**, 42 core objects, generated COM inputs, five product hashes/PE versions and the resource. Bridge is rebuilt for the version increment and 41 other core objects are reused. The build execution handle disappeared after two backend targets linked. Authoritative inspection found no compiler, linker or build driver remaining. Their hashes and 9.87 resources verify, so only unfinished targets resume; all five backend suites then execute normally. [Interruption and reuse record](validation/completion-close-9.87/build/interruption.json), [current build](validation/completion-close-9.87/build/build-result.json).

Before its first UI compilation, the new fixture gains a direct-admission priority assertion. This does not change production/backend inputs, and its old bytes exactly match the frozen baseline fixture hash. The metadata distinguishes the initial product inventory and the final fixture amendment; it does not claim every final entry was frozen before all compilation. [Capture record](validation/completion-close-9.87/build/source-inputs.json), [final audit](validation/completion-close-9.87/final-audit.json).

After sanitized export, **189 individually path/type/hash-checked files totaling 529,468,823 logical bytes** are removed, including the output, private catalogs, source snapshots, fixture programs, rendered images and disposable objects. Five superseded helper/object artifacts are retired. Current products/resource/42 objects and the frozen 9.86 App remain. A fresh protected baseline before runtime qualification matches the earlier eleven reference hashes; installed programs/manifests, both personal catalogs and shared FFmpeg/FFprobe remain unchanged. Physical space reclaimed is not measured. [Cleanup receipt](validation/completion-close-9.87/cleanup-receipt.json), [export hashes](validation/completion-close-9.87/export-manifest.json).

## Remaining work

Next qualify deferred completion while File Properties or another ordinary modal window owns a disabled main window, then profile remaining address/icon/shell and armed-action persistence work. Active progress construction, installed/browser acceptance, a responsive IDM reference, previous sequential/split speed losses and non-aligned live DASH ends remain open. These results do not prove complete inside/outside parity. No installer, installation, administrator prompt, commit, push or release occurs in this increment.
