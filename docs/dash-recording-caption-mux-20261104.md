# DASH recording caption muxing — 1.13.1

Recording Period assembly now recognizes a retained MP4 WebVTT track, runs the verified retained-caption decoder, and supplies its clipped/rebased WebVTT to ffmpeg as an explicit subtitle input. The output encodes that track as mov_text and retains its language. One subtitle track is allowed with video; audio-only caption output is rejected.

Period output validation now checks the selected subtitle count. Compatible session joining retains the subtitle stream with stream copy and checks stream signatures across Periods and the final output. Caption files use the existing temporary-input/receipt naming and cleanup contract.

This completes a native muxing step, not end-to-end live capture. Browser selection, live-plan admission, recovery/session track limits and capture behavior still need integration before exposing captions. Runtime tests, installers, installation and publication remain deferred. Installed UDM remains 1.6.0; browser source remains 0.63.84.

All five native production targets built successfully at 1.13.1; build-start source hashes remained unchanged.
