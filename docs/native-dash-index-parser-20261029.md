# Native DASH SIDX decoding

All five native production targets and the UDM 1.1.0 / extension 0.63.65 installer built successfully. Logs, source hashes and the installer checksum are recorded in [the build record](validation/native-dash-index-parser-build-20261029.json). The candidate is unsigned, staged, uninstalled and unpublished.

The native DASH module now decodes ISO BMFF segment-index boxes into typed references containing byte position, length, media time, duration, index/media kind and the original SAP field. It preserves the reference ID, timescale, earliest presentation time and first offset. Both version 0 and version 1 SIDX fields and ordinary or extended box lengths are supported.

The decoder accepts at most 512 KiB of index data, 64 SIDX roots and 10,000 total references. Child indexes are bounded to 512 KiB; individual media references to 256 MiB. Resource sizes, byte arithmetic and clocks remain within the exact integer range used by the browser/native handoff. Truncated headers/tables, nonzero reserved fields, unsupported versions/flags, zero timescales/durations, invalid ranges and overlapping index/media locations are rejected. Cancellation is checked while walking boxes and references.

Embedded indexes resolve offsets relative to the end of the SIDX box in the resource. External indexes maintain separate index and media address spaces, matching the existing recorded-stream parser's model. Child locators cannot point back into their parent or partially overlap another decoded index box. SAP fields are retained without inventing a random-access interpretation.

This is native index decoding, not completed SegmentBase capture. Bounded tree fetching, parent/child timing and identity checks, external-index traversal, manifest addressing integration and browser admission remain unfinished. The existing SegmentBase rejection remains in place. Runtime parser, network and playback tests are deferred at user request; source review and production compilation do not establish runtime acceptance.

Extension 0.63.65 is unchanged. No installation or publication is performed.
