# Prepared browser handoff — 29 September 2026

**A staged UDM implementation now waits for confirmed browser cancellation before native admission.** The public W3C ZIP that previously completed in both Edge and UDM passed six repeated downloads with matching output hashes: all six completed once in UDM, with Edge canceled. This is evidence for those tested races, not a claim of universal ownership or IDM parity.

Installed UDM remains native **0.74.0 / browser 0.47.1**. This candidate uses the 0.74 native source plus the latest uninstalled 0.49 filename/form revision. It is separate from the staged COM 0.75 work. Its version strings have not been promoted to a new release, and no installer was built or deployed.

## Behavior

1. The extension validates the paused browser download and asks the desktop to prepare the request.
2. UDM validates ordinary HTTP/HTTPS request metadata and saves the request with Windows DPAPI protection. Preparation creates no download job, file-info offer or network transfer.
3. The extension checks that the original browser item still belongs to this handoff, requests cancellation and observes its final state. A successful cancel callback alone is insufficient: the item must be interrupted with `USER_CANCELED`.
4. If the browser completed first, the extension releases the prepared request. Otherwise, it saves the cancellation proof and commits the prepared request.
5. UDM admits that request through its existing ordinary-download flow and persists the accepted job identity. Lost replies query the durable status; accepted requests are not replayed.

Release barriers reject delayed preparation/commit. Protected pending requests are bounded to 128 entries and 8 MiB of encrypted request storage within the existing catalog limit. Accepted/released receipts discard the extra encrypted request copy. Unresolved prepared/admission records are not silently aged away. The browser journal contains only opaque identity, phase and protocol metadata, not URLs, credentials or request bodies.

The desktop advertises `captureTransaction: 1` separately from the legacy recovery protocol. New operations are `capture-prepare`, `capture-status`, `capture-release` and `capture-commit`. Existing browser clients continue using the old protocol; both updated components are required for the new guarantee. Manual Add and media-panel commands keep their existing flows.

[Native protocol](evidence-capture-transaction-20260929/source/native/CaptureTransactions.hpp) · [browser protocol](evidence-capture-transaction-20260929/source/browser/chromium/capture-recovery.js) · [complete source diff](evidence-capture-transaction-20260929/source-review.diff)

## Verification performed

| Verification | Result and scope |
|---|---|
| Native build | C++/MFC app, host, monitor and tests compiled successfully. Existing compiler warnings remain. |
| Full native regression suite | **1,959 passed, 0 failed**. Includes 28 new transaction checks. |
| Full extension/component/protocol suite | **1,140 passed across 42 scripts**. Includes 39 new transaction checks across Chromium and Firefox. |
| Focused native receipt tests | 46 passed; included in the full native suite, not additional coverage. |
| Actual Edge form matrix | 13 scenarios, **28 assertions passed**. Ten native transfers preserve original request bytes/output hashes; three unsupported submissions stay in Edge. Each native commit is observed after cancellation of its corresponding browser URL/final URL. |
| Actual Firefox form matrix | Same 13 scenarios, **28 assertions passed**, including Firefox's zero-byte pause behavior and cancellation ordering. |
| Actual Edge lost prepare reply | Binary and redirect scenarios: **10 assertions passed**; one acknowledgement deliberately lost after native preparation. Recovery commits one job without resending preparation. |
| Actual Edge lost commit reply | Binary and redirect scenarios: **10 assertions passed**; one acknowledgement deliberately lost after native admission. Recovery identifies the accepted job without another transfer. |
| Repeated public HTTPS ZIP | Six requests, **9 assertions passed**, with normal TLS validation. All six completed once in UDM; Edge remained canceled; output hashes match the retained reference ZIP. These tests check ownership and do not replace the separate controlled recognition matrix. |

The actual-browser observers record browser state immediately before native commitment, correlated by the prepared URL and the browser's original/final URL. Fixture registrations and native diagnostics identify private test catalogs. The lost-reply cases inject a response failure after the real native operation, not a replacement native implementation.

Two earlier failed runs remain in the evidence. The first smoke run timed out before native connection while the build was still completing its assets; no fixture download began. The next smoke passed after the complete build. The first full Edge run passed the per-file outcomes but its final observer assertion compared only original URLs, incorrectly rejecting a redirected URL. After adding final-URL correlation, the full matrix passed. Failed-run assertions are not counted as complete acceptance.

[Native results](evidence-capture-transaction-20260929/native-test-evidence.json) · [unit results](evidence-capture-transaction-20260929/unit-results.json) · [Edge results](evidence-capture-transaction-20260929/transaction-edge-forms-2/results.json) · [Firefox results](evidence-capture-transaction-20260929/transaction-firefox-forms-1/results.json) · [public ZIP results](evidence-capture-transaction-20260929/transaction-edge-public-1/results.json)

## Remaining work before release

- **Native admission is still not an atomic catalog transaction.** If a process/storage failure occurs after the native commit reservation but during `Manager::receive` or before the accepted receipt is saved, status remains uncertain. The encrypted request is retained and neither replayed nor released automatically. This prevents guessed duplicate work, but complete automatic recovery and a review UI remain unfinished. Real process termination at every admission write has not been qualified.
- The controlled 13-case HTTPS recognition acceptance remains incomplete. Six successful ZIP ownership races and form tests cannot establish all MIME/name/error-response recognition cases.
- The legacy protocol fallback retains its previous admission-before-cancel behavior. This candidate must be released as a compatible browser/native pair and verified in the user's Edge session.
- Integration with the separate COM candidate, release numbering, packaging and deployment remain undone. The broader video, driver, GUI, proxy and distribution gaps in the research report remain open.

## Preservation and cleanup

The 179 top-level native baseline files, all 74 installed browser receipt entries and six network entries were rechecked. No unexpected deployment changes were found. Personal state retained the same SHA-256 as before this work and contains 25 download records. No installed source/executable, IDM setting, driver setting, certificate or personal download was modified.

All eight fixture runs removed their native-host registration. Final read-only cleanup checked 16 browser/key combinations, found no remaining keys or owned test processes, and confirmed the previously removed localhost certificate absent. The build/test sources and logs remain available for reproduction. [Summary, source and binary hashes](evidence-capture-transaction-20260929/summary.json) · [cleanup receipt](evidence-capture-transaction-20260929/cleanup.json)
