# WebVTT cue metadata and line endings

Plaintext HLS/DASH caption parsing now retains cue identifiers and positioning settings in the intermediate WebVTT supplied to muxing. Previously both were discarded even though the merged writer could retain them for MP4-derived captions. Deduplication already includes these fields, so cues with different settings remain distinct.

The plaintext parser, header validator and MP4 assembly now share CRLF/CR/LF normalization. MP4 cue text with ordinary CRLF is accepted instead of rejected just for containing CR. Serialization still rejects timing delimiters and blank-block boundaries that would change the cue structure; empty MP4 cue payloads do not produce empty output blocks. Existing timestamp mapping and Period offsets are unchanged.

The [W3C format syntax](https://www.w3.org/TR/webvtt1/#syntax) defines the accepted line endings and optional cue identifiers/settings. Retaining these fields in intermediate WebVTT does not prove that FFmpeg mov_text preserves every styling feature in final MP4 playback. STYLE/REGION blocks in the existing plaintext path still need separate preservation work.

Extension 0.63.70 is unchanged. All five production targets and the unsigned installer built successfully. See the [build record](validation/webvtt-cue-fidelity-build-20261029.json) for source and artifact hashes. Runtime, parser and playback tests remain deferred at user request; no installation or publication is performed.
