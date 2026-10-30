# Recorded DASH Period-window expansion

Recorded browser capture previously expanded every SegmentTimeline repeat into a download URL. It did not filter whole segments before presentationTimeOffset or after the Period end, and rejected any repeat longer than 10,000 segments even if only a small overlap was relevant. The native window planner already selects overlaps.

The Chromium and Firefox parsers now calculate the first and last overlapping repeat ordinals before generating URLs. Segments touching a boundary with no overlap are omitted; partially overlapping segments are retained. Original $Number$ and $Time$ values are preserved. Long skipped runs advance the cursor arithmetically, while the existing 10,000-entry plan cap applies to retained segments and initialization. Integer overflow and unbounded-repeat checks remain in place. A template producing no media is no longer offered as an initialization-only track.

For a source-derived example, a timeline with t=0, d=2 and r=99999, a presentationTimeOffset of 199980, timescale 1, Period duration 20 and startNumber 1 should retain numbers 99991 through 100000 at times 199980 through 199998. This expectation was not executed as a parser test.

This changes recorded explicit SegmentTemplate expansion, including recorded multi-Period tracks. Duration-only templates, SegmentList, SegmentBase and live-native selection are unchanged. Sample-level clipping of retained boundary segments remains an assembly responsibility; this change does not establish final output duration or decoder correctness.

Reference: [DASH-IF timing model](https://dashif.org/Guidelines-TimingModel/), sections 9.2 and 9.3, distinguishes segment references, Period overlap and presentation of samples. This is format evidence, not an observed IDM comparison.

Both extensions are 0.63.72. Syntax checks passed; runtime/parser/browser tests are deferred at user request. Native sources and binaries are unchanged from the successful duration-offset build. Installer packaging passed. See [build evidence](validation/dash-period-window-build-20261029.json) for hashes and limits; the production bundle manifest records all 147 payload files. Nothing is installed or published.
