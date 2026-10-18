# IDM size reporting research

Research date: 2026-10-18 UTC. This is evidence of specific paths and tests, not a complete reconstruction of IDM.

## Runtime findings

IDM fetched the complete synthetic 16,777,339-byte HTTP fixture while Download File Info remained open, before Start Download was clicked. The dialog showed 16.00 MB. Start produced immediate completion, with no subsequent HTTP requests. The saved file matched the expected payload byte for byte. This demonstrates preconfirmation transfer and a size available from Content-Length without prediction.

See [results](validation/idm-http-prefetch-20261018.json) and [server events](validation/idm-http-prefetch-20261018-events.jsonl). Events contain only synthetic offsets, counts and timestamps. Cumulative sent events must not be summed. The result uses completed request ranges and their union; overlapping transfers explain the excess transferred bytes. Dialog latency was not instrumented.

The earlier [YouTube 360p observation](validation/idm-360-size-comparison-20261017.json) initially showed 5.89 MB, matching the video URL length of 6,179,573 bytes. Completion reported 12,091,913 bytes, whereas the saved MP4 contained 12,031,058 bytes. This initial display was not the final assembled size. One observation does not establish universal behavior.

## Static evidence

Installed IDMNetMon64.dll SHA-256: `9840d78e94f9df6b03be2246b9814263a5ddf287489cd260a25a4d51e4a1bc5b`.

Existing REA results and the matching historical Ghidra export identify:

- `0x180055300`: parses the URL parameter `clen` as an integer.
- `0x180055ab0`: parses `contentLength`, including `transcodeMetadata`, and passes media metadata to record construction.
- `0x180056950`: stores its thirteenth parameter unchanged at media-record offset `0x48`. This does not by itself prove the later UI meaning of that field.
- `0x18002e9b0`: recognizes HTTP Content-Length and Content-Range. Its complete downstream numeric/UI binding remains untraced.

These paths support source byte lengths, not a universal calculation of final muxed size. Decompiled types and parameters are inferred; full assembly/data-flow corroboration remains outstanding.

### Follow-up: metadata handoff to the desktop

The hash-matched export provides a longer chain than the initial string search:

1. In `0x180055ab0`, the parsed `local_120` content length is assigned to `uVar8`, which is passed as argument 13 to `0x180056950`.
2. The constructor stores that argument in the media record at offset `0x48`. Assembly at `0x180056d37` loads a stack value into RAX; `0x180056d3e` stores RAX at `[R12 + 0x48]`, corroborating the unchanged store. Full calling-convention tracing remains outstanding.
3. Network-component function `0x180029790` reads offset `0x48` and serializes nonzero lengths as `len` and `alen` alongside the audio URL metadata. These are separate lengths, not a computed final mux size.
4. Desktop function `0x0048a820` parses those fields. It converts each through `0x0063dd3c` and stores the result at offset `0x110` on the corresponding object path. The helper delegates to `__strtoi64` with base 10. Assembly at `0x0063dd41` confirms the radix argument `0xa`.

The desktop export is pinned to IDMan.exe SHA-256 `03cc62e9adb77a380f9dc12f67ccaaee5106f12844aa73ce32c914ddd16d607c`. These findings do not prove which value every dialog displays.

Correction to the strength of the `clen` finding: `0x180055300` parses it into a local value, but its visible call to `0x180056950` passes zero for the size argument. Therefore that parser alone must not be cited as proof that URL `clen` reaches the media-record size field. The JSON contentLength path above provides stronger evidence.

### Follow-up: the Download File Info size control

The existing [PE resource analysis](reference/pe-analysis.json) identifies dialog 130 as Download File Info and control 1101 (`0x44d`) as its centered static label at dialog coordinates (311,57), dimensions 64 by 10.

Desktop routine `0x0043fb40` reads the object referenced at dialog offset `0x88`. One branch takes the 64-bit value at object offsets `0x110/0x114`, calls formatter `0x0043e6b0`, then writes control `0x44d` through `0x00423790`. That wrapper ends in `SetDlgItemTextW`. Assembly at `0x00440178` through `0x0044019b` corroborates the field loads, formatter call, control ID, and text-wrapper call. This connects the previously identified size field layout to the actual dialog label; object identity across the complete handoff has not been dynamically traced.

Another branch uses the value at `0x518/0x51c`; assembly at `0x004400c2` through `0x004400e5` corroborates its display path. Its producers and meaning remain unresolved, so it must not be labeled final output size based on the offset alone.

Formatter `0x0043e6b0` uses binary unit divisors (1024, 1048576, 1073741824) and a locale decimal separator, with two fractional digits for scaled sizes. It formats a supplied byte count; no container-size calculation is present in that formatter.

Routine `0x00441570`, when handling value `0x14a5`, reads the same `0x110/0x114` pair and updates control `0x44d`. This establishes an update path, but does not by itself establish when every producer sends that message.

A sender is present in `0x005baf10`: it posts `WM_COMMAND` (`0x111`) with command `0x14a5` to available window handles after checking a nonzero 64-bit field at offsets `0xf0/0xf4` of a referenced object. This corroborates asynchronous notification rather than requiring a dialog reopen. The sender's complete input provenance remains to be traced.

