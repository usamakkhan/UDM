# Keyboard focus during video catalog updates in UDM 5.57

The 5.56 control replaced a focused audio and subtitle options button when an earlier format appeared. It retained focus on main format rows only. Both staged 5.56 bundles reproduced the options focus loss in the controlled Edge fixture.

UDM 5.57 gives each format action and its track options separate focus identities. The audio-only and Other qualities entries also have stable identities. Back from a track chooser returns to the opening control, even if a finalized catalog changed that row's label or position while the chooser was open. If the opening control disappears, focus moves to the first remaining format or the panel trigger.

The 5.57 Chromium and Firefox content bundles each passed four focus checks in isolated Edge. The Chromium bundle repeated all four checks in Chrome. They covered Arrow Down, reordered results, final catalog arrival while the track chooser was open, Back from both chooser entry points, and Escape. Both bundles passed 19 panel control checks each, 24 source-race checks in total and 18 lifecycle checks in total. Five ad-transition checks passed in Chrome, including the verified video becoming ready while the ad flag was still present. [Build receipt](panel-focus-5.57-build.json)

The legacy `video-menu.browser.cjs` fixture failed on both builds because it looked for Download all after opening the separate panel options menu. The focused regression uses its own catalog and keyboard fixture. These runs exercise controlled pages. They do not measure a public pre-roll transition or live Firefox behavior.
