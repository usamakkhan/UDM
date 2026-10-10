# Changes from 0.84.0 RC3 to 0.85.0 RC1

Native version advances to 0.85.0 and both browser manifests advance to 0.63.0. The installer uses those versions as well.

- Preserve single-use download links while previewing or probing a transfer. A disposable range request no longer consumes the only usable response before the download starts.
- Retain the initial full HTTP response for a new transfer. Recovery still checks established validators before publishing resumed bytes.
- Support Digest authentication for explicit HTTP proxies, with proxy challenges kept separate from origin authentication.
- Refresh versioned browser recognition tests so they validate the current manifests.

The product changes are commits `ae7535e` through `251deef` after tag `v0.84.0-rc.3`. Validation and release documentation follow in the RC1 release commit. These changes improve specific backend cases; they do not establish complete IDM feature parity.
