# YouTube main-player readiness in UDM 5.60

The controlled pre-roll fixture reproduced a mismatch in 5.59. A readiness marker for the current video made three overlays visible, including a second video in the same player and a separate ad. The catalog still waited for player metadata before it could prepare in the background.

5.60 scopes the readiness override to the primary video inside YouTube's main player. If there are several video elements and no unique primary, the override waits. A separate ad flag on a nested element still applies. The selected player can prepare the catalog while it reports no metadata. The preparation uses the existing request budget, and the background still checks the captured session, page identity and download policy.

- The isolated fixture passed six checks per bundle in Edge. The Chromium bundle repeated all six in Chrome. The first click showed both prepared formats with one total catalog lookup and zero download handoffs.
- The isolated source-change fixture passed 24 checks across both bundles. The Chromium lifecycle fixture passed 18 in Edge.
- The YouTube ad-marker and page-capture suites passed 264 combined checks.

The early timing result comes from a controlled pre-roll page. A live comparison of public YouTube ad timing against IDM remains open.
