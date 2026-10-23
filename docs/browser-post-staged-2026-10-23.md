# Captured POST handoff: staged bodies above 4 MiB (2026-10-23)

Captured form bodies above the 4 MiB inline limit no longer stay in the browser. The
extension hands the bytes to the desktop in ordered base64 chunks over the existing
native host pipe, and the download record keeps a reference instead of the bytes.

## Protocol

| Message | Fields | Reply |
|---|---|---|
| `post-body-begin` | `size` (4 MiB < size <= 64 MiB), `sha256` (hex), `contentType` | `bodyToken` (32 hex), `chunkBytes` (4 MiB raw per chunk) |
| `post-body-chunk` | `bodyToken`, `offset` (must equal the bytes received so far), `data` (canonical base64, at most `chunkBytes` raw) | `received` |
| `post-body-finish` | `bodyToken` | `request`: `{method, contentType, bodyFile, bodySize, bodySha256}` |

The `add` (or `capture-prepare`) message then carries that `request` in place of the
inline `body`. `hello` and `preferences` advertise `stagedPostBodyLimit`,
`postBodyChunkBytes` and the capability `post-body-chunks`; an extension talking to an
older desktop sees no `stagedPostBodyLimit` and keeps the download in the browser
without pausing it.

## Desktop storage

Chunks append to `<data>/requests/<token>.part`. `finish` checks the declared size and
SHA-256, DPAPI-protects the bytes for the Windows account, writes `<token>.bin` by
rename, and removes the part file; any failure removes both. `Transfer` resolves the
reference, re-verifies size and checksum, and only then marks the submission attempted,
so a missing or damaged body file stops before any request. Body files are deleted when
no record references them (redownload shares a reference) and part files no staging owns
are pruned at startup. At most eight stagings are open at once; an abandoned one expires
after an hour. Catalog exports never carry a staged reference, even with credentials:
such records import as needing browser recapture.

## Checks

Native (`PostChecks.hpp`, in the full suite): a 6 MiB + 13 body containing every byte
value (so not valid UTF-8) is staged in two chunks, rejected out of order, past its
size or above the frame budget, sealed with no plaintext run on disk, submitted once
with exactly the original bytes, survives reopening the catalog, is shared by redownload,
is deleted with its last reference, and is refused when its checksum does not match or
its file is missing. Extension (`tests/post-handoff.test.cjs`): a 5 MiB + 13 body goes
out as six ordered chunks and the add references the sealed body; a desktop without
staging, a body above the staged limit, and a refused staging never submit a download.

Not run: live browser acceptance (real Chromium or Firefox against the built host).
Multipart forms with file inputs still use the 4 MiB verified-snapshot path.
