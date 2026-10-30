# MP4 WebVTT initialization metadata

The caption decoder now has a bounded initialization reader. It binds the WVTT sample description to its own track ID and media timescale, retains its UTF-8 configuration, and reads matching fragment defaults. It rejects ambiguous tracks/descriptions, duplicate IDs, invalid box extents, zero clocks and unsupported edit-list mapping. Initialization data is limited to 16 MiB, 64 tracks, 64 descriptions per track and 16,384 visited boxes.

This extends the sample-payload decoder foundation; it is not yet connected to a download. Fragment sample offsets, timing, data references and assembly integration still require implementation before browser admission can be enabled. Configuration is retained, not rendered. Browser version 0.63.69 is unchanged.

Format roles were checked against the [Shaka MP4 WebVTT parser](https://raw.githubusercontent.com/shaka-project/shaka-player/main/lib/text/mp4_vtt_parser.js); UDM implementation is independently written. No reference code was copied.

All five production targets and the unsigned installer built successfully. See the [build record](validation/mp4-webvtt-initialization-build-20261029.json) for source and artifact hashes. Runtime and parser tests remain deferred at user request. No installation or publication is performed.
