# Native live-DASH caption admission — 1.14.0

Live-DASH plans now admit video, optional audio and one final subtitle track, while keeping audio-only recording restricted to one audio track. AudioExpected is checked against the actual audio count, so video+captions no longer looks like video+audio. Duplicate IDs, Period consistency and credential-origin limits remain enforced.

Manifest preflight requires the subtitle identity to resolve to application/mp4 with wvtt and an initialization segment. Refresh-time kind checks use the shared classifier. Protected recording sessions now retain up to three track identities; existing exact plan/format/clock comparisons remain in force for continuation and Period transitions. The desktop advertises live-dash-mp4-webvtt for the upcoming browser capability gate.

The decoder and muxing path are already present. Browser live subtitle discovery/selection and installed workflow acceptance remain unfinished. Silent subtitle Periods, timing, continuation, cancellation and Period joining still need runtime validation. Tests, installer creation, installation and publication are deferred. Installed UDM remains 1.6.0; extension source remains 0.63.84.

All five native production targets built successfully at 1.14.0; build-start source hashes remained unchanged.
