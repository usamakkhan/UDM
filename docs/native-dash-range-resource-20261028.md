# Native DASH byte-range resources

Build complete: all five native production targets and the UDM 1.1.0 / extension 0.63.64 installer compiled successfully, with compiler warnings recorded in the build log. Evidence: validation/native-dash-range-resource-build-20261028.json. The candidate is unsigned, staged, uninstalled and unpublished.

The native DASH resource store now accepts a range-bound resource identity created by `dashRangeBinding`. Its protected binding includes integer start and length; whole-resource callers retain their existing identity and receipt keys. Ranges must be positive, fit signed 64-bit coordinates without overflow, and stay within the existing 256 MiB per-resource bound. Malformed or oversized range fields are rejected before filesystem recovery or network requests.

Every alternative URL and redirect rebuilds the Range header from that binding after removing captured Range and conditional-cache headers. Ranged requests explicitly ask for identity encoding. Existing exact-origin credential lookup, anonymous defaults and HTTPS downgrade rejection still apply.

A ranged response must be HTTP 206 with a single numeric Content-Range whose start/end equal the requested interval and whose total exceeds its final byte. Multipart ranges, encoded responses, inconsistent Content-Length, excess bytes and truncated bodies are rejected. A server returning the whole object with HTTP 200 is not accepted as the requested slice. The expected length is enforced even when Content-Length is absent. Receipt recovery additionally requires its saved byte length to equal the bound range length, then performs the existing digest and file verification.

This is a backend prerequisite, not completed live SegmentList/SegmentBase support. Current template planners continue to request whole resources. Follow-up work must carry ranges through planned media and initialization descriptors, protected recording indexes, reconciliation and sliding-list identity checks; native list/index parsing and browser admission remain separate. An index-based file also needs coherent resource-version binding across index, initialization and media slices before it can safely be enabled. Existing recorded-DASH resource validation is a reference for that contract.

Runtime HTTP, cancellation and recovery tests are deferred at user request. No installation, extension reload or publication is performed. Extension 0.63.64 is unchanged.
