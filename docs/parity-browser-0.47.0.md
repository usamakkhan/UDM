# UDM browser 0.47.0 with native 0.69.0

UDM can now capture downloads returned by supported UTF-8 multipart text forms in Edge. It preserves the original field order, repeated names, encoding, MIME boundary and clicked submitter, checks the request against browser metadata, and keeps same-origin redirect continuity.

**846 checks passed:** 818 browser unit checks, 8 native protocol checks and 20 real Edge assertions. Eight multipart cases reached native UDM with byte-identical request bodies and verified downloaded files. File uploads with unavailable bytes and oversized forms completed in Edge.

[Evidence](evidence-browser-0.47.0/summary.json) · [Detailed behavior and limitations](evidence-browser-0.47.0/DEVELOPMENT.md)

Native UDM remains 0.69.0. The driver is unchanged. File/blob uploads, non-UTF-8 or script-created multipart requests and broader site/browser coverage remain open. Full IDM parity is not established.

Reload UDM Browser Integration in **edge://extensions**, then refresh pages. Computer control still fails before initialization, so personal-session activation remains unverified.

## Installation on this PC

Browser files and the combined installer are installed. All 23 history records, queues and preferences are unchanged. Native executable and network runtime hashes are unchanged. Edge requires an extension reload; activation of the personal session is not yet verified.

[Installer](../installer-out/UDM-0.69.0-Browser-0.47.0-Setup-x64.exe), SHA-256 `797cc6cc9e5aafb4cbdd9c8551df819995ea37ba90289beeac24e8516f9cbab0`.
