# Browser live DASH SegmentList selection

All five native production targets and the UDM 1.1.0 / extension 0.63.65 installer built successfully. Logs, source hashes and the installer checksum are recorded in [the build record](validation/browser-live-dash-list-build-20261029.json). The candidate is unsigned, staged, uninstalled and unpublished.

Extension 0.63.65 adds supported live SegmentList representations to the DASH quality menu, including separate audio renditions and audio-only output. Inherited list attributes, timelines, initialization and replacement URL lists are resolved for catalog selection. Native code remains responsible for timing, range validation, version discovery, capture, recovery and final assembly.

The catalog continues to skip unsupported representations individually. SegmentBase, external lists, per-segment indexes, mixed template/list inheritance, ambiguous addressing, missing live timeline anchors and unsupported list boundary adjustments are not advertised as supported. Unsupported-only audio still produces an explicit error rather than a silent video-only selection.

For list resources the browser retains one concrete URL per exact origin, bounded to 64 credential lookup origins per track. This keeps long SegmentURL lists out of the handoff while preserving the existing origin-scoped request-context and optional cookie policy. The desktop fetches the manifest itself and derives all media locations from it; browser metadata does not override the native resource planner.

Each selected list track carries a marker. Handoff checks the final selected tracks, after audio selection, for the new live-dash-segment-list desktop capability. The updated native host advertises that capability. Older desktop builds receive an update-required message instead of an unsupported handoff. Existing template-only handoffs retain their current capability requirements.

Chromium and Firefox media and handoff scripts are mirrored. JavaScript syntax checks passed; runtime browser, capture, interruption, redirect, playback and IDM-comparison testing remain deferred at user request. No installation, extension reload or publication is performed.
