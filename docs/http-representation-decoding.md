# HTTP representation decoding

## Why this path exists

The isolated 7.43 comparison found a concrete parity gap: IDM decoded the
server's gzip response, while UDM rejected the same response because it did
not honor `Accept-Encoding: identity`. The original probe is retained in
`validation/http-gzip-foundation-7.43.json`. This observation concerns file
contents, not a paired speed measurement.

Version 7.44 adds gzip decoding for ordinary HTTP GET downloads. Version 7.45
extends gzip decoding and completed gzip-response reuse to form downloads,
including POST-to-GET redirects. Streaming resources, ZIP range previews,
synchronization probes and website crawling keep their existing encoding
admission rules. Brotli and stacked HTTP content encodings are not added.

## Transport and output are different coordinate systems

- While downloading, `Size`, `Received` and `Segments` describe encoded bytes.
  HTTP Range and If-Range use these offsets and the existing validator and
  overlap checks. A changed content encoding invalidates the old identity.
- Gzip starts on one full response. It does not issue an extra helper probe.
  An interrupted stream may resume encoded offsets after validation.
- The encoded spool remains the recovery source. A completed receipt binds
  URL, protected headers, encoding, size, segment plan and segment hashes.
  Form receipts also bind the protected request fingerprint and redirect URL/
  headers. They do not duplicate the encrypted request body in the catalog.
  Reopening or resuming verifies the saved bytes before bypassing the server.
- Decoding writes a separate unpublished file with bounded buffers. The
  decoder checks member structure, end-of-stream and gzip checksums. A corrupt
  stream invalidates its receipt durably and schedules the old spool for reset
  on the next attempt. Cancellation and destination write failures retain the
  original spool.
- A server representation digest is checked against encoded bytes. A user
  ExpectedSha256 is checked against the final output, including any approved
  removal of an inserted prefix.
- The existing content detector and prefix policy inspect decoded bytes.
  Publication uses the existing journal and atomic destination rename.

## Visible size and progress

Compressed Content-Length is not the final file size. File Info and output
progress initially show unknown final size. After decoding and verification,
prefetch publishes `PrefetchedFinalSize` while waiting for Start; the final
destination remains absent. Start confirms the existing job rather than
creating a second download.

`HttpDecodedReceived` and `HttpDecodedSize` supply a display-only projection.
This projection must never overwrite resumable wire offsets. Network speed
still measures received network bytes; it is not multiplied by compression
ratio or manufactured to show a burst. ETA is suppressed while encoded and
decoded rates would otherwise be mixed.

Completion records output Size/Received and a corresponding final segment map,
while retaining HttpWireSize/HttpWireReceived for diagnosis.

## Form response ownership in 7.45

Fresh forms remain excluded from speculative prefetch: opening File Info is not
permission to submit a POST. Once a gzip response has been received completely,
its encoded spool and fingerprint-bound receipt can survive a final checksum
rejection or another publication-stage failure. Reopening File Info may verify
and decode that response without contacting the origin, show the exact decoded
size, and wait for Start or Download Later.

The UI and queue use a cheap catalog-only candidate check. The transfer worker
computes the actual current request fingerprint and checks every saved segment
hash before reuse. Cached UI metadata is never proof of saved bytes or request
identity. Request, header, encoding or redirect changes cannot silently reuse an
old response.

Resume of a matching candidate preserves PostAttempted. Failed byte verification
invalidates the receipt and fails without another POST. The next separately
displayed Restart rearms submission and clears any old POST-to-GET continuation.
Interrupted forms without a complete response keep the existing explicit
Restart behavior. Queue failure retries still exclude POST requests.

The progress view takes its saved-response candidate from authoritative wire
metadata before projecting decoded output. Otherwise its output-sized segment
map would incorrectly invalidate the wire receipt and advertise Restart.

Transfer properties such as the expected checksum cannot be edited during
active prefetch. Rejected edits roll back without closing confirmation. The
publication path also rechecks the confirmed expected final digest.

## Plain forms in 7.46 and ASCII PDF notices in 7.47

