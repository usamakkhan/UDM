# Live DASH catalogs with mixed addressing

Build complete: UDM 1.1.0 / extension 0.63.64 installer compiled successfully. Evidence: validation/browser-live-dash-mixed-addressing-build-20261028.json. The candidate is unsigned, staged, uninstalled and unpublished.

Extension 0.63.64 keeps supported SegmentTemplate qualities in a live DASH catalog when another audio/video representation uses SegmentList or SegmentBase. Previously either structure threw out the entire catalog. Unsupported representations are now excluded individually, while stable-ID, manifest, timing and ambiguity validation remains in place.

If audio exists only through unsupported list/index addressing, the catalog reports that limitation rather than presenting a video-only choice as the complete program. Supported audio alternatives remain available when present. A supported audio-only option may still be offered when no supported video representation exists; it is explicitly labeled audio only by the existing catalog path. Unsupported representations do not contribute credential URLs or enter the native handoff plan.

This does not implement native live SegmentList or SegmentBase. The retained IDMVMPrs64.dll analysis recognizes both at AdaptationSet, Representation and Period parser sites (SegmentBase references include 180006420, 180006790 and 180007120; SegmentList references include the same functions). These are static parser observations, not evidence of complete live recording behavior. No proprietary implementation is copied.

UDM's recorded browser parser already understands these addressing structures, but the native live parser rejects them. Its resource transfer currently removes Range headers and requires HTTP 200. Completing live list/index support therefore needs native inherited list resolution, stable segment identities across sliding lists, initialization/media byte-range descriptors, validated 206 Content-Range handling, range-bound recovery receipts and index expansion. Removing browser rejection alone would not supply these capabilities. The next implementation should begin with native resource range identity and transfer validation, then list planning and browser admission.

Chromium and Firefox media scripts pass JavaScript syntax checks and mirror each other. Runtime catalog, playback and browser tests are deferred at user request. The extension is staged with the existing compiled UDM 1.1.0 binaries; no native source changed in this batch and no redundant native rebuild is needed. Installation and publication are not performed.
