# Recorded DASH audio-only presentations

UDM previously parsed MP4 audio tracks but rejected the presentation if it contained no supported video representation. The browser now exposes a recorded audio-only DASH presentation as M4A, using the existing native audio-only transfer and muxing path.

The default audio track follows the existing main-role/bandwidth ordering. Available languages and alternate tracks remain selectable. The browser catalog carries an explicit audio-only flag, labels the offer as M4A, applies M4A panel filtering, and forces audio output at handoff even when the ordinary download button is used. The options screen hides Download video and focuses an attached audio action when there is no track selector. Subtitle choices are omitted for this output.

Existing segment URLs, ranges, initialization sections, SIDX expansion, credential scoping, declared duration, cancellation and handoff checks remain in use. This does not enable dynamic MPDs, multiple Periods, protected content, non-MP4 audio, or change which page players receive a panel. It does not establish that every IDM audio capture case is supported.

## Build status

Chromium and Firefox extension versions are 0.63.57. JavaScript syntax checks passed for all three changed scripts in both families; their mirrored scripts match byte for byte. The installer uses the previously compiled native 1.0.4 production payload because native source did not change in this increment. See `validation/dash-audio-only-build-20261028.json` for packaging and hashes.

Runtime, browser, download-completeness and output-size testing are deferred at the user’s request. This candidate is not installed or published and remains unsigned.