The initialization routine also has a duration branch using `0x0043e890`. Its divisions by 60 and 3600 distinguish time formatting from size formatting. Do not misinterpret that branch as a byte-size prediction.

### Follow-up: indexed stream sizes

The alternate `0x518/0x51c` field has more than one producer. `0x005cc1c0` copies it from a transfer object at `0x1c0/0x1c4`; `0x004a6cd0` can also populate it from command `0x1473` arguments. This does not identify it as final muxed size. Offset-only searches also matched unrelated settings/UI objects and those matches are excluded.

A separate, identifiable size-update chain reads an MP4 segment index:

- `0x005d0ad0` opens a local file and calls `0x005d0a00`, which reads boxes through `0x005d0770`.
- `0x005d0770` recognizes `0x78646973` (the little-endian representation of the bytes `sidx`) and calls `0x005d0600`.
- `0x005d0600` handles index offsets, validates the reference-table extent, rejects the high reference-type bit, and sums the lower 31-bit reference sizes. The outer reader adds the index-end position and first offset to form a referenced stream extent.
- `0x0055be70` uses the parsed ranges to build a segment list and stores the size in its list object at `0x38/0x3c`.
- Callers `0x00599460`, `0x00599aa0` and `0x00599e40` have conditional paths that copy this size into an expected-size field and call notifier `0x005baf10`. The notifier also persists the field under the name `nExpectedSize`.

This supports obtaining a stream extent from index metadata without reading every media segment. It is not evidence of calculating a new audio/video container's output size. Calling-convention types remain inferred from the decompiler; these paths have not been instrumented in a live YouTube run.

UDM already implements SIDX range resolution in `browser/chromium/media.js` and its Firefox counterpart. The current `node tests/dash-index.test.cjs` run passed all 40 checks, including version 0/1 offsets, malformed indexes, selected-track-only range retrieval, handoff, navigation cancellation and browser implementation parity. This is parser/handoff verification, not a new live-browser size comparison. No duplicate index parser was added.

Additional current runs passed all 42 checks in `tests/dash-external.test.cjs` and all 51 in `tests/dash-external-tree.test.cjs` (133 checks across the three suites). These cover external indexes and hierarchical range resolution; they do not establish support for every YouTube transport or final mux-size calculation.

A fresh REA open_binary call identified the installed binary, but analyze_function timed out after 240 seconds. No new decompilation result is claimed. Searches in IDMan.exe also found CRT `__mbclen` and outgoing request Content-Length construction; neither is evidence of incoming media sizing.

## Implications for UDM

The current exact-final-size path prefetches tracks, assembles a staging file and measures it before confirmation. This is measurement, not prediction. Earlier source lengths are useful, but their sum does not establish final container length.

Current UI inspection confirms the sample-metadata mux calculation is manual: the media dialog does not call `refreshLookup` automatically. Its automatic path starts prefetch and displays available sizes. Thus an initial-dialog delay cannot be attributed to that manual calculation without runtime evidence.

A subsequent source fix addresses failure latency in the manual calculation: paired metadata collectors now cancel one another on failure, join both workers before cleanup, and preserve the first exception without cancelling the caller. Previously an audio failure could wait for the video future's full deadline; a video failure could remain unobserved while audio continued. The native build and all 47 media-size regression checks passed; see [results](validation/media-preview-cancel-20261018.json). The cancellation fix was installed and its executable hash verified; the prior executable was backed up. A normal desktop restart reports 25 jobs (eight paused, 17 complete), matching PowerShell/.NET. Python and its child processes expose a separate six-job view of the same nominal catalog path, which was used during the preceding live tests. Neither catalog view was overwritten or migrated; filesystem-view isolation remains unresolved.

Open the dialog promptly, show authoritative source sizes with an accurate label when available, and prefetch independently. Label an output size exact only after measurement or a verified container-size calculation. Do not block dialog presentation on assembly.

Prior [UDM 0.63.16 live validation](validation/concurrent-capture-06316-20261017.json) matched preconfirmation and saved bytes at 360p and 1080p. That is not proof of equivalence across every format. No production sizing change was made during this research pass.

## Remaining investigation

Trace producers of the alternate `0x518` field and runtime object identity through the handoff. Identify the senders of the size-update message. Repeat IDM observations for combined/separate streams, 1080p, AV1 and audio-only: first size, subsequent updates, selected tracks, final bytes and timing. Chrome recovered for fresh UDM AV1 and M4A comparisons. Both showed an exact preconfirmation size matching the saved file, and both passed full decoding; see the linked validation records below. Fresh matching IDM quality comparisons remain outstanding; the native HTTP test is not a substitute for that matrix.

## Primary sources

