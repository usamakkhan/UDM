# IDM capture rules (R02) and file-type recognition (R01): recovered semantics and UDM plan

Date: 2026-10-16. Reference: IDM 6.43 build 10 (installed copy, identical to `IDM FIles/`).
Method: REA 6.5.0 (Ghidra 12.1.3 provider) for live, evidence-pinned decompilation of
`idmftype.dll` and `IDMNetMon.dll`; the existing full Ghidra export in
`benchmarks/idm-complete-20260927/ghidra/` (same binaries, same Ghidra version) for
`IDMan.exe`, whose first-pass analysis exceeds REA's 330 s startup limit. Static only:
no IDM binary was executed. This continues
`docs/idm-research-current-status-2026-09-29.md` (gaps R01/R02).

| Artifact | SHA-256 (prefix) |
|---|---|
| IDMan.exe | `03cc62e9adb77a38` |
| idmftype.dll | `3a47dbb1f86f2c51` |
| IDMNetMon.dll | `6829aea180f12c29` |
| idmfc.dat (rules) | `05def0184416d4a1` |
| IDMFType.dat (signature DB) | `a21f4d385269a38e` |
| IDM browser bundle background.js | `3590408894c70193` |

REA evidence IDs: `ev_8b8eff71af0a…` (IDMFileType dossier), `ev_1ac9eda3ffd5…`
(FUN_10001a30 classifier core), `ev_fd5ca3bbacf7…` / `ev_e7c8ad0bc58b…` /
`ev_b7f49bde2615…` (IDMNetMon idmfc string/xrefs).

Licensing boundary: UDM must not ship IDM's rule list, signature database, or code.
The facts below describe *behaviour*; the plan reimplements it with UDM-owned data and
permissively licensed upstream components.

---

## 1. File-type recognition (R01)

### 1.1 Observed

**IDMan side** — `FUN_005da140` (body sniff, runs on the partially downloaded temp file)
and `FUN_005d58a0` (same logic, used by the media/merge path at `FUN_005c2e40`):

1. Load `idmftype.dll` exports `IDMFileType` and `IDMExtensionForMimeType`
   (`FUN_005d2dd0`) and `urlmon!FindMimeFromData` (`FUN_005d2eb0`).
2. Read the first 256 bytes; call `FindMimeFromData`.
   - If the result is anything other than `application/octet-stream`, `text/plain`, or
     `text/html`, use it.
   - Special case (asm `005da31c–005da38d`): when the result is octet-stream, ≥7 bytes,
     bytes `F L V`, version byte 1–2, and flag bits 0x80/0x40/0x20/0x10/0x02 clear →
     `video/x-flv`.
   - Otherwise fall through to `IDMFileType`.
3. Read up to **5000 bytes** (10000 in `FUN_005d58a0`) and call
   `IDMFileType(buf, len, 0, out)`, where `out` is a 0x2a0-byte struct with `out[0]=1`
   (struct version):
   - `+0x001` description (200)
   - `+0x0c9` MIME (100)
   - `+0x12d` extension (25)
   - `+0x146` secondary text
   - `+0x160` u64 prefix length
4. Return codes:
   - `1` = matched at offset 0.
   - `2` = matched **after a text/HTML prefix** (see 1.2).
   - `3` = undetermined, more bytes needed.
   - `0` = no match.
   - `-1` = database not loaded.

   Code 3 is accepted only if ≥5000 bytes were available or the whole file is that size.
   Otherwise the decision is deferred (`local_3be`) and retried as more data arrives.
5. Result → download's content-type field (`+0xcc`):
   - The detected extension is `txt` → `text/plain`; `htm`/`html` → `text/html`.
   - Any other extension → `idmcheckedtype/<ext>`. If
     `IDMExtensionForMimeType(detectedMime, currentExt)` returns 1, the *current*
     filename extension is consistent and is kept instead.
   - There is no extension but a specific MIME (not octet/plain/html) → that MIME.
   - Return code 2, with download flag `0x100` set → persist `exLenToSkip` = prefix
     length (registry, per download). The junk prefix is stripped from the saved file.
