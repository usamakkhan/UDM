# Native DASH availability filtering

The native resource planner now has a live availability entry point that accepts a synchronized wall-clock timestamp. It maps each segment's media-time end to presentation time using the selected Period start, timescale and presentationTimeOffset. It excludes segments outside the selected Period, expired segments and future resource alternatives. The presentation's availability end closes the result entirely.

The implementation follows the segment-end availability model in the [DASH-IF timing guidelines, section 13.3](https://dashif.org/Guidelines-TimingModel/#availability). The time-shift depth supplies the oldest boundary, and applicable availability offsets extend the newest boundary. Suggested presentation delay is not used as a download delay: playback policy and resource availability are separate concerns. A signalled resource can still fail on the server, so transfer retry behavior remains necessary.

BaseURL alternatives now retain their own accumulated availabilityTimeOffset and completeness metadata. Template attributes inherit down the selected hierarchy. Finite signed decimal/exponent offsets and INF are parsed explicitly; malformed or unsupported values fail. Equal URL strings with different timing remain distinct until the availability decision. The output includes a URL when at least one complete alternative for that URL is ready. Incomplete alternatives are excluded; an entirely chunk-only selection raises an explicit unsupported error.

Large shared media epochs are subtracted as integers before conversion to seconds. Remaining tick values and timescale must fit the exact integer range used by the platform's floating-point representation. This is a bounded timing implementation, not a claim of arbitrary-precision boundary arithmetic. The caller still supplies a bounded media-time window; broad windows can hit the existing planning limits before filtering.

## Remaining live work

The new entry point calls the existing resource-window planner and filters its URLs. It does not synchronize clocks, fetch resources, schedule manifest refreshes, record media or enable browser live admission. Those integrations are still required. Low-latency chunk streaming, complete DASH conformance and end-to-end IDM parity are not established. The recorded browser path and extension 0.63.61 are unchanged.

Runtime and regression tests are deferred at the user's request. Production compilation and installer packaging are recorded in `validation/native-dash-availability-build-20261028.json`. The candidate remains unsigned, uninstalled and unpublished.
