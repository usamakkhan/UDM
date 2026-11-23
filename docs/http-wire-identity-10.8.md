# Canonical HTTP identity encoding — 10.8

Native version advances to 10.8 and both source browser manifests to 0.73.72. Internal media and Site Grabber requests can already specify identity encoding, while the shared HTTP transport also emitted its default identity field. Actual 10.7 wire captures showed duplicate values through both WinHTTP and curl. Differently cased map keys could add further copies or a conflicting gzip value.

## Product change

The shared HTTP constructor removes all case-insensitive Accept-Encoding entries from its effective internal header map, rejects CR/LF injection before removing them, and emits one canonical `Accept-Encoding: identity` field. The same normalized map feeds request admission, so callers whose effective wire policy is identical no longer differ merely because an internal layer repeated this reserved header.

Identity encoding remains the byte-preserving transport policy. User-supplied request headers already reject this reserved field; no user setting is removed. Body, method, Range/If-Range, credentials, redirect and response-validation code retain their existing behavior. WinHTTP and curl consume the same normalized header block. Encoded server responses still follow the existing explicit validation/decoder rules.

Only Transfer and Bridge require fresh core compilation. The retained 10.7 DashManifest object includes the pending-alias correction; its public interfaces and included implementation sources are unchanged. Product binaries, actors and resources are rebuilt against the selected complete core set.

## Wire and recovery checks

A frozen 10.7 core control and the current candidate use the same wire actor. An IPv4 loopback URL takes the Windows HTTP route; a mapped IPv6 loopback URL selects curl according to the production routing helper. Both are served by the same owned HTTP origin. Actual requests test default headers, explicit identity, lower-case identity, mixed-case duplicates, conflicting internal encoding values, POST, HEAD and byte ranges. The actor checks complete response bytes, status and route selection. The independent server checks the encoding fields, exact POST bytes, POST content defaults, methods and range validators. Injection must fail before a request reaches the server.

The frozen control reproduces duplication; the candidate emits exactly one identity field in all 16 requests. Both retain 48 actor assertions. A first actor compile attempted to inspect a private transport member; that failed compilation is retained. The corrected actor uses the public routing helper and real wire observations without changing the production interface.

Additional full-resource admission checks exercise three simultaneous workers sharing one URL. Equal versions succeed; conflicting strong ETags or total lengths fail without combining versions. After recovery, all remaining entries use the saved descriptor and accepted files remain intact. Cross-origin redirects and alternative locations check origin-specific authorization/cookies, cancellation, conditional retries, and validators sent only to the pinned resource origin. Arbitrary owned bytes validate storage here; complete-media decoding is a separate suite. The pilot fixture used disallowed request-header inputs and failed at the established validator; the corrected fixture preserves that validator.

All five native programs build as 10.8. Verification pins 757 main source inputs, 42 core objects and 20 main executables, plus one frozen header control, one current alias actor and three HTTP regression executables. Only Transfer and Bridge change from 10.7; 40 core objects retain their qualified hashes. The full build entrypoint, MIDL/tool discovery and installer packaging are not exercised.

Final checks pass: the 107 existing focused assertions; 48 wire-actor assertions and 16 independently inspected HTTP requests; six parallel/origin scenarios; 105 admission, 17 gzip-decoder and 14 read-reservation assertions; and 53 POST-response assertions. Seventeen single-pass cases, three real process-crash cases, both alias-recovery regressions, 14 metadata-isolation requests, held-header cancellation, eight freshness controls, seven production recording cases and nine direct/caption media cases also pass. Four positive recordings publish once with physical size/SHA-256 matching the accepted records; the strict negative cases publish nothing. Complete-media checks keep their frozen capture baselines explicit.

See [verification](validation/http-wire-identity-10.8/verification.json), [frozen wire requests](validation/http-wire-identity-10.8/frozen-wire-requests.json), [current wire requests](validation/http-wire-identity-10.8/identity-requests.json), [parallel/origin qualification](validation/http-wire-identity-10.8/parallel-qualification.json) and [POST recovery results](validation/http-wire-identity-10.8/post-results.json). Source snapshots, failed pilot records and sanitized metadata are tracked. The parallel actor's first link lacked its resource input; it was relinked from the same compiled object after that input was supplied. Failed pilots are not counted as successful qualification runs.

C:'s build-volume reserve passes. D:'s workspace reserve remains below 1 GiB, so combined verification explicitly reports `passed: false`, `technicalChecksPassed: true` and `releaseQualified: false`. Nine protected installed-file hashes remain unchanged. No installer, installation, release, explicit artifact deletion, browser comparison or actual IDM timing claim is included.

## Remaining work

1. Finish hard-stop coverage at flush, receipt writing and receipt/index replacement. Existing process-stop tests are not hardware power-loss tests.
2. Extend origin-isolation coverage to HTTPS downgrade rejection, authenticated proxies, redirect limits, alternate-source credential rotation and changing source lists. The current HTTP/mapped-loopback fixtures do not prove every proxy, TLS or browser route.
3. Collect timing and accounting for request setup, server writes, client reads, final flush and background completion together. Do not infer latency parity from canonical headers or fewer requests.
4. Complete non-aligned video preroll and interior AAC continuity across multiple Periods using decoded-source oracles.
5. Recover sufficient workspace capacity, then qualify installed browser handoff, File Info latency, exact pre-Start size and completion behavior against actual IDM. Keep installed and source-candidate versions distinct.
