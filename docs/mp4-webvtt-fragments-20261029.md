# MP4 WebVTT fragment timing and sample mapping

The native caption decoder now reads selected-track fragment headers, decode clocks and sample runs. It uses explicit base offsets or supported movie-fragment-relative addressing, continues samples across runs, supports multiple movie fragments per supplied segment, and handles unsigned version-0 or signed version-1 composition offsets. Decode time advances by sample duration independently of presentation offsets. Results retain integer track ticks and the next decode clock; Period/PTO conversion remains an assembly responsibility.

Before decoding text, every selected sample must fit wholly inside one media-data payload and the supplied byte range. Byte-range inputs carry their absolute resource offset. Aliased sample ranges, invalid sizes/durations, time overflow, changing sample descriptions, backward decode intervals and encrypted auxiliary metadata are rejected. Other tracks can occupy separate media-data extents. Bounds include 256 MiB input, 1,024 fragments, 10,000 samples, 2 MiB per sample, 100,000 output cues and 16 MiB cue text. Cancellation is checked during traversal and decoding.

Initialization also verifies that the selected sample description uses one self-contained data reference. The parser does not fetch external referenced data. Implicit offsets inherited from a preceding different track, edit lists, and encrypted captions remain unsupported. A missing initial decode timestamp requires an explicit previous decode clock; none is invented.

These readers are compiled into the production source path but remain unconnected to downloads. Browser admission, transfer-plan format validation, Period clock conversion, cue configuration/settings serialization and assembly are still pending. Extension 0.63.69 is unchanged. The existing standalone WebVTT path remains in use.

Offset/default and timestamp field roles were checked against [FFmpeg MOV parsing](https://raw.githubusercontent.com/FFmpeg/FFmpeg/n8.0/libavformat/mov.c) and the [Shaka MP4 box parser](https://raw.githubusercontent.com/shaka-project/shaka-player/main/lib/util/mp4_box_parsers.js). UDM code is independently written; no reference source was copied.

All five production targets and the unsigned installer built successfully. See the [build record](validation/mp4-webvtt-fragments-build-20261029.json) for source and artifact hashes. Runtime and parser tests are deferred at user request. No installation or publication is performed.
