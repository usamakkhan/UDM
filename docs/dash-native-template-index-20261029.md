# Native DASH representation-index templates

The native manifest resolver now accepts representation-wide SegmentTemplate@index or inherited SegmentTemplate/RepresentationIndex. It expands RepresentationID and Bandwidth where permitted, retains a separate media-file BaseURL, and resolves initialization from a template attribute or Initialization element. Number/Time template variables are unavailable in this path and therefore rejected. Explicit timeline, duration, media-template and numbering declarations are rejected as conflicting sources.

The resulting addressing object uses the existing indexed-media implementation. That implementation discovers bounded index/media versions, parses and resolves SIDX trees, materializes segment clocks/ranges, validates availability, and binds its source to protected recording checkpoints. Template normalization supplies the same resolved URLs, timescale, presentation offset and index/initialization ranges to source identity. Refreshes that change the indexed source still fail instead of silently reusing old media. In-file index/initialization locators require ranges; separate index URLs can use the existing bounded whole-index read.

Duplicate locators/ranges, malformed resources, external template xlinks and missing initialization/media BaseURL are rejected. Existing exact-clock, resource-version, byte/request/depth and segment budgets remain active. Growing indexed resources and incomplete low-latency chunks are not implemented by this change.

The hello response advertises live-dash-template-index. Extension 0.63.80 is unchanged: its recorded catalog already resolves these indexes, but live browser catalog selection has not yet been connected. This is native source integration, not an end-to-end live-browser completion claim. The previous [recorded template-index work](dash-recorded-template-index-20261029.md) records the reference-source analysis and separate browser implementation.

All five native production targets compiled and installer packaging passed. See [build evidence](validation/dash-native-template-index-build-20261029.json); the production bundle manifest records all 147 payload hashes. Runtime manifest, browser, recording, recovery and completed-file tests remain deferred at the user's request. Nothing is installed or published; full IDM parity is unproven.

The subsequent [0.63.81 live-browser integration](dash-live-template-index-20261029.md) supersedes the catalog-connection gap above. Runtime acceptance remains deferred.
