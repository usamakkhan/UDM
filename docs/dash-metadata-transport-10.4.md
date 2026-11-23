# Recording-owned DASH metadata transport — 10.4

The 10.3 phase trace narrowed a failed recording's delay to HTTP construction/header handling: three stages took 208–590 ms against a 200 ms manifest interval, while parsing and accounting were below 1 ms. A private transport-reuse prototype passed the complete recording suite, but its unrestricted thread cache was unsuitable for product ownership and isolation.

## Product implementation

A live DASH recording now creates one stack-owned metadata scope on its worker. Manifest and HTTP clock requests can borrow transport sessions from that scope. The cache key includes the canonical origin and the complete serialized request preferences. Changes to protocol, port, proxy, browser session, login preferences or any other preference therefore cannot silently reuse a mismatched session.

The cache retains at most 16 top-level sessions, with serialized keys limited to 1 MiB each. A full cache or an oversized key uses ordinary per-request transport ownership. Neither condition introduces a new request rejection or evicts a pool still owned by a request. Existing proxy-routing behavior inside each session remains unchanged.

The active scope is local to its worker. Nested scopes have independent caches and restore their predecessor when they exit. Normal return and exception unwinding release the recording's sessions. An individual request still retains shared ownership until its asynchronous callbacks have closed. No global warm cache or recording cookie jar survives the recording owner. The existing blocking request facade, parent cancellation, response accounting and shutdown ordering remain in place.

Clock requests still remove captured browser state and site logins and disable automatic Windows login. The resulting sanitized preferences select a different transport even when the clock endpoint shares the manifest origin. Cross-origin redirect sanitization still runs before transport selection. Headers and authorization remain request-local. Response cookies can rotate within the appropriate recording/origin, while a later recording begins from its own captured snapshot.

The 10.3 bounded freshness recovery and all stale-response checks remain enabled. Reuse addresses repeated request setup; it does not make an expired response fresh, renew a direct-time body anchor, remove metadata charging or weaken retained-Period boundaries.

## Qualification

The 20 transport lifetime/isolation checks pass, alongside 20 AAC structural and 42 strict retained-boundary checks. Real HTTP requests verify rotated-cookie reuse, a clean next recording, changed/removed authorization, an anonymous clock request and a sanitized redirect to another origin. A separate held-response cancellation control exits while the server still withholds headers, propagates cancellation and releases the transport; the measured cancellation-to-unwind time is 31 ms. That is a background request measurement, not GUI paint or IDM timing.

The production-code seven-case continuous recording suite passes on its first attempt for this candidate: complete audio/video, video-only and audio-only recordings; rejection of gaps, short audio and changed presentation identity; and Save while refresh headers remain held. Decoded source PCM, audio packet hashes and video frame timing match. Each positive result publishes once, with final size/hash matching verified assembly, and the fixture checks one GET per expected captured media resource. All actors terminate and the origin closes. The intentionally abandoned held response logs a connection-reset diagnostic; this is retained with the result.

All eight bounded-freshness controls also pass, including persistent stale responses, slow clock responses, changed identity, HTTP error and parent cancellation. These exercise the ordinary unscoped fallback as well as the unchanged three-attempt policy; the real recording and wire controls exercise the new owner.

The first local build attempt failed because the new test omitted its header-helper include. The corrected test is retained alongside that compile log. A later frozen-control link failed when it mixed new Adaptive code, which references the scope, with an old DashManifest object. The final controls use their complete hash-verified previous core sets. Runtime core objects were retained through these build-driver corrections; no product behavior was changed to satisfy the controls.

All five native programs build with version 10.4, and both source browser manifests advance to 0.73.68. Browser scripts are unchanged. Verification checks 745 pinned build inputs, 42 core objects, 14 build outputs and the separately compiled cancellation actor. Adaptive, DashManifest and Bridge are rebuilt; the other 39 core objects are reused with matching hashes. The complete build entrypoint, MIDL generation and installer packaging are outside this bounded native build.

The four positive continuous cases publish final files of 351,173 bytes (audio/video), 324,776 bytes (video), 26,450 bytes (audio) and 97,822 bytes (Save). An independent final audit rehashes each published file and matches it to its accepted result and verified assembly size. These are owned local fixtures, not public-site or IDM speed measurements. The nine protected installed-file hashes remain unchanged; no installer, installation, release or explicit temporary-file deletion was performed.

See [verification](validation/dash-metadata-transport-10.4/verification.json), [HTTP isolation](validation/dash-metadata-transport-10.4/wire-qualification.json), [held-response cancellation](validation/dash-metadata-transport-10.4/cancellation-qualification.json) and [complete recording qualification](validation/dash-metadata-transport-10.4/continuous-qualification.json). The tracked records contain text metadata and exact source snapshots; media and executable artifacts remain local.

## Remaining work

1. Audit freshness across expensive window construction and later capture/admission boundaries, using the actual DASH availability contract before introducing extra requests or rejection.
2. Implement exact non-aligned video packet/decode ownership and preroll, then interior AAC continuity.
3. Investigate the separately observed repeated capture-resource GETs in direct-recording fixtures. The continuous recording suite's one-GET checks cover its own path and do not close that broader issue.
4. Qualify installed browser/desktop, authenticated media and cross-player flows. A local cookie/credential isolation test is not broad website compatibility or full IDM parity. No new IDM speed comparison or GUI timing is established here.