The 7.45 core baseline received a complete plain HTTP form response but failed
to retain a reusable receipt after final checksum rejection. Version 7.46 adds
that checkpoint on the failure path. A successful plain form keeps its inline
hash and does not add a response-receipt pass. An observer checked this in the
actual production-core fixture; this is not a millisecond speed comparison.

Only a response that reached EOF in the current attempt can acquire a new
failure-path receipt. Canceled or incomplete attempts are excluded. A previously
rejected response cannot be rebound to a changed request. Receipt-checkpoint
failure preserves the original transfer error and records a separate retention
warning; it does not promise that Resume can reuse the response.

Pending publication journals retain ownership. A real injected catalog-save
failure after destination publication leaves that journal intact. Resume
finishes publication recovery without another POST, instead of creating a
competing form-response receipt. A separate destination-collision case preserves
the existing file and publishes saved response bytes after changing the folder.

Retained plain responses can reopen File Info, supply verified output size
before Start, survive Download Later and catalog reopening, and honor an
approved inserted-prefix removal. Completed records discard the obsolete
receipt. For prefix-stripped form output, completion normalizes range coverage
to final file bytes while preserving the original response byte counts.

Qualification of 7.46 exposed an additional detector gap: an ASCII-only PDF after
an HTML notice was classified as HTML because the whole prefix window was text.
Version 7.47 recognizes a standalone numeric PDF header after a closed HTML/body
notice and intervening whitespace, within the existing 2,000-byte prefix bound.
It retains the existing short-window pending decision and leaves embedded HTML/
script examples and malformed headers classified as HTML. This is bounded
header recognition, not full PDF document validation. The format reference is
Adobe's [PDF 1.7 reference](https://opensource.adobe.com/dc-acrobat-sdk-docs/pdfstandards/pdfreference1.7old.pdf).

The final 7.47 qualification passed 278 targeted checks: 53 plain-form, 49 gzip
form, 36 GET, 17 decoder, 24 notice, 17 filename and 82 recovery/size checks.
Before-Start sizes of 88,000 bytes and 395 prefix-stripped bytes matched the
completed outputs. Eleven plain-form outputs were independently hash checked;
the destination sentinel remained unchanged. The original failing baseline and
7.46 detector failure remain recorded. No installed-browser or timing parity is
inferred from these results. [Qualification](validation/plain-form-recovery-7.47.json)

## Validation entry points

`HttpGzipDecoderTests` checks arbitrary chunk boundaries, dynamic-Huffman input,
concatenated and empty members, invalid CRC/truncation/trailing data, output and
member limits, cancellation and writer failures.

`HttpGzipTransferTests` requires an explicit loopback server supplied by
`tests/http-gzip.native-live.cjs`. The runner generates its payload independently
with Node zlib and checks exact SHA-256, pre-Start size, publication, prefetch
reuse, one-response behavior and validated encoded-offset resume. It never
uses the personal catalog.

`HttpPostGzipTests` is a manual backend target driven by
`tests/http-post-gzip.native-live.cjs`. It checks exact binary request bodies,
known/unknown response lengths, redirects, pre-Start size of retained responses,
Download Later/reopen reuse, rejected property edits, changed request binding,
damaged saved data, explicit-only restart and ambiguous connection loss.

`HttpPostIdentityTests`, driven by `tests/http-post-identity.native-live.cjs`,
checks plain form success without a receipt pass, failure-path retention,
unknown/zero lengths, redirects, folder changes, pending publication recovery,
changed requests, damage, prefix choices and reopening. `ContentSniffNoticeTests`
qualifies the ASCII PDF rule and HTML false-positive boundaries.

Current qualification executables share
`candidates/native-build-current/release-native`, replacing the current
build instead of adding versioned executable copies. Older qualification used
`%LOCALAPPDATA%\UDM-Development\native-validation-current`; that older directory
does not identify the latest build. Each version's
documentation retains executable/source hashes and results. This does not
remove older historical artifacts or make the cache a permanent archive.

The 7.45 qualification passed 49 form integration, 36 GET integration, 17
decoder, 256 progress component and 82 recovery/size checks. The retained form
case announced 140,000 bytes before Start and published that exact length and
SHA-256. Eight persisted form outputs were independently hash checked.
[Qualification](validation/form-gzip-7.45.json)

