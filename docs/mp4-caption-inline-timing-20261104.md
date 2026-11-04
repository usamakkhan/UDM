# Forward MP4 WebVTT inline timestamp mapping — 1.16.0

MP4 cue-text timestamps now use ctim as the VTT clock associated with the sample start. The decoder maps those times through positive playback-rate edits, presentation offsets and output clipping. Source text timestamps must increase; mapped timestamps outside the emitted cue or collapsed by millisecond rounding are removed while text/other markup is retained, matching the existing plain-WebVTT clipping policy.

Recorded Period assembly now rebases mapped MP4 cue-text timestamps when joining Periods. Retained live-caption assembly also shifts them when clipping/rebasing the capture interval. Missing ctim for timestamped text fails explicitly. Timed payloads under reverse/dwell edits remain unsupported; ordinary payloads retain existing reverse/dwell support. Exact delayed-word rendering in final mov_text remains unverified and the source markup conversion does not establish player animation parity.

All five native production targets built successfully at 1.16.0; build-start source hashes remained unchanged. Runtime tests, installers, installation and publication remain deferred. Installed UDM remains 1.6.0; browser source remains 0.63.85.
