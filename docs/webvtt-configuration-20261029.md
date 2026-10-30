# WebVTT style and region preservation

Plaintext caption assembly now retains STYLE and REGION blocks in its intermediate WebVTT. The parser collects bounded definitions before cues, and recorded HLS, standalone DASH, multi-Period DASH and live HLS pass them through to the merged mux input. Identical repeated definitions are emitted once; differing nonempty definition sets across segments fail explicitly. Multi-Period assembly requires matching configuration across Periods. Segments without definitions can reuse the track configuration.

Mapped WebVTT initialization can contain valid configuration blocks. It still cannot contain cue timing. The combined header/body receives an explicit blank block separator, so the first cue is not absorbed into a final style or region definition. Configuration is limited to 128 blocks and 64 KiB per segment; existing combined subtitle limits remain.

This closes the earlier source-level loss of plaintext configuration. It does not implement CSS evaluation, fetch referenced resources, reconcile changing style definitions, or establish the final styling fidelity of FFmpeg mov_text conversion. Runtime and playback verification remain deferred. The format reference is the [W3C WebVTT syntax](https://www.w3.org/TR/webvtt1/#syntax).

Extension 0.63.70 is unchanged. All five production targets and the unsigned installer built successfully. See the [build record](validation/webvtt-configuration-build-20261029.json) for source and artifact hashes. No parser/runtime tests, installation or publication are performed.
