# Recorded DASH MP4 WebVTT browser handoff

Chromium and Firefox extension 0.63.70 now recognize application/mp4 caption representations with the wvtt codec. They require explicit initialization metadata and expose compatible caption options through the existing recorded DASH selection UI. Template, list and indexed addressing use their existing expansion paths. Standalone plaintext WebVTT keeps its neutral Period clock; MP4 captions retain the addressing clock and presentation offset, including single-Period plans.

Selected MP4 caption tracks carry the native mp4WebVtt contract. The handoff requires the desktop dash-mp4-webvtt capability after caption selection, covering both single- and multi-Period plans. Older desktops receive an update-required message rather than an unsupported plan. The desktop now advertises that capability for its implemented caption assembly path.

This completes the browser-to-native wiring for this caption format. It does not establish runtime correctness, complete subtitle rendering fidelity, or full IDM parity. Unsupported native edit lists, encryption and cross-track implicit offsets still fail explicitly. No live DASH caption support is added.

Both browser copies passed JavaScript syntax checking. All five native production targets and the unsigned installer built successfully. See the [build record](validation/dash-mp4-webvtt-browser-build-20261029.json) for artifact and source hashes. Parser, browser, download and playback tests remain deferred at user request. No installation, reload or publication is performed.