6. **Error-page verdict** (`*param_3`) is true when either:
   - the sniffed type is `text/html`; or
   - the expected filename extension is in the fixed binary list and the sniffed type is
     `text/plain`. The list: `3GP 7Z AAC ACE AIF APK ARJ ASF AVI BIN BZ2 DOC DOCX EXE FLV
     GZ GZIP IMG ISO LZH M4A M4V MKV MOV MP3 MP4 MPA MPE MPEG MPG MSI MSU OGG OGV PDF PLJ
     PPS PPT QT R0* R1* RA RAR RM RMVB SEA SIT SITX TAR TIF TIFF WAV WEBM WMA WMV Z ZIP`.

   This applies when the download is browser-originated (`+0x3c`) or flag `+0x164 & 8`
   is set. This is IDM's "server sent a web page instead of the file" detection.

**idmftype.dll side** — `IDMFileType` @`0x100047e0` → core `FUN_10001a30`:

- It is a **libmagic (`file(1)`) engine**. It has the "Empty file" /
  `application/x-empty` and "Very short file" cases, a soft-magic matcher, and a text
  fallback (`text/plain`) or `application/octet-stream`.
- Matcher `FUN_10001ef0` walks the 3,224 × 38-byte records in `IDMFType.dat` block `f5`:
  - byte 0 = continuation level.
  - byte 1 = flags: `0x02` offset-add (relative to the parent match), `0x10` no-space
    join, `0x20` text-test vs binary-test pass, `0x80` suppress description.
  - word +0x20 = description string index into block `f2`.

  These are libmagic's `struct magic` semantics in a compacted IDM layout. Descriptions
  (`" v4 (H.263/AMR GSM 6.10)"`, `"%.4s"`, …) are libmagic's.
- Prefix detection: `FUN_10005730` (a `<`+letter start, up to 2000 bytes of text) and
  `FUN_100057d0` (≥31 bytes, a base64/printable run up to 100 bytes) return the offset
  where text ends. Binary magic is retried at that offset; a hit is return code 2.
- For `audio/*` results it extracts ID3-style metadata (title, artist, album, track,
  length, bitrate) into the extended struct (`out[0]=2`).

### 1.2 Inferred / unknown

- The 38-byte record layout beyond the fields above (type enum values 0x1d–0x1f, value
  field) is not fully mapped. UDM does not need it (see plan).
- What sets download flag `0x100` (prefix stripping) is unknown. It is probably an
  option or per-site behaviour; strip only behind an explicit UDM setting.

---

## 2. Capture rules (R02)

### 2.1 Observed

- **Data:** `idmfc.dat` holds 81 active lines in a stream-cipher container (decoder:
  `tools/idm-research/decode-capture-config.py`). Header
  `FORMAT=3;VERSION=26021801;LINES=81`.
- **Distribution:**
  - IDMNetMon.dll (`FUN_10016c60`) parses the header (` FORMAT=%hu`, ` LINES=%u`) and
    serves the rules to the browser on an `idmfc` message (`FUN_10026fd0`). It also
    sends them to the WFP driver under tag 0x12 via IOCTL `0x12c008`.
  - IDMan keeps two copies, promoting the one with the higher VERSION (`FUN_0046e6d0`).
    The "report capture problem" flow (`FUN_00485350`) posts diagnostics to
    internetdownloadmanager.com (`report_to_force_capture642b3.html`). It may receive a
    newer `idmfc.dat`, and stamps `HKCU\Software\DownloadManager\ConfigTime`.
- **Evaluation is in the browser extension.** Compiler `yc` and evaluator `Gc` are in
  `background.js`.

**Line grammar:** `[FLAGS][#EXT]!PATTERN`, or a bare `PATTERN`. FLAGS ⊂
`GPTSDVACRHX`. All lines are joined with `|` into one anchored regex over the full
URL `href`.

| Pattern token | Meaning |
|---|---|
| `*` | lazy `.*?` |
| `*` in the scheme (`http*://`) | `\w*` (http or https) |
| `~` right after the host start | optional one-label subdomain |
| trailing `?` | optional query |
| `<F:*>` | capture group `[^/\?#&;]*` → filename |
| `<T:*>` | capture group `[^/\?#&;.]*` → extension |
| regex metachars `.?+(){}[]` | literal |

**Evaluation** (first matching alternative only):

1. `G`/`P` present → the method must be GET/POST respectively. On mismatch the rule
   does not apply; normal capture logic continues.
2. `H` → mark the request as hidden (`b.Y`).
3. **Ignore (result 1)** when:
   - it is a stream rule (`V`/`A`/`C`) and the request is hidden; or
   - it is not a stream rule, the request is a *background* resource
     (XHR/media/object/subframe) and `S` is absent; or
   - it is not a stream rule, the request is a foreground navigation and `X` is
     present; or
   - `D` is present and the request lacks the secure/proxy context bit (`b.J`).
