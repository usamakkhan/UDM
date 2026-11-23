# Recording-owned DASH resource transport — 10.9

Fresh HTTP sessions were still being created for independent initialization and segment bodies after the 10.6 single-pass change. In an owned HTTP/1.1 control, nine sequential resources opened nine TCP connections through Windows HTTP. A separate mapped-loopback curl control opened five connections for five resources. Native 10.9 retains bounded resource transports for a recording; both source browser manifests advance to 0.73.73.

## Ownership and identity

Live DASH creates one resource-transport scope alongside its separate metadata scope. Capture takes an explicit owner context and binds it on each joined worker. An unrelated thread cannot implicitly borrow the recording's sessions. A standalone capture creates a local owner for its window; a public storage call without an owner keeps ordinary per-request ownership. Nested scopes and worker bindings restore their previous contexts.

The owner retains at most 16 top-level sessions, keyed by canonical origin, complete transport preferences and the origin's original credential/identity headers. Per-resource ranges, validators and cache-control fields added by the body writer do not split that stable identity. A complete serialized key above 1 MiB or a full cache uses ordinary request ownership. Known entries are not evicted while requests use them. A mutex serializes lookup/construction across workers.

Body sessions enable bounded resolved-route retention: each session retains at most 16 protocol/PAC routes, with a 1 MiB route-key limit. A new route beyond either limit remains request-owned; an existing route is still reusable. The containing session mutex protects this policy. Ordinary sessions retain their previous unbounded route policy. These limits describe top-level sessions and each route map separately, not a claim that the complete nested proxy hierarchy contains only 16 objects. Live PAC/proxy acceptance remains a separate gate.

Authorization, cookie, request identity, preference, scheme or port changes select separate pools. Body requests keep their existing sanitization of browser/site/intranet credentials and use the current origin's explicit headers after redirects. Metadata/clock/discovery sessions are separate owners. Existing WinHTTP cookie disabling and curl's explicit cookie handling remain in effect; response cookies in the connection fixtures are not added to subsequent resource requests.

The same response still establishes a fresh resource version, saves its binding before body writes, and flows through the existing length/hash/flush/receipt writer. Known versions still use conditional requests. Cache scope ends normally or on an exception, while an active request's shared reference retains its transport until callbacks close. Capture's worker limit remains three.

## Qualification

The Windows HTTP candidate uses one TCP connection for nine resources in each of three runs; the frozen 10.8 control uses nine. Curl uses one connection for five resources in each of two runs; the control uses five. Requests, body accounting, stored resource bytes/SHA-256, explicit cookies and authorization match. There are no artificial response delays. Raw native process durations are retained as observations, not accepted as an IDM latency comparison or a general throughput result; host load and filesystem work vary. These connection tests use HTTP loopback, not TLS.

New ownership and route-retention suites cover explicit worker binding, nested restoration, normal/exceptional release, held request ownership, full identities, concurrent access, capacity fallback, route-factory failure and key bounds. A three-worker cancellation test holds every media response before its headers, signals parent cancellation, and requires the native process to exit before the server releases them. It checks no body consumption, no accepted receipts, joined-worker unwind and final shared-session release within three seconds. Its initial supplemental link failed during COFF conversion; relinking the unchanged compiled object succeeded. The failed link is retained; its exact cause is not established.

Final verification, counts and links will be added after the audit finishes. This candidate is uninstalled, with no installer or release. Installed browser/UI behavior and actual IDM timing remain unverified.

## Ordered remaining work

1. Extend the new owner to live proxy/PAC, HTTPS/TLS, anonymous cookie-injection and credential-rotation wire controls. Verify route-cap fallback without weakening routing or authentication boundaries.
2. Inject real process stops at final flush, completion-receipt writing and receipt/index replacement. Current process-stop coverage does not simulate hardware power loss.
3. Trace request setup under the user's actual proxy/network settings, final flush and complete background work. Measure browser-to-dialog and completion timing against installed IDM after the delivery gate passes.
4. Complete non-aligned video preroll and interior AAC continuity across multiple Periods using decoded-source oracles.
5. Correct the stale default HTTP User-Agent (`UDM/0.35.0`) found in Transfer.cpp, preserving explicit/captured browser User-Agent overrides and qualifying both HTTP backends.
6. Resolve D: workspace capacity before another installed/lab delivery cycle, then compare the current UI and extension with IDM and reconcile the remaining visual/workflow gaps.
