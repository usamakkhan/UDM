# Recorded DASH SegmentList Period windows

Extension 0.63.82 validates recorded SegmentList timing before expanding its URLs. Explicit timelines must have one media resource per expanded entry; repeat boundaries, monotonic clocks, positive durations, safe integers, list capacity and declared final numbering are checked. An intermediate negative repeat requires an explicit divisible next timestamp; the last negative repeat consumes the remaining list entries, matching native list resolution.

For finite Period assembly, lists with SegmentTimeline or duration now retain only whole media segments whose nominal intervals overlap [presentationTimeOffset, presentationTimeOffset + Period duration). Segments crossing either boundary remain included for the existing native assembly trim. Segments wholly before or after the Period are not requested by this selection. Empty overlapping selections do not create initialization-only tracks.

Duration-only lists retain their existing native timeline origin of zero; they are not treated as duration-based SegmentTemplates, whose timing rules differ. Recorded lists with no declared timeline or duration retain full-list behavior, and legacy flat selections are not window-filtered. The list resource count remains bounded even when most entries fall outside the selected Period. Existing selected-resource range validation, handoff budgets and native clock handling remain in place.

This follows the existing native SegmentList resolver and the Period-overlap handling already used for recorded templates. It is source integration, not a measured bandwidth improvement or proof of exact IDM behavior. Multi-Period assembly, MP4 WebVTT and media request recovery still need runtime qualification for affected lists.

Both JavaScript syntax checks passed. All five native production targets compiled and installer packaging passed. See [build evidence](validation/dash-recorded-list-window-build-20261029.json); the production bundle manifest records all 147 payload hashes. Runtime parser, browser, transfer and completed-file comparisons remain deferred at the user's request. No installation or publication is performed.
