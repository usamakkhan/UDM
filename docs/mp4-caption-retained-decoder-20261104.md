# Shared retained-file MP4 WebVTT decoder — 1.12.2

MP4 WebVTT assembly now accepts a bounded list of initialization/media files, original resource offsets and an explicit presentation clock. The recorded adaptive-transfer adapter uses this shared entry point, preserving the existing fragment continuation, cue limits, timestamp mapping, edit-list rates and output ordering.

This removes the decoder's dependency on a recorded-download layout at its API boundary, allowing retained live recording files to use the same decoding logic in the next integration step. Negative resource offsets and unsupported clock/offset magnitudes are rejected before decoding. It does not yet enable live caption selection, recording or muxing. Those end-to-end requirements remain in live-dash-caption-integration-audit-20261104.md.

All five native production targets built successfully at 1.12.2; build-start source hashes remained unchanged. Runtime tests, installer creation, installation and publication are deferred. Browser source remains 0.63.84 and installed UDM remains 1.6.0. This is backend integration groundwork, not proof of live caption support or full IDM parity.
