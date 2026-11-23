# Single-pass full DASH resources — 10.6

**Follow-up finding:** an additional shared-URL restart probe found that 10.6 can reject an unfinished alias after another entry has pinned its version. Accepted bytes remain intact, but the retry cannot complete. The original qualification below did not cover that state. [The 10.7 correction and frozen/current controls](dash-pending-alias-10.7.md) address segment and initialization aliases. The candidate remains uninstalled and not release-qualified.

The capture worker can now establish a fresh independent SegmentList resource's version from the same full GET whose body it stores. This removes HEAD and fallback range probes when that response supplies a valid strong ETag and bounded Content-Length. Initialization files use the same path. Shared ranged objects and indexed media keep their existing preflight and conditional range requests.

## Ownership and persistence

Before issuing a body request, capture saves an explicit pending-version intent containing the track identity, segment coordinates and original URL set. A new inner recording schema, 2, makes this recovery policy explicit. The protected outer envelope remains schema 1; existing inner schema 1 recordings retain their original receipt rules. Older readers must reject schema 2 without rewriting it.

Only active capture workers open responses, with the existing maximum of three workers. There is no global response cache or preflight loop holding every media body open. Redirects select credentials for their own origin, suppress implicit browser/site credentials and reject HTTPS downgrade. The response must be a plain full resource: status 200, no Content-Range, no HTML or multipart body, no unexpected content encoding, no duplicate length/tag metadata or length/transfer-encoding ambiguity. Length must be positive and at most the existing 256 MiB resource bound.

Under the checkpoint mutex, the worker compares the new descriptor with all known versions of the same actual URL and ordered source list. It constructs the versioned binding and atomically saves that binding before body accounting or writing. Conflicting parallel admissions fail without accepting the conflicting data. Once admitted, the resource cannot silently acquire a different version through an alternative URL.

The validated, still-open response passes directly to the existing storage loop. Length checks, cancellation, accounting, SHA-256, file flushing, completion recovery and durable receipts keep their existing order. A subsequent retry uses the saved version and If-Match. A valid existing receipt avoids a network refetch.

If the first GET lacks enough version metadata, it is closed and the established HEAD/range discovery route remains available. In particular, HEAD may supply the total for a chunked GET. This compatibility case can incur an extra initial GET; the optimization does not promise a single request for every server.

## Recovery contract

An unbound pending intent cannot claim committed data, a resource receipt, a filename or a byte range. Recovery ignores unversioned orphan receipts for such an intent. A versioned completion receipt written before the recording index update remains recoverable using the pinned descriptor. The helper that proves a retained Period ending also refuses unresolved or malformed pending-version records.

These changes add no global transport lifetime or public recording-structure layout. Bridge is rebuilt for the version and DashManifest for capture/storage/recovery. Adaptive retains its qualified 10.5 object because its included interfaces and implementation inputs are unchanged. All produced executable resource versions are independently checked.

## Qualification and delivery

All five native programs build as 10.6, and both source browser manifests are 0.73.70. Browser scripts are unchanged. Verification pins 750 source inputs, 42 core objects, 17 main executables and two supplemental recovery controls. Only DashManifest and Bridge change from the retained 10.5 core set. Nine protected installed-file hashes remain unchanged. The private driver uses retained compiler/core inputs; the full packaging entrypoint, MIDL regeneration and installer creation are not exercised.

The final runtime checks pass:

- 107 focused assertions: 22 pending-version, 23 transport/role, 20 AAC and 42 retained-boundary checks.
- 17 HTTP workflow cases, including one GET per fresh full resource with HEAD supported or unsupported, initialization plus segment, receipt reuse without refetch, shared ranged-object preflight, invalid responses, chunked fallback, cancellation, changed-version rejection and original-version recovery.
- Three separate recovery controls: ignore an unversioned orphan while retaining its bytes; recover a pinned receipt committed before its index update without an additional recovery fetch; reject schema 2 through frozen 10.5 core code without rewriting it. Legacy schema 1 still adopts its own unversioned receipt.
- Fourteen transport-isolation requests, held-header cancellation, eight freshness controls and seven production continuous-recording cases. The four positive recordings publish exactly once and their physical sizes/hashes match the accepted records; gaps, short audio and changed identity publish nothing.
- Five direct-media and four caption-session cases compare decoded PCM/video/captions and final publication size/hash. Their first cases use frozen capture controls as designed; the remaining cases exercise the current capture path. These checks do not establish universal playback compatibility or exact multi-Period interior boundaries.

Failed attempts remain visible. The first raw fixture lacked stable manifest IDs and was corrected before its successful runs. A supplemental recovery actor initially omitted its digest header, then compiled after adding that include. The first continuous run could not locate shared media tools; a junction to the installed tools fixed the test setup without copying FFmpeg. An unscoped standalone clock control expired before reaching its clock endpoint during the parallel build (HTTP/header stages exceeded its 200 ms manifest validity); the unchanged recipe passed after compilation. That separate rerun does not prove the cause of the earlier timing failure or eliminate it under every host load. No validity window was relaxed.

See [technical verification and storage gate](validation/dash-single-pass-10.6/verification.json), [single-pass and recovery results](validation/dash-single-pass-10.6/single-pass-qualification.json), and [complete-media results](validation/dash-single-pass-10.6/continuous-qualification.json). Source snapshots and sanitized metadata are tracked; binaries and owned media remain private.

**Delivery remains unqualified:** the final audit observed 1,031,274,496 free bytes on D:, below the unchanged 1 GiB reserve (1,073,741,824). Technical checks passed, but the combined verification explicitly reports `passed: false`, `storageReservePassed: false` and `releaseQualified: false`. Further candidate builds are held for space. No installer, installation, release or explicit artifact deletion is included. Browser/desktop and actual IDM timing comparisons remain outstanding.

## Required next work

1. Extend fault injection to real process termination before headers, after admission, during body writing and at each final receipt transition. The deterministic cancellation and receipt-before-index controls cover specific durable states, not every power-loss outcome.
2. Add adversarial parallel admissions, alternative/redirect chains and credential rotation specifically through the new body-admission route. Preserve the separate existing transport isolation qualification.
3. Measure server bytes written, client bytes consumed/accounted, request counts and latency together under controlled network conditions. Request elimination alone is not a timing comparison with IDM.
4. Complete non-aligned video preroll/ownership and interior AAC continuity across multiple Periods with decoded-source oracles. The single-Period AAC-tail and caption guarantees must remain intact.
5. At the delivery milestone, qualify the installed browser-to-File-Info workflow, exact pre-Start sizes and actual IDM timing. Keep new source versions distinct from the protected installed programs until then.
