# Experimental browser streaming transport

UDM 0.6 / extension 0.5.0 includes an original SABR/UMP implementation. It does not invoke yt-dlp. **It has synthetic test coverage, but no successful current live YouTube download has been verified.** The most recent HD selection still reported that a usable streaming session had not been captured.

The document-start observer captures playback request context and parses bounded response metadata to associate formats with the current video. The extension rejects stale/ad identities, supplies only observed qualities, and offers either usable direct URLs or a matched captured streaming session. It does not invent 2K/4K/8K streams.

The native implementation in `src/SabrWire.cs` and `src/Sabr.cs` decodes UMP framing/protobuf fields, preserves opaque request fields, selects the requested audio/video format identities and begins at time zero. It handles bounded context updates, verifies video identity and segment continuity, assembles separate initialization/media tracks, and sends those local files through the existing FFmpeg merge and FFprobe dimension checks. Browser request context is protected with Windows DPAPI at rest.

Controlled tests cover integer boundaries, unknown-field preservation, truncated framing, format selection, exact track assembly, other-video rejection, gaps, short segments, encrypted content rejection and expired capture. A large-frame quota regression verifies that partial progress reaches the configured quota and cancellation publishes no output. These fixtures validate our implementation against the supplied transcripts, not YouTube's current live service behavior or IDM's internals.

Current limits include recorded MP4 video/audio only, captures no older than five minutes when creating a job, bounded frame/context sizes, and no DRM, attestation bypass or automatic external resolver. The native session can still be rejected or require fresh browser playback. Raw captured URLs/request bodies should not be included in shared diagnostics. A WFP transport-byte monitor does not replace this browser/session work.

See [live observations](browser-capture.md), [validation](validation.md) and the local sanitized extension diagnostics page. A successful real capture, verified output tracks/quality, and a new speed measurement remain required before claiming working current YouTube support.
