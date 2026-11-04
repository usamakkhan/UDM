# DASH media-kind preflight — 1.12.3

Native live-DASH representation selection now compares the manifest-derived track kind against the requested kind after resolving stable IDs and inherited metadata. A selected audio/video ID can no longer silently identify a different kind until final assembly. Unsupported or conflicting MIME/contentType declarations are rejected before media recording.

The shared classifier recognizes application/mp4 with wvtt and contentType text as subtitles. This classification is needed for the forthcoming live-caption selection contract; current live-plan validation still permits only video/audio. Subtitle capture and final muxing remain unfinished and no browser caption option is enabled. Existing identity comparison continues to protect later refreshes and Period transitions.

All five native production targets built successfully at 1.12.3; build-start source hashes remained unchanged. Runtime tests, installers, installation and publication remain deferred. Installed UDM remains 1.6.0; extension source remains 0.63.84. Full IDM parity remains unproven.
