# Playlist capture across SPA revisits — 2026-10-08

Two stale-capture paths reproduced: an old fetch response was accepted after route A -> B -> A, and network response headers could restore a playlist whose request began before navigation. Browser 0.62.12 fixes those reproduced cases in both bundles.

The early observer now tracks navigation generations through successful pushState/replaceState, popstate and hashchange. Pending fetch clones and XHR results must match their starting generation. Original responses and history exceptions remain unchanged. Same-URL state updates retain valid capture. If a site makes its history methods read-only, ordinary capture continues instead of failing script initialization.

The webRequest playlist fallback retains request-start navigation stamps through redirects and rejects known stale arrivals. Its memory-only tracking is bounded to 2,048 requests, with 180-second age pruning at request start and completion/error cleanup. Unknown starts still use existing response-time behavior. Worker restart, eviction and unhookable fast route changes remain qualification gaps; this is not universal stale/ad detection.

Fresh validation: 24 real Edge observer checks, 27 mocked navigation checks for each browser bundle, 12 Dailymotion recovery checks and 17 HLS catalog checks. A full real Edge/native recheck passed 15 download checks, including MP4/M4A, parallel ranges and mapped live captions. The first full attempt passed 10 checks then failed because the preview fetch returned ERR_INVALID_HTTP_RESPONSE. It did not reach live submission; its cause remains unestablished. Both temporary registrations were removed. Failure evidence is retained.

[Validation receipt](validation/observer-navigation-20261008.json). No native source/binary changes. Source updated and tested; not packaged or activated in personal browsers. Latest installer remains browser 0.62.11. Multi-player attribution, embedded advertisements and public-site acceptance remain open; full IDM parity is unestablished.

Follow-up qualification: a full real isolated Firefox run passed 14 checks with browser 0.62.12 and the same app/host hashes used by Edge. It verifies selected-audio MP4/M4A, exact parallel indexed requests, live mapped subtitles and rejection of ignored ranges. Temporary native registration and Firefox/geckodriver processes were removed. All recorded source hashes still match. This is wider than the earlier six-check mapped-caption-only Firefox run.

Personal-desktop recheck through the documented computer-use API returned only Codex as a targetable window. Launching Edge failed with `GetCursorPos: Access is denied (0x80070005)`; a fresh window enumeration still exposed no Edge or IDM. No personal browser activation, public-site test or IDM side-by-side measurement occurred. The prior preview HTTP failure remains unexplained despite the passing recheck and Firefox run.
