# Native DASH timeline support

The native MPD module now reads SegmentTimeline entries and expands the segments overlapping a requested media-time window. It handles explicit and implicit start times, fixed repeats and negative repeats bounded by the next explicit start or the caller's window end. Segment numbers advance across skipped history, so selecting a recent window does not renumber its segments.

Expansion uses checked integer ticks, not floating-point timestamps. It skips old repetitions arithmetically and limits the returned window to 10000 segments. Input entries are limited to 12000. Invalid integers, nonpositive durations, unsupported unqualified entry attributes, backward/overlapping entries, arithmetic overflow and unbounded internal repeats fail explicitly. Cancellation is checked during parsing and expansion. Gaps are retained rather than filled with invented segments.

The window is half-open: a segment ending exactly at its beginning is excluded; a segment beginning exactly at its end is excluded. A final negative repeat is expanded only as far as that window requires. This does not establish server availability or a presentation's end. A caller must apply presentation offsets, wall-clock availability and refresh policy separately.

## Integration status

These functions are compiled with the native manifest reader. They are not yet connected to a live recorder. Dynamic browser MPDs remain rejected. URL template resolution, availability filtering, refresh scheduling, durable recording and live browser admission still need implementation. Compatible recorded multi-Period handling remains on its existing path.

Runtime and regression tests are deferred at the user's request. Only production compilation and installer packaging are performed for this candidate; those do not establish download correctness or complete IDM parity. The browser remains version 0.63.61. The candidate is uninstalled and unpublished.

Build evidence: `validation/native-dash-timeline-build-20261028.json`.
