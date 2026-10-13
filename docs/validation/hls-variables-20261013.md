# HLS variable substitution

UDM now expands local `NAME`/`VALUE`, media-playlist `IMPORT`, and playlist `QUERYPARAM` definitions in clear HLS playlists. Definitions must precede use; duplicate, missing, malformed, or unsafe values fail before a native download. Browser catalog discovery carries definitions from each declaring master into its media, audio, and subtitle children. Recorded plans contain resolved media URLs. Live plans also carry bounded imported values so the native recorder can resolve each later playlist reload. Live variable handoff requires the new `hls-variables` native capability.

The implementation follows [HTTP Live Streaming 2nd Edition, draft 21](https://datatracker.ietf.org/doc/draft-pantos-hls-rfc8216bis/21/), sections 4.3, 4.4.2.3 and 6.3.1. Variable replacement is single-pass; it does not recursively expand text inserted by a definition.

Validation on 2026-10-13:

- All 58 browser `tests/*.test.cjs` files passed. Focused variable parsing checks cover both browser bundles, master/leaf imports, map byte ranges, query values, nested audio masters, and rejection cases.
- Browser site integration checks exercise recorded and live video, audio and subtitle plans, plus rejection when an older desktop lacks the capability.
- The final native suite passed 2,614 checks with zero failures, including local, imported, query, and map byte-range variables.
- The packaged `Udm.NativeHost.exe` answered a framed `hello` request with the `hls-variables` capability after the final rebuild.

These are parser and handoff-plan checks plus native controlled fixtures. An isolated real-browser transfer for variable-bearing HLS was not run. No installed app or release was changed.
