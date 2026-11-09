# Track selection while a video catalog changes in UDM 5.58

The browser can publish provisional and final catalogs during the same panel visit. Site offers bind the format, media source, audio and subtitle options to a key. The background retains previously published adaptive keys briefly so an in-progress selection can finish.

In 5.57 an open track chooser still enabled its download actions after a later catalog replaced its key. The controlled Edge fixture reproduced that behavior in both extension bundles. The replacement has the same displayed quality and a different key.

UDM 5.58 keeps the chooser visible and disables its download actions if an accepted catalog revision drops the exact offer. If a download action had focus, focus moves to Back. The warning directs the user to the current list. If the same offer returns while the chooser is open, the actions become available again without rebuilding the chooser. The existing background offer validation still applies.

The keyboard fixture passed six checks per bundle in isolated Edge. The Chromium bundle repeated all six in Chrome. The two bundles passed 24 source-race and 18 lifecycle checks in total. Five ad-transition checks passed in Chrome. The five versioned native binaries and both extension bundles are staged. [Build receipt](panel-track-revision-5.58-build.json)

The isolated Edge extension-to-desktop fixture passed 25 checks with the staged native host and app. It completed five downloads across MP4, TS and audio-only M4A, decoded each through EOF, and verified the selected audio tone and language. The subtitle case preserved Unicode, language, timing and unique cues. The fixture removed its temporary native-host registration. The catalog replacement itself was exercised by the controlled keyboard fixture.
