# Media assembly track verification

Development version 1.17.0; browser source remains 0.63.85.

The YouTube direct-track media assembly path now probes its selected saved source tracks and muxed staging file before hashing, exact-size confirmation or publication. It requires exactly one video plus one audio track for video output, or one AAC audio track for audio-only output. It compares codecs, video width/height, audio sample rate/channels, and the selected height when exact quality was requested.

When the source exposes a finite positive per-track duration, the output must expose one within 0.5 seconds. Unknown source duration remains unknown; no duration is invented from a container maximum. This is a coarse mux consistency guard, not decoded playback or proof of packet completeness. Probing uses metadata, without packet-count or frame-decode scans. See the primary [ffprobe documentation](https://www.ffmpeg.org/ffprobe.html) for stream selection/output fields.

The worker changes to Verifying output before these checks. Failure follows the existing staging cleanup path and retains the downloaded child tracks for recovery. The existing pre-confirmation download and exact staged-byte size behavior are retained. Browser code is unchanged. Live HLS/DASH have their own existing assembly paths and are not covered by this change.

Build status: passed for all five production targets. Embedded versions and SHA-256 hashes were recorded; source hashes matched the pre-build snapshot. Runtime tests, installer creation, installation and publication remain deferred. Installed UDM remains 1.6.0. See [build receipt](validation/media-assembly-verification-build-20261104.json).
