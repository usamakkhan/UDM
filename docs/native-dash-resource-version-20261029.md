# DASH resource-version binding

All five native production targets and the UDM 1.1.0 / extension 0.63.64 installer built successfully on October 29. Build logs, source hashes and the installer checksum are recorded in [the build record](validation/native-dash-resource-version-build-20261029.json). The candidate is unsigned, staged, uninstalled and unpublished.

The native resource store now accepts an explicit resource-version identity containing the exact HTTP resource URL, total file length and strong ETag. The identity is included in the protected receipt key along with any byte range. Weak validators, malformed tags, non-integer or oversized totals, and ranges outside the bound file are rejected before cache access or network requests.

Versioned requests use identity encoding and bypass stale cache entries. If-Match is sent only to the exact bound URL. Redirects and alternative locations retain origin-scoped credentials, but a successful response must resolve to that exact URL and return the same ETag. A validator from one URL is not assumed to identify an alternative CDN resource. HTTP 206 responses must match the requested slice and the bound total file length; whole-file responses must match the bound length, including when Content-Length is absent.

Existing whole-resource and range-only callers retain their current identities. Offline receipt inspection validates the version descriptor and byte count as well as the existing file hash. This is a storage prerequisite, not completed live SegmentList support: acquiring versions, persisting them through recording descriptors and keeping all slices of a shared source bound to one version remain unfinished. Native selected-track admission and browser live-list admission remain gated. SegmentBase still needs native index parsing.

Runtime network, restart, browser and playback tests are deferred at the user's request. The extension remains 0.63.64; this change is native only. No installation or publication is performed.
