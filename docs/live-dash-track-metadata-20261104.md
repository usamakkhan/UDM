# Live-DASH selected-track metadata — 1.15.1

Live-DASH admission returns before the ordinary recorded-adaptive validator. It now validates selected audio/subtitle names and language tags in its own contract: fields must be strings, names are bounded to 512 bytes without control characters, and language tags follow the existing recorded-track syntax and 63-byte limit. Nonempty descriptions must correspond to an actually selected audio or subtitle track.

This brings the new live-caption handoff into line with recorded selection metadata validation. It does not establish browser recording, silent-caption behavior, playback timing or continuation acceptance.

All five native production targets built successfully at 1.15.1; build-start source hashes remained unchanged. Runtime tests, installer creation, installation and publication remain deferred. Installed UDM remains 1.6.0; extension source remains 0.63.85.
