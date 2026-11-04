# Retained DASH caption assembly — 1.13.0

The native recording backend now provides assembleDashRecordingCaptions for a reconciled retained MP4 WebVTT track and a Period-local capture interval. It verifies initialization and fragment file names, sizes and hashes, holds read leases excluding writes/deletion, preserves original range offsets, orders committed segments, and invokes the shared MP4 WebVTT decoder. The resulting WebVTT is clipped to the requested interval and rebased to zero.

The helper rejects redirected storage, pending initialization, duplicate/uncommitted segment records, unsupported clocks and excessive part counts/sizes. It returns subtitle text without publishing or muxing a video. Recording-wide coverage and session continuity must be established by the caller.

This is a backend assembly step. Live caption selection, capture admission, joined Period subtitle mapping and final stream validation are still unfinished; browser choices remain disabled. Embedded cue-text timestamp remapping remains a separate known limitation.

All five native production targets built successfully at 1.13.0; build-start source hashes remained unchanged. Runtime tests, installers, installation and publication remain deferred. Installed UDM remains 1.6.0; extension source remains 0.63.84. Full IDM parity is unproven.
