# Native live SegmentList capture

All five native production targets and the UDM 1.1.0 / extension 0.63.64 installer built successfully. Logs, source hashes and the installer checksum are recorded in [the build record](validation/native-dash-list-capture-build-20261029.json). The candidate is unsigned, staged, uninstalled and unpublished.

Native live track admission now accepts the inherited SegmentList planner added previously. Capture obtains a version descriptor before requesting listed media or initialization resources and passes it through the existing protected recording checkpoints and conditional transfer path. Template/list addressing changes during refresh are rejected instead of silently changing an active recording's storage contract.

Discovery first requests HEAD metadata. When HEAD is unsupported or omits usable metadata, it requests headers for bytes 0-0. The fallback validates a returned 206 range, or reads the whole resource length from a 200 response without consuming the media body. A positive bounded total and strong ETag are required. HTML, encoded or multipart responses, weak/missing validators and malformed lengths are rejected. The descriptor uses the exact final URL, without a fragment.

Each discovery has a 15-second cancellation deadline, at most 12 requests, six redirect hops per attempt, and at most 32 source alternatives. Every request resolves credentials from its own explicit origin scope; implicit browser/site/intranet credentials are disabled. HTTPS downgrades are rejected. Subsequent media requests enforce the discovered version rather than trusting the metadata probe alone.

Capture reuses saved descriptors for an unchanged ordered source-URL set and shares discoveries within a window. New source sets are probed; conflicting versions for the same final URL are rejected by the recording-wide consistency check. Existing segment and initialization identities remain immutable during refresh. Metadata is checkpointed before media workers begin. Sources without strong validators remain unsupported for this live-list path.

This enables the native capture path, including recovery and Period transitions, but the browser live catalog still excludes SegmentList. Browser catalog and capability integration remain next work. SegmentBase still needs native index parsing. Runtime recording, restart, redirect and playback tests are deferred at user request; successful compilation does not establish runtime acceptance.

The extension remains 0.63.64. No installation or publication is performed.