4. Extension E is resolved in this order:
   1. the `<T:>` capture;
   2. the extension of the `<F:>` capture;
   3. with the `T` flag, the URL path extension;
   4. the literal `#EXT`.
5. Stream rule (`V`/`A`/`C`) → record E, mark processed, continue (result 6; no
   immediate capture).
6. Otherwise, if not yet processed: when E exists and is in the user's capture-type
   list, **or** there is no E and the pattern had no capture group → **capture (result
   4)**, with reason flag `0x1000000`.
7. `R` is accepted by the compiler but has no effect in this bundle.

Flag usage in the shipped list: G 36, T 16, S 14, V 14, P 10, D 5, H 2.

### 2.2 Unknown

- The driver-side (`idmwfp*.sys`) evaluator was not traced. It belongs to R08.
- The meaning of `b.J` for `D` is only partly established: the secure-scheme bit or the
  HTTPS-proxy state.

---

## 3. Where UDM stands (source as of d8f090c)

- `browser/*/file-recognition.js` classifies from metadata only: browser filename,
  Content-Disposition, URL path, and Content-Type via mime-db 1.54.0.
  - It already rejects web types that conflict with a strong extension
    (`conflicting-web-response`).
  - It has no content sniffing and no site rules.
- `background.js:337` captures when `UdmFileRecognition.allowed(recognition, extensions)`
  passes. This is post-`downloads.onCreated`, not `webRequest`, so URLs the browser
  shows as `download.php` with no usable extension are missed. That is exactly the
  class IDM's rules exist for.
- The native transfer path does not inspect the first bytes. An HTML login or error
  page served instead of a file completes as a "successful" download. The exception is
  the player probe path (`PlayerChecks.hpp:64`).

---

## 4. Implementation plan

### Phase A — native content sniffing (R01), highest user value

1. **`native/ContentSniff.hpp/.cpp`**: `SniffResult sniff(span<const uint8_t>, bool complete)`.
   - Stage 1: `FindMimeFromData` (urlmon, a system API) on ≤256 bytes, plus the FLV
     header rule.
   - Stage 2, if stage 1 is generic: **upstream libmagic** (BSD-2-Clause; vcpkg
     `libmagic`) on ≤5000 bytes, with `MAGIC_MIME_TYPE` and `MAGIC_EXTENSION`.
     Compile the upstream `magic.mgc` into the installer.
     - Do not use IDMFType.dat.
     - Bound libmagic with `MAGIC_PARAM_*` limits and a byte cap. Run it in-process on
       an untrusted buffer only after fuzzing (see tests).
   - Stage 3: text-prefix detection (an HTML or printable run ≤2000 bytes, then binary
     magic at that offset) → `PrefixLength`.
   - Return `{mime, extensions, verdict: Match|MatchAfterPrefix|NeedMore|None, prefixLength}`.
     Report `NeedMore` when fewer than 5000 bytes are available and the download is
     not complete.
2. **Hook** in `Transfer.cpp`: once the first segment has ≥5000 bytes (or at EOF),
   sniff once per download and persist `SniffedType`/`SniffedExt` in job data.
3. **Error-page guard:**
   - Trigger: the sniff says `text/html`; or the expected extension is in a binary set
     (UDM-owned list, seeded from the category extensions) and the sniff says
     `text/plain`.
   - Response: pause the download with status "Server returned a web page", and offer
     **Refresh link** (existing `DirectMediaRefresh`/link-refresh flow), **Open page in
     browser**, or **Save anyway**.
   - Default on; option in Options › Downloads.
4. **Extension correction:**
   - Applies when the target name has no extension, a weak one (`bin`, `dat`, `tmp`,
     `download`), or an endpoint one (`php`, `asp`, …), and the sniffed extension is
     specific.
   - Rename before publication through the existing protected-publication path
     (`Duplicates.cpp`). Keep the current extension when the sniffed MIME lists it.
     This mirrors `IDMExtensionForMimeType`.
5. **Prefix stripping**: off by default; per-download opt-in only. Record
   `PrefixLength` and show it in File Info.
6. **Category re-evaluation** after correction, via the existing `downloadCategory()`.
7. **Tests**:
   - `native/ContentSniffChecks.hpp`: fixture bytes for zip/7z/rar/pdf/mp4/webm/mkv/
     exe/msi/iso/mp3(ID3)/flv/html/plain/empty/1-byte, HTML-prefixed zip, NeedMore at
     4999 bytes, and the rename and error-page decisions.
   - Add a libFuzzer or corpus run over libmagic in `benchmarks/`.

