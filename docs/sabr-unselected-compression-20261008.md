# SABR unselected compressed track candidate

The streaming parser previously rejected a compressed segment header before checking whether it belonged to the selected video or audio tracks. A compressed unrelated side track could therefore stop a valid selected download. The native fixture reproduced this against the unchanged parser: the suite stopped at the new mixed-track case after 2,290 passes with `Compressed streaming segments are not supported.`

The parser now records an unselected segment as skipped before applying the compression restriction. Selected compressed segments remain rejected. The fixture includes both paths and verifies the selected video and audio bytes after the unrelated compressed segment. This does not add decompression support or weaken rejection of encrypted media.

The rebuilt isolated native suite passed 2,570 checks with zero failures, including both new compression cases. This is a source/binary candidate, not an installed release. Actual public-site SABR variants and IDM behavior were not measured in this check.

## Selected gzip segment candidate — 8 October 2026

Selected segments using compression type 1 now decode through zlib 1.3.2 after the declared on-wire length and MEDIA_END checks. The native decoder validates gzip checksums, supports concatenated members, caps the total decoded segment at 64 MiB and caps member count at 1,024. It checks cancellation during decoding and removes decoded and compressed temporary files when decoding fails. Cache publication follows successful decoding; unselected compressed tracks retain their skip behavior. Brotli/type 2 remains unsupported.

The source archive was obtained from https://zlib.net/zlib132.zip and checked against the SHA-256 published at https://zlib.net/zlib.html. An unmodified inflate/checksum source subset and its license are vendored; the native build compiles it statically and copies zlib-LICENSE.txt, and the installer includes that notice. No separate runtime DLL is required. Protocol mapping: https://github.com/LuanRT/googlevideo/blob/main/protos/misc/common.proto; on-wire length/decompression ordering: https://priveetee.github.io/Docs-PipePipe/extractor/sabr-media.html.

The focused native harness passed 112 checks: gzip video/audio split across responses, CRC failure, truncation, trailing data, unsupported compression, wire-length mismatch, decoded-size overflow, failure cleanup, concatenated members, plus existing media/parallel/audio regressions. A 65,250-byte generated gzip fixture expands to 64 MiB plus one byte; its test requires the precise decoded-size error. Portable evidence: [validation/sabr-gzip-20261008.json](validation/sabr-gzip-20261008.json). Raw logs: `candidates/sabr-gzip-20261008`.

This is a source candidate, not installed or packaged. The full native suite and public-site gzip capture have not yet been run for this change. The previous installed continuation build remains intact. This supersedes the earlier blanket statement that all selected compressed segments are unsupported, but does not establish Brotli support or IDM parity.

## Packaged gzip candidate

The full native app, host, monitor and setup helper built successfully. The full native suite passed 2,572 checks with zero failures. The exact app, host and extension files staged for the installer passed 10 isolated Edge and 9 isolated Firefox indexed-DASH checks; both temporary native-host registrations were removed. The separately recorded 112-check focused harness provides gzip-specific acceptance. All recorded native-source and package-input hashes remained unchanged.

Inno Setup produced `candidates/gzip-release-20261008/project/installer-out/UDM-0.84.0-Browser-0.62.8-Setup-x64.exe`, SHA-256 `90970CBD96F3D3C024B57C17F88D7ADB98BF2556B3F17E9452FE68074A13B1D3`. The package includes zlib-LICENSE.txt. Portable receipt: [validation/gzip-release-20261008.json](validation/gzip-release-20261008.json).

This package has not been installed, published, or tested on a clean machine. The installed UDM process was running throughout packaging and was left intact. Public-site gzip behavior, Brotli, and full IDM parity remain open.

## Brotli source candidate — 8 October 2026

Selected compression type 2 now uses the official Brotli 1.2.0 decoder/common source subset, pinned to commit `028fb5a23661f123017c060daa546b55cf4bde29`. The source was fetched from the official repository at that commit; the archive hash is recorded in vendor/brotli/PROVENANCE.json, and every vendored file was compared with the downloaded source. Only decoder/common C files are compiled. The build copies Brotli-LICENSE.txt and the installer includes it. No new runtime DLL is needed.

The decoder preserves wire-length validation, writes bounded temporary output, checks cancellation, rejects trailing data and truncation, and applies the existing 64 MiB decoded-segment limit before cache publication. Brotli itself does not supply gzip-style checksums; this change does not claim otherwise. Both gzip and Brotli now allow the decoder to drain buffered output after encoded input EOF. The focused harness passed 127 checks, including split-response decoding, malformed Brotli, unsupported algorithm IDs, truncation/trailing bytes, declared length mismatch, precise over-limit failures, temporary-file cleanup, output draining, and all previously included media/parallel/audio cases. Portable evidence: [validation/sabr-brotli-20261008.json](validation/sabr-brotli-20261008.json); raw build/result files: `candidates/sabr-brotli-20261008`.

This candidate is committed source only: no full native suite, new installer, installation, or public-site Brotli capture has been qualified yet. The staged gzip installer and installed continuation build remain unchanged. Earlier statements that Brotli is unsupported refer to those earlier builds. Full IDM parity remains unestablished.

## Packaged gzip and Brotli candidate

Source b574c25 was built into the full native app and companions. The full native suite passed 2,572 checks with zero failures. The earlier 127-check harness supplies compression-specific evidence. The installer compiled successfully and includes both decoder licenses; all recorded native-source and package-input hashes stayed unchanged.

The first Edge indexed-DASH run passed bridge connection and panel discovery, then timed out preparing the selected video index before creating a native download. Its saved panel displayed the preparation-timeout error. Installer compression was running concurrently, but no causal relationship was established. After the installer finished, a fresh two-cycle Edge recheck passed all 18 checks and Firefox passed all 9 checks against the same staged binaries. Both recheck browser registrations and the failed run's registration were removed. The initial failure is retained under candidates/brotli-release-20261008/edge-media; it is not counted as passing.

The package is staged at `candidates/brotli-release-20261008/project/installer-out/UDM-0.84.0-Browser-0.62.8-Setup-x64.exe`. Its exact hash and binary identities are in [validation/brotli-release-20261008.json](validation/brotli-release-20261008.json). It has not been installed or published: the existing UDM process is still running and was left intact. Public-site compressed media, the initial Edge timeout, clean-machine installer lifecycle, and full IDM parity remain unverified or incomplete.

## Edge preparation timing follow-up

The indexed-DASH live fixture now supports opt-in `UDM_TEST_PREPARE_DIAGNOSTICS=1`. Its isolated extension worker records operation names, durations and completion state for tab lookup, settings/session storage, player inspection, permission checks, fetch headers and body reads. It records no arguments, URLs, cookies or credentials. At failure or completion, timings are saved with the fixture results; the normal fixture behavior remains unchanged when the flag is absent.

A fresh three-cycle Edge run against the same Brotli package passed all 26 checks and removed its temporary host registration. There were no pending measured operations; the maximum observed duration was about 214 ms for a tab lookup, with index body reads at most 168 ms. The 28 Chromium/Firefox deadline tests also passed, including protection against late native admission after a timeout. The original 20-second preparation stall did not recur. Its cause remains unestablished; production deadlines were not changed. Portable evidence: [validation/edge-preparation-timing-20261008.json](validation/edge-preparation-timing-20261008.json); raw evidence: `candidates/brotli-release-20261008/edge-timing`.

The installed UDM process remains open. The user has been asked to exit it so the already validated package can be deployed without replacing files in use.
