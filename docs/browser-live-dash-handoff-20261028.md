# Browser live DASH handoff

Chromium and Firefox extension 0.63.62 now catalog clear single-Period live DASH presentations with stable Period, AdaptationSet and Representation IDs. The catalog supports MP4 video, selectable MP4 audio and audio-only output using inherited SegmentTemplate addressing. It passes stable IDs and the manifest URL rather than expanding a live timeline into a fixed segment list. Native code remains responsible for timeline parsing, server-clock synchronization, availability, resource retrieval, recording and final assembly.

Each selected track carries bounded credential URLs derived from its inherited BaseURL alternatives and media/initialization URI references. These references identify request origins; they are not substituted segment requests. The extension resolves captured request headers and optional permitted cookies for those exact origins plus the manifest origin. When a declared origin has no captured request, it receives the page-origin referrer and user-agent defaults. Native admission rejects credentials outside these declared origins and limits the combined scope to 20 origins. Redirects and newly discovered hosts still use the native exact-origin request policy.

The extension requires the desktop `live-dash` capability. Native admission now accepts the validated live DASH plan, and the existing adaptive handoff reaches the new native worker. The file-properties link model exposes the live manifest. The old fixed-segment plan path and recorded multi-Period selection remain separate and unchanged.

## Current limits

- Live Period transitions and live subtitles remain unfinished. The browser explicitly reports unsupported multi-Period live presentations.
- A stable explicit Period start, availability start and declared UTC timing source are required by this first handoff.
- SegmentList, SegmentBase, encryption and DRM are not admitted through this path. Native template and low-latency restrictions still apply.
- Native parsing can reject a browser catalog choice whose detailed template semantics are unsupported; catalog presence is not proof of a complete download.
- Live recordings have no final file size until their stop boundary and output are known.

Both edited JavaScript files passed syntax checks, and all five native production targets and the installer compiled successfully. Runtime/browser and regression testing remain deferred at the user's request. Results and hashes are recorded in `validation/browser-live-dash-handoff-build-20261028.json`. This unsigned update is staged only, not installed or published; full IDM parity remains unproven.
