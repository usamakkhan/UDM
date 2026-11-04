# Live-DASH captions across silent Periods

Development version 1.19.0; browser source remains 0.63.85.

Live recording assembly now joins media-only Period files and decodes caption fragments into one bounded recording-relative cue timeline. Captions are encoded once into the final MP4 rather than requiring a subtitle stream in every intermediate Period. This removes the requirement for a spoken caption in every Period while retaining the selected caption track, complete retained-resource coverage, hash/lease checks, initialization and configuration checks, capture clipping, inline timestamp rebasing, Period adjacency and media codec compatibility.

FFmpeg's [concat demuxer documentation](https://www.ffmpeg.org/ffmpeg-formats.html#concat) requires matching streams across inputs. The revised path keeps audio/video stream signatures consistent independently of sparse subtitle events. Final validation checks the expected subtitle count, mov_text codec, unchanged audio/video stream signatures and coarse recording duration.

An entirely silent capture produces the selected audio/video output without an invented caption or empty WebVTT demuxer input. Its zero captured-cue result is persisted and shown in progress information and Properties > More > Last attempt and result. Nonzero counts report decoded cue records, not encoded subtitle packet counts. The standalone retained-Period assembly API handles empty decoded cues in the same way.

Caption assembly binding revision advances to 3. Session receipts bind the complete source caption selection/resources and retain the cue count. Media-only Period output receipts can still reuse their verified bytes. Recording indexes and downloaded fragment journals remain unchanged. Receipts permit one concat list plus an optional generated WebVTT input, with bounded names/counts; output size and SHA-256 remain verified on reuse.

The change does not allow missing selected caption tracks, incomplete capture coverage, incompatible caption configurations, Period gaps, unsupported formats, encryption or reverse/dwell timed-text payloads.

Build status: passed for all five production targets. Both full and final incremental builds completed; final embedded versions and SHA-256 hashes are recorded. Feature source hashes matched the final-build snapshot. Runtime and playback tests, installer creation, installation and publication remain deferred. Installed UDM remains 1.6.0. See [build receipt](validation/live-dash-silent-captions-build-20261104.json).