### Phase B — capture rule engine (R02)

1. **UDM-owned rule file** `capture-rules.txt`:
   - Same line grammar as §2.1, so the syntax and evaluator are a faithful
     reimplementation. Header `UDMRULES=1;VERSION=<yyyymmddNN>`.
   - Rules are written by UDM from public, observable site behaviour. Do not copy
     IDM's 81 lines; use them only as private regression references in
     `benchmarks/` (already git-ignored research output).
   - Ship with the app. User additions go in Options › Capture › Site rules
     (`ServerRulesUi` can host the editor).
2. **Native**:
   - `CaptureRules.hpp` parses, validates and versions the file.
   - `Bridge.cpp` gains action `capture-rules` → `{version, lines}`. The reply must fit
     the 256 KiB limit, so cap at 2,000 lines.
   - Updates arrive only with UDM releases or the user's file. No phone-home.
3. **Extension**:
   - New `capture-rules.js` (mirrored to firefox/) compiles lines into a regex using
     the token table in §2.1, and keeps per-alternative flags, `#EXT`, and capture
     indices.
   - Use explicit index bookkeeping instead of `RegExp.leftContext`. Reject patterns
     longer than 2 KB and catastrophic constructs, so no ReDoS on user rules.
4. **Evaluation points**:
   - **`downloads.onCreated`** (existing path, before `allowed()`): a rule match yields
     E and source `'site-rule'`.
     - With E: E is tested against the user's extension list.
     - With no capture group: capture outright, as IDM does.
     - G/P use the request method from `request-context.js`. `D` uses `https:`.
   - **`webRequest.onBeforeRequest`/`onHeadersReceived`** (optional, behind the
     existing capture permission): needed for `S` (background/XHR) rules and for
     rules that must win before the browser opens a page. Results 1/4/6 map to
     ignore / hand off to UDM / continue.
   - **`V`/`A`/`C` stream rules**: tag the request for the media panel (`media.js`)
     rather than capturing.
5. **Diagnostics instead of server reporting**: "Report capture problem" writes a local
   redacted bundle (URL host and path shape, flags, matched rule, recognition source).
   The user can attach it to a GitHub issue. Nothing is uploaded.
6. **Tests**:
   - `tests/capture-rules.test.cjs` covers compiler token cases, every flag, the
     E-resolution order, the G/P mismatch fall-through, and ReDoS limits.
   - A golden table of synthetic URLs per flag combination; the same table is reused
     by the native parser tests.

### Phase C — later

- Feed the compiled rules to the signed-network/WFP observation path when R08 lands.
- Map the IDMFType record type enum only if a concrete libmagic divergence is found.

### Phase A status (implemented)

- A1 is in `native/ContentSniff.hpp`, header-only, as pure functions. It differs from
  the plan above in two ways:
  - It does not call `FindMimeFromData`. Its result depends on per-machine
    IE/registry state, which would make decisions nondeterministic and untestable.
  - It does not link libmagic. Stage 2 is a UDM-owned signature table covering
    archives, installers, documents, media and images, extended on 2026-10-17 with
    the long-tail formats listed in the backend gap document (disk images, fonts,
    e-books, raw camera and texture images, lossless and tracker audio, broadcast
    video, data and capture files, further ZIP producers). Compound documents are
    resolved to MSI/MSP, Word, Excel, PowerPoint, Outlook or Visio from the root
    entry's class identifier when its directory sector lies within the sniffed
    bytes, or from well-known stream names. Containers that stay ambiguous (plain
    compound files, ELF, Mach-O, ID3/MPEG audio, ASF, MPEG elementary streams,
    registry hives, DER) and two-byte magics (PNM) are recognized but marked
    non-specific, so they never rename a file.

  `SniffResult` is the seam where libmagic can be added later for long-tail formats.
- A2: `checkDownloadedContent` in `Transfer.cpp` reads up to 0x8806 leading bytes from
  the contiguous parts after the network phase and before assembly. It records
  `ContentCheck`, `SniffedType`, `SniffedExtension` and `ContentPrefixBytes`. Grabber
  jobs (`ProjectId`/`GrabberImmediate`) are skipped.
