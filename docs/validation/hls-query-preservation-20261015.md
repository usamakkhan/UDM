# HLS complete-playlist retry query preservation

The native live recorder now removes `_HLS_skip`, `_HLS_msn`, and `_HLS_part` without rewriting other query fields, including empty fields and trailing separators. This matters for servers that verify the exact signed query string when a delta playlist must be retried as a complete playlist.

The isolated HTTP fixture accepts only the exact directed URL and its byte-preserving complete form. Its initial-delta recording completed and decoded after the retry. Browser catalog checks assert the same query behavior for Chromium and Firefox.

Validation on 2026-10-15: **2,629 native checks passed, 0 failed**; **39 HLS browser integration checks passed**; `native/build.ps1 -AppOnly` produced the desktop app successfully. These are fixture results, not installed-app or public CDN acceptance.
