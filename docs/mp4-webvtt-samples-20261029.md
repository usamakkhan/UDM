# Bounded MP4 WebVTT sample decoding

The new Mp4WebVtt.hpp decoder reads one already-demuxed WVTT sample extent. It returns typed cue payloads, identifiers and settings, recognizes explicit empty samples, and preserves simultaneous cues. It validates normal/extended box sizes, sample boundaries, UTF-8, duplicate fields and conflicting empty/active cues. Limits are 2 MiB per sample, 4,096 boxes, 1,024 cues and 64 KiB per text field; cancellation is checked during traversal.

Timing is deliberately not guessed from text. The container layer must supply validated sample boundaries and clocks from initialization and fragment metadata. Unknown extension boxes are skipped only within checked extents. No byte buffer is treated as a NUL-terminated string.

This is the payload-decoding prerequisite for segmented MP4 captions. Initialization parsing, track binding, fragment sample mapping, composition/decode timestamps, browser admission and assembly integration remain unfinished. The browser does not advertise MP4 captions yet. Adaptive.cpp includes the decoder so the production compiler checks it; it is not yet invoked in a download.

The box roles were checked against Shaka Player's primary implementation: https://raw.githubusercontent.com/shaka-project/shaka-player/main/lib/text/mp4_vtt_parser.js . UDM's decoder was written separately with explicit budgets and typed results. No reference source was copied. FFmpeg's n8.0 MOV subtitle tag table lacks a wvtt entry, so general WebVTT codec presence was not assumed to prove MP4 support: https://raw.githubusercontent.com/FFmpeg/FFmpeg/n8.0/libavformat/isom.c . This is not a qualification of the bundled binary.

Extension 0.63.69 is unchanged. All five production targets and the unsigned installer built successfully. See [build record](validation/mp4-webvtt-samples-build-20261029.json) for artifact and source hashes. Parser/runtime tests remain deferred at user request; compilation alone does not establish correctness. No installation or publication is performed.