- A3 is implemented as follows:
  - **Trigger:** HTML under a non-web extension, or text under a binary-only extension.
  - **Result:** the download fails with `WebPageReceived` and the parts are kept.
    `queueFailureCanRetry`/`queueFailureExhausted` treat it like a pending
    authentication prompt.
  - **Controls:** the global option `StopOnWebPage` (Options › File Types) and the
    per-download override `AllowWebPageContent`, which has no UI yet.
- A4: `CorrectFileExtensions` (default on) renames only names without a usable
  extension, or with a weak or script extension, to the specific content extension.
  - Skipped when the new name is occupied on disk or by another download, and for
    replacements.
  - The previous name is kept in `OriginalFileName`.
- A7: `native/ContentSniffChecks.hpp` (`Udm.NativeTests.exe --content-sniff-checks`;
  also part of the full suite).
- A6: after a correction, a category that still matched the old name is recomputed
  with `downloadCategory()`; a category the user changed is kept.
- Per-download override: resuming a download marked `WebPageReceived` asks whether
  to save the received content anyway (sets `AllowWebPageContent`) or retry.
- Early detection: the worker that writes the first segment sniffs its first
  5,000 bytes (or the whole segment when shorter) and stops with `WebPageReceived`
  before the rest of the body is downloaded.
- A5 (prefix stripping): when the content check finds a file behind inserted web
  page text (`MatchAfterPrefix`), the download stops with `ContentPrefixPending`
  and its parts kept. Resuming asks whether to remove the text
  (`StripContentPrefix`: assembly skips `ContentPrefixBytes`, the result is
  re-checked to start with a recognized signature, and `StrippedPrefixBytes` is
  recorded) or keep the data as received (`AllowContentPrefix`). Queues do not
  retry while the choice is pending.

### Phase B status (implemented)

- B1: rules live in the `CaptureRules` setting (Options › File Types › Site rules…),
  one line each, same grammar as §2.1. The default is empty: UDM ships no rule list
  until individual rules have been verified against the sites they describe.
- B2: `native/CaptureRules.hpp` parses and validates (flags ⊂ `GPTSDVACRHX`, scheme
  required, only `<F:>`/`<T:>` captures, ≤2,000 lines of ≤2,048 characters).
  `browserPreferences` serves the active lines as `captureRules`; the extension reads
  them from its cached `desktopPolicy`, so no new bridge action was needed.
- B3: `browser/*/capture-rules.js` compiles each line to its own anchored regex
  (token table from §2.1) and evaluates with the §2.1 order: method flags →
  `H` → ignore conditions → `<T:>` / `<F:>` extension / `T` path extension / `#EXT`
  → stream / capture / continue. Patterns produce only `.*?`, character classes,
  `\w*` and optional groups; addresses over 8 KiB are not evaluated.
- B4: evaluated in `downloads.onCreated` before the file-type list. A rule with an
  extension replaces the recognized extensions and still passes through the
  local and desktop type lists; a rule without one captures outright. Stream (`V`/`A`/`C`)
  results are reported but not yet routed to the media panel, and the
  `webRequest` evaluation point (needed for `S` background rules to act early) is
  not implemented, so `background` is always false at this point.
- B6: `tests/capture-rules.test.cjs` (compiler, every flag, resolution order, limits,
  and worker harness cases for both families) and `Udm.NativeTests.exe
  --capture-rules-checks`.
- Response-level evaluation (follow-up): `onHeadersReceived` evaluates rules for
  every non-navigation reply with `background: true`. A `V`/`A`/`C` match places
  the reply on the video panel (`media:<tab>`) with a media type derived from the
  rule's extension; an `S` match hands the request off to UDM through the
  ordinary `handoff` path, once per tab and address for five minutes. The page's
  own request keeps running, because cancelling needs a blocking `webRequest`
  permission the extension does not hold.
- B5 (diagnostics): the extension keeps the last 50 rule decisions (host, path
  with digits replaced, query parameter names, resource type, action, extension,
  rule) and the diagnostics page shows them and saves the whole report as a file.
- Not done: a bundled verified rule list.

### Order and size

A1–A4 + A7 first (one PR). Then B1–B4 + B6 (one PR). Then A5/A6 and B5.
Phase A touches `Transfer.cpp`, `Duplicates.cpp`, the options UI, the build
(vcpkg libmagic, plus `magic.mgc` in the installer) and NOTICE (libmagic BSD-2).
Phase B touches `browser/*`, `Bridge.cpp`, and the options UI.