[IDM options](https://www.internetdownloadmanager.com/support/options.html) documents preconfirmation downloading and its disable option. [Starting downloads](https://www.internetdownloadmanager.com/support/using_idm/starting.html) documents the dialog workflow. These pages do not establish a universal exact-final-remux-size guarantee.

## Completed UDM follow-up

The installed 0.63.16 extension and desktop SHA-256 `5d9376d3d517bc767262644075a6f91c0cf4928f9dbca097f1cbd5496d6729c9` were tested through the actual Chrome video menu. The 1440p AV1/AAC output matched 54,867,860 preconfirmation bytes; M4A matched 5,908,172 bytes. Both destinations were absent before confirmation, and recorded network counters did not increase after Start. Full decoding passed with exit 0 and empty error logs. The AV1 first decode attempt timed out; the subsequent complete run passed. See [AV1 evidence](validation/av1-live-20261018.json) and [M4A evidence](validation/audio-live-20261018.json). These observations do not establish universal format support or consistently fast handoff.

At the initial research checkpoint, confirmation discarded staging and remuxed retained tracks. Subsequent installed fixes now publish the verified staging file directly when its folder is unchanged (renaming is supported), and continue in-flight prefetch when confirmation values are unchanged. The installed desktop SHA-256 is `C7E9B285CAD8DEA3D76271550164B75C9898D9D447C3E3FA893461ABC6494D8A`; 12 HTTP/confirmation and 59 media checks passed. Installation and preservation of the normal 25-job catalog are verified in [the follow-up record](validation/early-prefetch-confirmation-20261018.json). Live browser verification of this build and staging reuse across destination-folder changes remain outstanding.

### Catalog-view investigation

The Python executable resolves through the Microsoft Store Python 3.13 package. Python's official [Windows documentation](https://docs.python.org/3.13/using/windows.html#redirection-of-local-data-registry-and-temporary-paths) documents private copies of AppData and temporary paths. That is a plausible explanation for the observed process-dependent catalog views, but the physical backing path has not been established: os.path.realpath and GetFinalPathNameByHandleW returned the nominal path. A PowerShell child launched by Python also reported six records, whereas an independent PowerShell process and the normally launched UDM reported 25. Treat this as an environment issue under investigation, not a confirmed UDM history-loss defect. Future live evidence must match the process's diagnostic count to the reader's count before using catalog values.

### Alternate size update follow-up

The same hash-matched desktop export provides a sender for command `0x1473`: `0x005bbf60` stores its supplied low/high count at transfer-object offsets `0x1c0/0x1c4`, sends `WM_COMMAND` (`0x111`) with command `0x1473` and a pointer to that count, and persists eight bytes under registry value `FileSize`. Disassembly places the command push at `0x005bc01f`. The receiver at `0x004a6cd0` writes that count to `0x518/0x51c` except for its `0x368 == 0x20` branch. Function `0x004a6700` also uses the transfer object's `0x1c0/0x1c4` count, falls back to `0x110/0x114`, and compares it with the adjacent transferred count to update progress. These are additional static data-flow observations, not proof that the supplied count equals a final merged container. The originating producers and live quality matrix still require investigation.

The subsequent media-publication fix is installed as desktop SHA-256 `9904DD92B75AACF33795F5E3288225DC716403EBCC2386E7107FA06510AF2D76`. It uses the existing publication journal for verified media and preserves recovery evidence if the completion save fails. All 63 media and 12 HTTP/prefetch checks passed; see [validation](validation/media-publication-recovery-20261018.json).

### HTTP response size producers

A further trace of the same installed IDMan.exe (SHA-256 rechecked as `03cc62e9adb77a380f9dc12f67ccaaee5106f12844aa73ce32c914ddd16d607c`) identifies concrete producers for the alternate size path:

- `0x00581a80` and `0x0058c650` recognize `content-length:` case-insensitively and parse a signed 64-bit decimal into response-object offsets `0x3220/0x3224`. Negative counts are cleared. The latter parser has a separate branch using `0x3288/0x328c`; these fields must not be conflated.
- `0x0058c650` recognizes `Content-Range:` and parses the decimal after `/` into `0x3290/0x3294`. It parses the range end separately into `0x3298/0x329c`.
- Within `0x00584ab0`, the HTTP 200 branch copies `0x3220/0x3224` to `0x58/0x5c`. Disassembly `0x0058563c` through `0x00585659` confirms the status comparison and the copies. The HTTP 206 branch selects its count using the response-length and range-total fields when the existing count is absent; this is conditional behavior, not a universal assignment.
- After cleanup, disassembly `0x005856e6` through `0x00585709` loads `0x58/0x5c`, checks that the count is positive, and passes it to `0x005bbf60`, with the transfer object from `0x3208`. This connects the parser fields to the previously traced `FileSize`/`0x1473` notification routine. The intervening `0x00582e40` export was inspected: it performs connection/decoder cleanup and contains no explicit write to `0x58/0x5c`; runtime tracing would be needed to rule out every indirect side effect.

This is stronger evidence for server-reported HTTP sizes reaching IDM's display-update machinery. It does not establish a formula for a final multiplexed MP4/M4A. The static corpus also contains callers that set the count from transferred-byte fields at completion, and FTP-related callers; field offsets alone cannot identify a shared algorithm across those object types.
