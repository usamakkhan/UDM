# Recorded HLS initialization changes - validated and installed

Implemented recorded HLS assembly across changing EXT-X-MAP initialization sections, including different fragmented MP4 track timescales. Local playlists preserve initialization references and discontinuities. Chromium and Firefox require the desktop capability hls-recorded-map-changes before handing off these plans. Changing subtitle initialization sections remain explicitly unsupported.

Validation: 9 native media-output checks, 8 isolated launch/native-bridge checks, 47 browser HLS handoff checks, and 32 cross-site media checks passed. Native fixtures verified full duration, 40 decoded frames, and error-free decoding both with and without discontinuities. The final build includes capability advertisement and subtitle validation. Git diff --check passed.

Installed UDM.exe and Udm.NativeHost.exe on 2026-10-16. All 29 catalog records were preserved, and the catalog hash was unchanged during replacement. Previous executables and catalog are backed up at %LOCALAPPDATA%/UDM/update-backups/20261016-hls-maps. Installation hashes are recorded in installed-hls-maps-20261016.json.

Post-install verification confirmed UDM process 14016 responding and the installed native host successfully reporting hls-recorded-map-changes. Existing browser sessions still require an extension reload to activate the edited scripts; live browser activation has not been verified.

Evidence: %LOCALAPPDATA%/Temp/udm-recorded-maps-final-20261016/results.json and %LOCALAPPDATA%/Temp/udm-maps-launch-final-20261016/launch-results.json.

This validates the scoped recorded-HLS feature, not full IDM parity or comparative download performance.