`tests/http-content-encoding.native-live.cjs` compares the actual UDM and IDM
executables with the same owned origin. IDM receives only explicitly named
fixture downloads; its settings, existing queue and process are preserved.

`tests/http-post-encoding.paired.cjs` uses the installed reference through its
[published COM API](https://www.internetdownloadmanager.com/support/idm_api.html),
via the local-only `benchmarks/speed/idm_post_probe.cpp` helper. The 7.44 baseline
rejected gzip POST while IDM decoded it. The updated actual 7.45 executable and
IDM both produced the same complete 67,200-byte response file. UDM sent one POST
per case, preserving the exact declared 25-byte API body. IDM's 26-byte request
hash matches that body plus a trailing NUL; it also retried POST ranges on the
identity origin that deliberately ignores Range. The fixture returns a fixed
payload, so this proves response decoding, not equivalence of arbitrary forms
or safe replay behavior. UDM does not copy the repeated submission behavior.

### Gzip alias follow-up in 7.55

The actual 7.54 executable rejected the same controlled gzip bytes when the
response used `Content-Encoding: x-gzip`; installed IDM 6.43.10.2 decoded them
to the expected 67,200 bytes. The 7.55 transport normalizes that alias to
`gzip` before acceptance, storage and resume comparison. It uses the existing
decoder, checksum checks, encoded offsets and verified-response receipts.
The HTTP specification treats `x-gzip` as equivalent to `gzip`.
[RFC 9110, section 8.4.1.3](https://www.rfc-editor.org/rfc/rfc9110.html#section-8.4.1.3)

The GET and form integration runners accept `UDM_GZIP_ENCODING=x-gzip` or
`X-GZip`; GET additionally accepts `UDM_GZIP_RESUME_CANONICAL=1` to switch a
resumed response back to `gzip`. The paired actual-executable runner accepts
`UDM_HTTP_PROBE_ENCODING=x-gzip`. Defaults remain ordinary gzip. The discovery
probe also observed IDM saving the encoded body for `deflate`, while UDM
rejected it; that result is separate from decoded-output parity and does not
justify treating arbitrary content encodings as identity bytes.

[7.55 qualification, comparison and limits](validation/gzip-alias-7.55.json)
records the targeted GET/form cases and paired output hashes. The separate
[cleanup receipt](validation/gzip-alias-7.55-cleanup.json) preserves hashes for
the removed synthetic payloads and encoded spools.

### Destination-volume staging in 7.56

Gzip decoding now writes `.udm-<job id>.assembling` beside the saved
destination, after checking publication recovery and folder restrictions.
The encoded spool and receipt stay in their original temporary location.
Decoded bytes are flushed and validated before the same journaled publication
boundary. Normal publication can rename the sibling without a cross-volume
copy and second output hash. If confirmation changes the folder, the existing
publication helper relocates and verifies the already decoded file.

The GET fixture accepts an optional `UDM_GZIP_DEST_ROOT` and requires a fresh
folder. The 7.56 qualification holds and hashes the actual decoded sibling on
C: before Start while state and encoded data are on D:, then separately checks
a destination edit back to D: without another response. Canonical, alias and
mixed-case gzip names pass, alongside form/recovery checks. The actual
four-MiB before/after outputs match, but the single descriptive runs do not
establish a consistent latency improvement; the baseline catalog was observed
before final phase telemetry persisted.
[Qualification and limits](validation/gzip-destination-7.56.json)

## Remaining work

Decoding currently follows network EOF instead of overlapping network and
decode writes. Performance against IDM, especially on slow disks and highly
compressed large files, remains unproven. Broader redirect, large staged-body,
cancellation/crash and disk-fault acceptance remain separate work. A redirected
response without an inline hash may require a local full-file read when saving
its failure-path receipt. Supporting other content
encodings needs the same wire/output separation, not automatic
decompression that loses HTTP byte offsets. Personal desktop visual inspection
and installed-browser acceptance remain separate from isolated core tests.
