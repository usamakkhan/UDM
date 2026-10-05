# Browser 0.55 / native 0.83 candidate

Installer delivered; source stays staged in D:\UDM-Workspace\candidates\browser-055-nested-audio\project. The verified 0.83/0.54 installer is retained. No personal app/extension activation or source promotion occurred.

Nested HLS audio renditions now traverse up to six masters, select the highest declared audio-only bitrate, follow redirect-relative URLs and preserve the chosen language. Video variants, alternate audio-group substitutions, cycles and ambiguous codec declarations are rejected. Existing preparation deadlines remain in force.

Acceptance:
- 48 browser regression scripts passed, including 18 new Chromium/Firefox nested-audio checks.
- Actual Edge: 68 checks, ten decoded outputs with selected language/tone, containers and subtitles.
- Actual Firefox: four grouped checks cover private identity, real video detection/format click, one decoded job/receipt acknowledgement, and English 440 Hz nested audio.
- Actual Edge cyclic-audio error/retry: ten checks; visible error, no initial native admission/receipt, enabled retry, then one decoded M4A.
- Screenshots reviewed; unique host registrations removed. Native binaries match the final verified 0.83 build.
- Installer inputs were fingerprinted before compilation and verified afterward. Clean-machine installation remains untested.

[Installer](D:/UDM/installer-out/UDM-0.83.0-Browser-0.55.0-Setup-x64.exe)

SHA-256: b0f5b3fe0df310c2826d380c500b3e9bdaa5edb8a8216d3d9c9cc1f2f4e551dc

The extension supports declared audio-only nested variants, not arbitrary malformed or mixed-media masters. Synthetic tests do not establish coverage of every video site, encrypted media, IDM speed equivalence or full parity. User activation remains pending.
