# Live HLS expired delta retry

When a live delta playlist skips media beyond the recorder's retained history, UDM now requests a complete playlist by removing HLS delivery directives from the playlist URL. It then parses the full window and lets the recording timeline keep already saved segments while appending new ones. The retry applies only to the specific missing-history condition; conflicting or malformed delta playlists still fail.

An isolated HTTP fixture first serves segments 0–1, then a delta that skips unavailable segment 2, then a complete window containing segments 1–3. The native test verifies that the recording completes with four downloaded segments, no duplicate media, and a probeable output of at least eight seconds.

Validation on 2026-10-15: the newly compiled `Udm.NativeTests.exe` passed **2,629 checks, 0 failed**, including this fixture. The desktop app also compiled successfully with `native/build.ps1 -AppOnly`. The fixture does not prove behavior against every live CDN or an installed application.
