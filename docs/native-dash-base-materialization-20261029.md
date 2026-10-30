# Native SegmentBase resolution and materialization

All five native production targets and the UDM 1.1.0 / extension 0.63.65 installer built successfully. The packaged binaries come from a final forced rebuild against the unchanged completed header, superseding the initial pass. Logs, source hashes and the installer checksum are recorded in [the build record](validation/native-dash-base-materialization-build-20261029.json). The candidate is unsigned, staged, uninstalled and unpublished.

The addressing resolver now reads inherited SegmentBase attributes, Initialization and RepresentationIndex children. It retains inherited BaseURL alternatives and availability metadata, rejects mixed addressing modes and duplicate locators, and validates embedded index/initialization ranges. External index files can use a bounded range or the entire file when it fits the index reader limit. Conflicting index locators and ambiguous embedded/external alternatives are rejected.

The materializer discovers a strong media version and, for an external index, a separate strong index version. It uses the version-bound index-tree reader to obtain ordered media references, then produces the list descriptors used by the existing native planner. Each reference carries the media version and exact byte range. The materialized addressing also retains the index version and reference ID for later capture identity integration.

A common integer timescale is calculated from the manifest clock and all index clocks. Presentation-time offset, timestamps and durations are rescaled exactly; overflow or a common scale beyond supported precision is rejected. This avoids rounding fractional timestamps into whole seconds when SegmentBase omits its timescale. Initialization metadata and availability offsets remain attached to the resulting addressing.

Unmaterialized indexes cannot enter the window planner. Live selected-track admission remains gated because materialized clock and index identities still need integration into capture bootstrap, refresh and recovery. Browser SegmentBase admission also remains unfinished. This batch provides manifest resolution and the native expansion API; it does not claim end-to-end SegmentBase recording.

Runtime manifest, network, capture, restart and playback tests remain deferred at user request. Extension 0.63.65 is unchanged. No installation or publication is performed.
