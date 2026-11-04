# Browser live-DASH MP4 WebVTT selection — 1.15.0 / 0.63.85

Chromium and Firefox live-DASH catalogs now discover application/mp4 wvtt caption representations with stable IDs and initialization. Video offers expose up to 32 language/label subtitle choices through the existing subtitle menu. Audio-only offers retain no subtitle choices.

The selected caption identity is appended before live capability and addressing checks, so its SegmentList/SegmentBase/template requirements and credential origins follow the same handoff checks as media. The browser requires live-dash-mp4-webvtt, preventing handoff to older desktops. DASH captions bypass the HLS subtitle-playlist path. Native 1.14.0 admission/recovery and the retained decoder/muxing path provide the corresponding source support.

The source workflow is now connected, but end-to-end browser capture, continuation, silent subtitle Periods, timing and playback remain unverified. Tests, installer creation, installation and publication remain deferred. Installed UDM remains 1.6.0 / extension 0.63.82; these source updates are not activated there.

All four JavaScript syntax checks passed. All five native production targets built successfully at 1.15.0; build-start source hashes remained unchanged.
