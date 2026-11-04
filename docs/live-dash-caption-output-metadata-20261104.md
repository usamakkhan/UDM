# Live-DASH caption output metadata — 1.15.2

Final session muxing now writes the selected caption label as handler_name/title and the selected language as subtitle stream metadata. Caption-bearing single-Period recordings also pass through this final metadata step instead of returning the raw Period result. Audio/video-only single-Period recordings retain their existing fast path.

Caption session assembly bindings include a metadata version, so a previously reused result without the selected caption metadata cannot bypass the correction. Existing stream signatures and duration verification remain in place; stream-copy joining retains the encoded subtitle track.

All five native production targets built successfully at 1.15.2; build-start source hashes remained unchanged. Runtime/playback checks, installer creation, installation and publication remain deferred. Installed UDM remains 1.6.0; extension source remains 0.63.85. Metadata/player display compatibility and end-to-end live recording remain unverified.
