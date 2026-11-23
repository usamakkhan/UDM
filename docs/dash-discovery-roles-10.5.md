# Purpose-scoped DASH discovery — 10.5

Resource-version discovery now borrows the recording-owned metadata transport introduced in 10.4. A supported HEAD request and its fallback range-header request can reuse a session without changing version checks, source credentials, cancellation, request deadlines or response validation.

The cache key now includes an explicit role: manifest, clock or resource discovery. Origins and complete request preferences remain separate dimensions. Requests with identical sanitized preferences still receive different transports when their roles differ. Each role can reuse its own pool. The existing 16-session and 1 MiB serialized-key bounds, ordinary-request fallback, nested scopes and recording-owned teardown remain unchanged. The two-argument helper retains its manifest-role behavior for existing callers.

This makes the role boundary explicit instead of depending on differences between sanitized settings. HTTP headers remain request-local, and clock requests still clear browser cookies and site logins. Real requests test a credential-bearing discovery sequence followed by an anonymous clock request. Protocol-specific authentication such as NTLM/Negotiate remains a separate integration gate.

## Why the direct fixture has extra GETs

The direct-recording fixture supplies a SegmentList. Before capture, UDM discovers a strong ETag and total length, fixes the version descriptors, validates shared-resource consistency and durably saves the pending index. Only then do capture workers fetch media and commit receipts.

Discovery first tries HEAD. The fixture does not implement HEAD, so its 501 response triggers a `Range: bytes=0-0` GET. Discovery reads its headers and closes the response; the later body request uses the pinned version. When a server honors the range, the first GET represents a one-byte metadata response. When it ignores the range and returns 200, it may send more data before the client closes. Request counts alone do not establish two complete client-side downloads.

The 10.5 change retains those probes. It reduces repeated session setup within the recording owner and separates transport roles; it does **not** claim to remove the direct fixture's extra GET. Removing that request correctly needs the response/checkpoint design below.

## Single-pass full-resource follow-up

1. Restrict the first implementation to independent full resources in SegmentList, with no byte range. Preserve the current preflight/If-Match path for shared ranged objects and indexed media.
2. Persist an explicit pending-version intent for each selected initialization/segment before requesting its body. Its original track identity, URL set, clock, segment number and range coordinates must remain bound. A pending intent cannot authorize an unversioned completion receipt.
3. Have the active capture worker open one full GET. Validate the final origin, scoped credentials, media encoding/type, positive bounded length and strong ETag from that response. Do not infer its body identity from a different endpoint or an earlier unauthenticated probe.
4. Before the first body write or resource receipt, reconcile the descriptor with every already known version of the same source, update the binding and atomically checkpoint it. Serialize this admission across workers. Conflicting versions must preserve previously accepted data and fail before accepting the new body.
5. Hand that same response into the existing validated storage loop. Retain its cancellation owner and normal length/hash checks, quota/rate accounting, final flush and receipt order. Hold only the currently active workers' responses; do not open thousands of responses during global preflight or retain an unrestricted response cache.
6. Define recovery explicitly. Only an uncommitted, resource-free pending intent may acquire its first version. Known or committed versions cannot be silently upgraded. Recovery must not accept an old unversioned receipt as completion for a new pending-version entry. Preserve orphan bodies safely without claiming them as accepted.
7. Exercise crashes/cancellation before header admission, after its checkpoint, mid-body and during final receipt commit. Test weak/missing/changed validators, wrong lengths/ranges, HTML/encoded bodies, shared URLs, redirects, changed credentials and legacy catalogs. Compare decoded media, accepted hashes and final publication sizes with independent source oracles.
8. Measure actual server bytes written and client bytes consumed/accounted, alongside request counts. Qualify both HEAD-supported and HEAD-unsupported origins, range-honoring and range-ignoring servers. A lower request count cannot substitute for correct version ownership or complete files.

## Qualification

The final binaries pass 23 transport/role checks, 20 AAC checks and 42 retained-boundary checks: 85 distinct assertions. Fourteen real HTTP requests verify HEAD fallback, repeated strong-version discovery and cookie/authorization/clock/redirect isolation. Both role reuse and separation with identical preferences are checked. A held-response control cancels before headers are released, unwinds in 63 ms in this run and verifies transport release.

All eight bounded-freshness controls pass. The production-code seven-case continuous suite also passes, preserving complete source PCM/audio packets/video timing, rejecting gaps, short audio and changed identities, and completing Save while refresh headers remain held. The four positive cases publish once; a final audit checks their physical file sizes and SHA-256 values against accepted assembly records. All actors terminate and origins close. The intentionally abandoned held-refresh response logs a connection-reset diagnostic, retained with the results.

All five native programs build as 10.5; both source browser manifests advance to 0.73.69. Browser scripts are unchanged. Verification covers 747 pinned inputs, 42 core objects and 15 produced executables. The dependency audit rebuilt Adaptive against the changed private-scope header, then rebuilt the final executable set using that object, DashManifest and Bridge. The other 39 core objects match their retained hashes. Initial cached-core executables are superseded and are not the qualification targets. The private-scope object layout and existing constructor/destructor/helper interfaces remain compatible; the fresh compilation avoids relying on that observation alone.

See [verification](validation/dash-discovery-roles-10.5/verification.json), [HTTP requests](validation/dash-discovery-roles-10.5/wire-requests.json), [role checks](validation/dash-discovery-roles-10.5/DashMetadataTransportTests.json) and [complete recording qualification](validation/dash-discovery-roles-10.5/continuous-qualification.json). Source snapshots, hashes and metadata are tracked; binaries and owned media remain local. The full build entrypoint, tool discovery, MIDL regeneration and installer packaging are not exercised. Nine protected installed-file hashes remain unchanged. No installer, installation, release or explicit temporary-file deletion is included.

After this work, exact non-aligned video ownership/preroll, interior AAC continuity, installed browser/desktop acceptance and cross-player/IDM comparisons remain open. The role/discovery checks do not establish general website compatibility or timing parity.
