# Changes from 0.85.0 RC1 to RC2

RC2 adds application-owned capture of original HTTP responses through the signed network helper. It assigns responses to live browser process trees, admits bounded bodies, saves recovery receipts, and publishes verified downloads into the native catalog. Cancellation, pause, restart reconciliation, filename selection, progress, completion, and transfer duration now follow the captured response. The desktop Downloads menu exposes the integration while queue ticks continue if capture polling fails.

YouTube handling excludes explicitly ad-marked media in browser capture and native fallback. After a captured response is durably published, UDM removes its duplicate temporary body when possible.

Native binaries remain 0.85.0 and browser manifests remain 0.63.0. This RC2 source is based on product commit `334bcbd` plus the sanitized storage fix `7d2e393`; newer validator work on main is outside this release. The original-response route remains HTTP-only, and complete IDM parity is not established.
