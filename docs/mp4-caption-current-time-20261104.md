# MP4 WebVTT cue-current-time metadata — 1.15.3

The MP4 sample decoder now retains optional ctim as bounded currentTimeMs alongside cue text, identifier and settings. Duplicate, invalid UTF-8, multiline, malformed or out-of-range cue timestamps are rejected using the existing WebVTT timestamp parser. The value survives fragment decoding in Mp4WebVttCue.

This supplies the missing input for future mapping of inline timestamps in split cues. It does not yet remap those timestamps through sample clocks, playback-rate edits or capture clipping; that remains an explicit source gap.

Research references: the author presentation for the W3C 2025 in-band text breakout describes ctim and delayed cue parts (https://ntrrgc.github.io/w3c-breakouts-2025-mse-text-tracks/); the ISO BMFF conformance inventory identifies CueTimeBox (https://download.tsi.telecom-paristech.fr/gpac/MPEG/ISOBMFF-Conformance/conformance_report.html). No third-party implementation code was copied.

All five native production targets built successfully at 1.15.3; build-start source hashes remained unchanged. Runtime tests, installers, installation and publication remain deferred. Installed UDM remains 1.6.0; browser source remains 0.63.85.
