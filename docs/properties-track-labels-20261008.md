# Properties track labels and menu review — 2026-10-08

Properties > Links previously called live subtitle playlists and subtitle segments video links. The model now names video, audio and subtitle tracks distinctly while preserving addresses, primary-address order and protected catalog metadata. Unknown track kinds receive a neutral media label.

Two failing label assertions reproduced on the prior source. All 19 focused address/search checks pass after the correction. The rebuilt app and existing menu workflow suite also pass 349 checks: reference menu shape and labels, selection enablement, queue membership and indicators, font preferences, pause/stop behavior, and resumed transfer byte integrity. These are private-profile native component checks, not exhaustive rendered-dialog or physical keyboard/mixed-monitor acceptance.

The 1 October menu audit is now explicitly marked historical: its missing Tasks/File/View/Help routes have since been implemented. The original reference evidence is retained.

[Validation receipt](validation/properties-track-labels-20261008.json). App candidate: candidates/properties-track-labels-20261008/release-native/UDM.exe. Native 0.84.0, browser source 0.62.11. App and menu test were rebuilt against previously compiled matching backend objects, including the latest mapped live subtitle implementation. No clean full-native rebuild was performed.

Not installed, packaged or published. Latest installer remains browser 0.62.10 with the earlier native backend. Public-site and controlled IDM performance comparison, full visual/accessibility parity and installer lifecycle qualification remain open.
