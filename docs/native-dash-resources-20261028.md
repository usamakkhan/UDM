# Native DASH template resource planning

The native manifest module now connects its XML tree, timeline expansion and template substitution into a resource-window planner. Given a selected Representation node and the final manifest URL, it walks MPD, Period, AdaptationSet and Representation ancestry. SegmentTemplate attributes inherit down that chain; a descendant SegmentTimeline replaces the inherited timeline. Timescale, presentationTimeOffset and startNumber preserve explicit zero versus missing values, with a strictly positive timescale.

BaseURL elements resolve at each hierarchy level. Multiple alternatives are retained in source order and duplicate resolved URLs are removed. The cross-product is limited to 32 alternatives. The result contains initialization URLs, segment URLs, sequence numbers, integer media times and durations, plus timescale and presentation offset. URL references use the selected representation ID and bandwidth. Number and Time substitutions use each expanded timeline entry. Initialization templates cannot invent a segment number or timestamp.

When no SegmentTimeline exists, positive template duration supplies a regular timeline starting at media time zero. The caller's half-open window bounds expansion; it must already be expressed in media ticks and account for the period mapping/presentation offset. The planner does not interpret the window as proof of server availability. Alternative URLs preserve order but have no fetching or failover behavior yet.

Resources must resolve to HTTP or HTTPS. URL output is bounded to 4 MiB in aggregate, with at most 10000 selected segments and 32 alternatives per resource. Cancellation is checked during ancestry processing, timeline expansion and URL planning. The existing URL parser rejects embedded credentials. Fetch-time credential scoping and redirect policy remain the transfer layer's responsibility.

Unsupported addressing is rejected explicitly: mixed SegmentList/SegmentBase addressing, duplicate templates/timelines, BaseURL byte ranges, template child Initialization/RepresentationIndex forms, and endNumber/eptDelta/pdDelta boundary adjustments. These remain implementation work, rather than silently ignored semantics. This planner supports the common template path; it is not a claim of full DASH conformance.

## Integration and build status

The planner calls the native timeline and substitution functions and compiles with the production manifest module. Live admission, availability/clock calculation, refresh scheduling, transfer execution and durable recording are still unfinished. Dynamic browser MPDs remain rejected. The recorded-DASH browser path and extension 0.63.61 are unchanged.

Runtime and regression tests are deferred at the user's request. Build/package evidence is in `validation/native-dash-resources-build-20261028.json`. The candidate is unsigned, uninstalled and unpublished. Build success alone does not establish actual download correctness or IDM parity.
