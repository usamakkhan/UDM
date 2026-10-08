# Live HLS subtitle offer correction

An HLS master playlist can advertise subtitles beside a live video rendition. UDM's native live recorder accepts one or two media playlists and rejects subtitle tracks, but the browser catalog previously displayed those subtitle options for the live rendition. Selecting one then failed during handoff.

The Chromium and Firefox HLS catalogs now omit subtitle options from live leaf choices, and both site offer builders keep that limit when publishing panel choices. Recorded HLS and DASH subtitles remain selectable. The focused checks passed: 18 HLS catalog cases, 9 site-catalog cases, and 31 audio/subtitle cases. A live master with an advertised subtitle group is covered in both browser families.

This is a local source candidate, not installed or released. It does not add live subtitle recording; that needs native capture, timing, assembly, and restart acceptance before the option can be exposed.
