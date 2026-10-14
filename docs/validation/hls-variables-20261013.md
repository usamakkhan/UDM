# HLS variable substitution

UDM now expands local `NAME`/`VALUE`, media-playlist `IMPORT`, and playlist `QUERYPARAM` definitions in clear HLS playlists. Definitions must precede use; duplicate, missing, malformed, or unsafe values fail before a native download. Browser catalog discovery carries definitions from each declaring master into its media, audio, and subtitle children. Recorded plans contain resolved media URLs. Live plans also carry bounded imported values so the native recorder can resolve each later playlist reload. Live variable handoff requires the new `hls-variables` native capability.

The implementation follows [HTTP Live Streaming 2nd Edition, draft 21](https://datatracker.ietf.org/doc/draft-pantos-hls-rfc8216bis/21/), sections 4.3, 4.4.2.3 and 6.3.1. Variable replacement is single-pass; it does not recursively expand text inserted by a definition.

Validation on 2026-10-13:

- All 58 browser `tests/*.test.cjs` files passed. Focused variable parsing checks cover both browser bundles, master/leaf imports, map byte ranges, query values, nested audio masters, and rejection cases.
- Browser site integration checks exercise recorded and live video, audio and subtitle plans, plus rejection when an older desktop lacks the capability.
- A follow-up handoff check confirms live audio behind a nested master sends that immediate master's variables to native playlist reloads, even when the outer master defines the same name differently. Both browser bundles pass; the full 58-file browser suite remains green.
- The final native suite passed 2,614 checks with zero failures, including local, imported, query, and map byte-range variables.
- The packaged `Udm.NativeHost.exe` answered a framed `hello` request with the `hls-variables` capability after the final rebuild.

An [isolated Edge/native transfer](hls-variables-browser-native-20261013.json) confirms both recorded and live variable-bearing playlists produce decodable MP4 output with audio and five signed resource requests each. The live recorder refreshed the playlist three times and completed four segments. A nested live-audio case also passed: the video and audio playlists refreshed independently, each fetched resources with its own master-defined token, and the MP4 contained the selected 880 Hz audio. No installed app or release was changed.
