# Browser filename and ownership follow-up — 29 September 2026

**The filename timing defect is corrected in staged code. A separate duplicate-download race remains unresolved and blocks deployment.** Installed UDM remains native 0.74.0 / browser 0.47.1. These changes form a later source revision of browser 0.49.0; the previously compiled 0.49.0 installer does not contain them. No new installer was built or installed in this pass.

## Actual browser finding and change

Edge emits `downloads.onCreated` before assigning the filename. The old handler immediately falls back to the URL's suffix. In the real HTTPS baseline, Edge eventually chose `archive.bin`, but UDM had already submitted `html40.zip`. A browser-selected `.txt` name could likewise be ignored when the URL ended in `.pdf`.

The candidate now reads the browser's resolved filename before classifying the download. It handles the matching filename event, removal, completion, lookup failure and a bounded timeout, and removes its temporary listeners. Already-named downloads need no extra API call. Policy checks apply again to the resolved item. This keeps generic-name MIME recognition while preserving explicit filename choices.

Actual Edge tests of the filename change showed:

- The public PDF and ZIP were captured as `report.bin` and `archive.bin`, with matching output hashes and native ownership before browser cancellation in that run.
- A later `keep.txt` filename, a local file-type exclusion and a site exclusion completed in Edge with no native job.
- The tiny Unicode PDF exposed the separate ownership failure described below.

The files were the [W3C dummy PDF](https://www.w3.org/WAI/ER/tests/xhtml/testfiles/resources/pdf/dummy.pdf) and [HTML archive](https://www.w3.org/TR/html4/html40.zip). Same-origin download attributes controlled the browser filename. TLS verification remained enabled and these public-file runs changed no certificate trust. These are functional capture checks, not speed benchmarks or a live IDM comparison.

## Ownership failure is still open

The browser can finish while the native handoff is being acknowledged. In that case both Edge and UDM can retain a completed file. The final source adds a pre-submission check for a completed, already-received, resumed, removed or replaced browser download. When detected before submission, it leaves ownership in the browser instead of sending another request.

That check passes its targeted regressions, but **it does not close the race**. A final public ZIP run still recorded an accepted native submission followed by both Edge and UDM completing. The failure is retained and is not counted as successful recognition acceptance. A fresh browser snapshot alone is insufficient to make the ownership transition atomic.

The next implementation needs an explicit handoff transaction: stage native ownership without starting replay, confirm the browser's final disposition, then commit native transfer or release the staged job. Crash/restart, lost acknowledgement, browser completion and user cancellation need acceptance at every transition. The exact implementation should be derived from the existing receipt protocol, rather than assuming the proposed sequence is already implemented.

A separate public [arXiv PDF](https://arxiv.org/pdf/1706.03762) attempt redirected, replaced the suggested filename and reported “The remote file changed during transfer” in the native job. It also invalidated the fixture's original-URL assumptions. It is retained as an unsuccessful external-source case; the native error's cause is not established.

## Verification and limits

The final source passes **1,153 regression checks**: 1,101 checks across 41 component/native-protocol scripts, plus 26 actual Edge/native and 26 actual Firefox/native assertions. Each form run covers 13 scenarios, including exact binary request replay and output hashes, with unsupported submissions left in the browser. These are fresh runs of the modified code. Earlier/repeated runs and checks inside failed public-file attempts are excluded from that total.

The new tests cover delayed filename assignment, explicit suffix precedence, MIME capture, unrelated events, stale reads, timeouts and listener cleanup, plus completed/received/resumed/removed/replaced downloads at final submission. The guarded Firefox zero-byte pause remains supported.

**The complete controlled 13-case recognition suite remains unqualified.** The new localhost certificate insertion returned Windows error 1223 (“operation was canceled by the user”) before tests began. A retry question remains pending, and the canceled operation was not retried. The exact temporary certificate is absent; no certificate-verification bypass was used. An alternative publicly offered development certificate failed its validity check and was not installed.

All eight browser fixture runs removed their temporary native-host keys. The final read-only cleanup found all 16 checked browser/key combinations absent, no processes with the fixture paths in their command line, and the exact test certificate absent.

Installed receipt, browser and network files remain unchanged except previously authorized research documentation. All 25 personal download records, queues and other preferences are preserved. The floating-basket coordinates changed during the session; replacing only those two coordinate values in a read-only copy reproduces the preceding state hash. Their cause was not established, and the current position was preserved.

[Verification summary](evidence-browser-filename-20260929/summary.json) · [source diff](evidence-browser-filename-20260929/source-review.diff) · [unit results](evidence-browser-filename-20260929/unit-results.json) · [Edge forms](evidence-browser-filename-20260929/candidate-edge-forms-final/results.json) · [Firefox forms](evidence-browser-filename-20260929/candidate-firefox-forms-final/results.json) · [final duplicate failure](evidence-browser-filename-20260929/candidate-edge-w3-final/trace.json) · [cleanup](evidence-browser-filename-20260929/cleanup.json)

The candidate remains staged. Fixing the ownership transaction and qualifying recognition precede deployment; the broader [IDM parity gaps](idm-research-current-status-2026-09-29.md) remain open.
