# UDM 0.74.0 / browser 0.47.1 — repeated-cancel exclusions

After two consecutive cancelled automatic browser offers from one file server, UDM offers exact-address or site exclusion and Don't show again. File Types > Edit list adds Add/Delete and prompt re-enabling. Choices persist only after acceptance, preserve unrelated settings and roll back on a failed save.

Exact URLs match literally, including asterisks and query strings. Wildcard patterns still work. Firefox regeneration now preserves all background dependencies, including multipart.js. Network-helper messages no longer claim that the unsigned helper is signed.

**2,821 checks pass**, including real native dialogs and native-host framing. [Evidence](evidence-0.74.0/summary.json), [behavior and limitations](evidence-0.74.0/DEVELOPMENT.md), [cancellation offer](evidence-0.74.0/cancel-offer.png), [exception list](evidence-0.74.0/address-exceptions.png).

Reload UDM Browser Integration in Edge and refresh pages to activate browser 0.47.1. Personal-session activation and precise IDM counter/reset comparison remain unverified. This is not a file-recognition, driver or speed update; full IDM parity is unfinished.
