# Candidate browser 0.48.0 on native 0.74.0

**Staged, not installed.** The recognition candidate is rebased onto browser 0.47.1, preserving exact-address exclusions and the corrected Firefox generator. Current native-host protocol and cancellation policy regressions pass.

The classifier uses browser filenames, observed Content-Disposition and MIME declarations to recognize supported extensionless/generic downloads. Explicit filename suffixes retain precedence. It declines ambiguous MIME inference, conflicting web/error responses, unsupported request replay and mismatched local/desktop filters. It performs no additional sniffing request, preserving the existing ownership and POST handoff paths.

The independent MIME map is generated from pinned MIT-licensed mime-db 1.54.0 plus the IANA audio/flac registration. Regeneration reproduces both bundles byte-for-byte. Browser identity and permissions are unchanged.

989 checks across the current browser/protocol suite pass. Two new actual Edge HTTP diagnostic runs reach the isolated native 0.74 catalog but cannot reach browser download capture: the fixture serves an attachment, Edge receives HTTP 204 with no file headers, and additional range requests retrieve the fixture. IPv4 and IPv6 show the same behavior. These are unsuccessful acceptance attempts, not passing classifier tests, and the injected response's exact owner is not established by this trace alone. IDM settings and driver state were not changed.

HTTPS baseline/candidate acceptance did not complete. The original helper was stopped at 13:32 UTC after its temporary certificate expired; the exact certificate was verified absent before and after cleanup. No replacement helper was started. Recognition acceptance remains required before deployment. [Cleanup receipt](evidence-candidate-0.75.0/trust-cleanup-1.json).

The byte-signature classifier, general capture-rule engine, broader media/driver parity and personal browser activation remain open. See the existing IDM research gap report for the complete scope.

The candidate installer compiled successfully and its packaged files match the reviewed source. Native binaries, the installed 0.47.1 browser, all 25 download records, queues and preferences were verified unchanged. No recognition code has been installed yet.

[Candidate/package verification](research-recognition-2026-09-29/candidate-verification.json), [regression results](research-recognition-2026-09-29/unit-results.json), [source review](research-recognition-2026-09-29/source-review.diff), [IPv4 response trace](research-recognition-2026-09-29/baseline-edge-diagnostic/browser-events.json), [IPv6 response trace](research-recognition-2026-09-29/baseline-edge-ipv6/browser-events.json).

The outstanding helper is `recognition-rebase-20260929/run-https.ps1`; it is waiting inside Windows certificate approval. The next step is to poll that existing process, inspect both actual browser/native test outputs, and verify `trust-cleanup-1.json` before deployment. Do not start a second helper while the first remains alive. If it fails, preserve the failed evidence and repair or further isolate the fixture before installing the candidate.
