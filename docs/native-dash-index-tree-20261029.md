# Native DASH index-tree resolution

All five native production targets and the UDM 1.1.0 / extension 0.63.65 installer built successfully. Logs, source hashes and the installer checksum are recorded in [the build record](validation/native-dash-index-tree-build-20261029.json). The candidate is unsigned, staged, uninstalled and unpublished.

The native resolver expands hierarchical SIDX references into ordered media ranges with their reference IDs, timescales, timestamps, durations and SAP fields. It supports embedded and external indexes, multiple complete roots in one range, and child ranges already contained in fetched bytes. It rejects cycles, repeated boxes, partial overlaps, index/media overlap and unaccounted indexes.

Parent and child reference IDs, starts and duration endpoints must agree. Independent roots and final media references must remain continuous in media time. Clock comparison uses integer quotients and remainder products rather than floating-point tolerances or overflowing timestamp cross products. External indexes additionally require contiguous media offsets as children advance the shared media cursor.

Traversal allows eight nesting levels, 64 uncached range-reader calls, 2 MiB of fetched index bytes, 20,000 total index references and 10,000 output media segments. Individual reads retain the 512 KiB parser limit. Cancellation is checked throughout traversal. Contained reads reuse exact cached bytes; partially overlapping fetches are rejected.

The resource-store adapter requires an explicit strong version descriptor. Every fetched index range uses the same version-bound identity, conditional HTTP checks and protected receipt cache. Each range fetch has a 15-second network deadline; application rate/quota waits are accounted separately. Actual received bytes use the existing metadata accounting callback. Index bytes are read with a bounded binary read and SHA-256 checked against the receipt immediately before parsing, including cache reuse. Existing source-alternative and redirect limits still apply within each logical range fetch.

SegmentBase manifest integration and browser admission remain unfinished. This batch adds tree resolution and its version-bound reader, without enabling unsupported selections. Runtime tree, network, interruption and playback tests remain deferred at user request. Extension 0.63.65 is unchanged. No installation or publication is performed.
