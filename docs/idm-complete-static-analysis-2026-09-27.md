# IDM: expanded static analysis and setup reconstruction

This investigation covers the supplied IDM directory, the installed program directory, the supplied build 11 setup, its complete extracted payload, nested browser/help packages, a legacy browser support directory, and the installed WFP driver. It extends the narrower [September 20 investigation](reverse-engineering-2026-09-20.md).

**The file inventory is exhaustive for those roots. The program is not completely understood.** Decompilation produces inferred pseudocode, not original source. Function discovery, decompiler errors, indirect calls, encoded data, and unobserved runtime branches prevent a defensible claim that every behavior or "secret" has been recovered. The user selected static analysis because no disposable VM is available; no installer, native component, or driver was executed during this investigation.

## Coverage and evidence

<!-- COVERAGE_START -->
| Coverage measure | Result |
| --- | ---: |
| Inventoried file entries / unique contents | 988 / 465 |
| Native binaries with full recognized-function export | 59 / 59 |
| Native binaries with assembly/byte audit | 59 / 59 |
| Independent DUMPBIN disassemblies / failures | 59 / 0 |
| Recognized functions, including thunks | 90,728 |
| Nonexternal, non-thunk functions attempted | 89,013 |
| Pseudocode recovered, including longer retries | 88,975 |
| Remaining pseudocode failures; assembly retained | 38 |
| Automatic-analysis timeouts | 0 |
| Embedded PE resources, all hashes rechecked | 1,321 |
| Snapshots / original file occurrences rehashed | 465 / 592 |
| Changed originals / inventory errors / validation errors | 0 / 0 / 0 |

These are static coverage counts, not percentages of fully understood behavior. Function totals include compiler/runtime and third-party code, plus variants across builds and architectures; they are not counts of distinct application algorithms. The installed IDMan and 32-bit monitor analysis databases were reused read-only after checking their SHA-256 values, then exported without the earlier function limits. Other project provenance and [per-function failures](../benchmarks/idm-complete-20260927/decompilation-failures.json) are recorded locally. See the [per-binary coverage table](idm-native-coverage-2026-09-27.md).
<!-- COVERAGE_END -->

The main evidence directory is `D:\UDM\benchmarks\idm-complete-20260927`. The setup-specific evidence is `D:\UDM\benchmarks\idm-setup-build11`. These ignored directories contain proprietary inputs and generated analysis; they are kept separate from UDM implementation code.

- [Every file occurrence, its hash, work completed, and limitations](../benchmarks/idm-complete-20260927/file-coverage-final.csv)
- [Every native binary and decompilation outcome](../benchmarks/idm-complete-20260927/native-coverage.csv)
- [Machine-readable verification results](../benchmarks/idm-complete-20260927/coverage-summary.json)
- [Full raw inventory](../benchmarks/idm-complete-20260927/inventory.json)
- [Resource inventory](../benchmarks/idm-complete-20260927/resource-coverage.json)
- [Imports, exports, and capability evidence](../benchmarks/idm-complete-20260927/component-evidence.json)
- [Cross-file references with source offsets](../benchmarks/idm-complete-20260927/file-consumers.json)
- [Independent DUMPBIN disassemblies and output hashes](../benchmarks/idm-complete-20260927/dumpbin-disasm/summary.json)

The supplied directory and installed program directory each contain 189 files and match byte for byte. Hidden files are included; there is no extension exclusion list. Hash deduplication avoids analyzing identical contents repeatedly while retaining every path. Nested archives are recursively expanded. No reparse directories or inventory read errors were encountered. The separate alternate-stream scan reported no named data streams in its recorded source scope.

The 988 inventory entries comprise 592 source-path records plus 396 entries from archive expansion. Each distinct archive is expanded once; identical copies at other source paths link to the same contents by hash. This count is not a count of all files elsewhere on the computer.

For each native binary, `objects/<sha256>/` contains the immutable snapshot, raw ASCII and ASCII-range UTF-16 strings with offsets, PE headers, sections, imports, exports, resources, certificate/overlay data, and a Capstone entry disassembly. The raw string scan uses a five-character threshold; it is not an exhaustive Unicode or encoded-string recovery method. `ghidra/<hash-prefix>__<name>/` additionally contains defined strings and their cross references, function relationships, pseudocode, per-function results, assembly, executable-byte accounting, and retry results. Assembly and residual byte accounting are retained even when pseudocode fails.

## What the setup actually contains

Input: `C:\Users\Abuzar\Downloads\Programs\idman643build11.exe`.

| Property | Observed value |
| --- | --- |
| SHA-256 | `87254f35dd11786f7ae0a8dc15e211f25cf64754e5fb02f279bcd2cd57bb0e5b` |
| File size | 12,552,928 bytes |
| File version | 6.43.11.1 |
| Architecture | x86 wrapper |
| Authenticode | Valid; Tonec Inc. |
| Requested execution level | `requireAdministrator` |
| Payload | 192 compressed records: manifest plus 191 files |
| PE sections end / payload starts | 95,232 (`0x17400`) |
| Records end | 12,542,245 |
| Certificate table | offset 12,542,264; 10,664 bytes |
| Data after certificate table | 0 bytes |

The setup is a custom container. Generic 7-Zip probing did not recover its complete payload: it found an incidental nested ZIP signature. A format-specific extractor was necessary.

Each record starts with a 32-byte little-endian `<IIQQQ>` header: output allocation/length, compressed length, and three FILETIME values. A zlib stream follows. Record zero contains the installation manifest; `<P1>` through `<P191>` identify subsequent files. The first field of record zero is an allocation capacity (18,441), whereas its decompressed content is 8,846 bytes. Treating that field as an exact length would incorrectly reject the installer. Subsequent payload lengths match exactly.

The final 19-byte trailer contains the payload-start DWORD followed by `!--END FILE--!` and a NUL byte. It sits immediately before the Authenticode certificate table. All bytes between the PE and certificate end are accounted for. Every compressed record passes zlib validation. A deliberately corrupted copy of a compressed stream is rejected during verification.

Evidence: [record offsets, sizes, timestamps, and hashes](../benchmarks/idm-setup-build11/setup-records.json), [decoded setup manifest](../benchmarks/idm-setup-build11/install-manifest.txt), and [extractor](../tools/idm-research/unpack-setup.py). Checksums and successful extraction establish internal consistency; signature verification is a separate recorded check.

### Two-stage installation

The wrapper's recognized non-thunk functions were all decompiled: 271 of 271. Relevant virtual addresses are in the supplied setup, image base `0x400000`:

| Address | Static evidence |
| --- | --- |
| `0x402000` | Locates the footer, walks record headers, decompresses into allocated buffers, creates `IDM<i>.tmp` files, and processes `<P#>` manifest entries. |
| `0x4016b0` | Launches `IDM1.tmp` with a temporary-directory argument and optional setup flags. |
| `0x4018f0` | Parses setup switches; contains setup mutex, temporary directory, error-log, and space-check handling. |

The manifest labels `Uninstall.exe` as the `<SETUP>` component and `IDMan.exe` as `<RUN>`. Consequently, **`Uninstall.exe` also implements the second-stage installer**. Its role cannot be inferred from its name alone.

Second-stage evidence refers to `Uninstall.exe`, SHA-256 prefix `6f6201b354db`, image base `0x400000`:

| Address | Static evidence |
| --- | --- |
| `0x4037ba` | Driver install/removal path. Builds `InstallHinfSection` commands or invokes the architecture-specific integrator; waits for the child process. Contains a `Sysnative\RUNDLL32.EXE` fallback. |
| `0x403268`, `0x403d02` | Service/device checks and IDMWFP installation state/start-stop paths. |
| `0x404d92` | Consumes `IDM0.tmp`, maps numbered payloads to destination names, handles shortcuts and `<SETUP>`/`<RUN>` markers, selects architecture-specific files, and invokes registration processing. |
| `0x405a73` | Opens/maps files for replacement, retries locked files, contains a branch that terminates a running IDMan process, and handles `.old` rename/cleanup paths. |
| `0x40af41` | Parses registry manifest entries; recognizes HKCR/HKCU/HKLM/HKCC, creates/removes values, and constructs uninstall version metadata. |

The architecture-selection branches in `0x404d92` explicitly filter `AA.dll` versus `64.dll` and the corresponding integrator executables. This is stronger evidence than merely finding ARM64 filenames in the archive. Driver INF files independently enumerate the supported architecture sections. These are static branches, not a record of changes made on this computer.

### Setup versus installed files

Of the 191 payload files, 178 match the installed directory, eight differ, and five are setup-only.

- Changed native files: `IDMan.exe`, `IDMNetMon.dll`, `IDMNetMon64.dll`, and `idmvconv.dll`. Their code sections differ, not only signatures or timestamps.
- Changed translations: Arabic, Brazilian Portuguese, Russian, and Turkish.
- Setup-only files: `IDMIntegratorAA.exe`, `IDMShellExtAA.dll`, `IDMNetMonAA.dll`, `idmbrbtnAA.dll`, and `IDMVMPrsAA.dll`.
- Installed-root-only files: `idmcchandler2.dll`, `idmcchandler2_64.dll`, and `IDMSetup2.log`. Some legacy handler DLLs also occur inside nested browser packages, so a root-level difference does not prove package-wide absence.

Evidence: [file comparison](../benchmarks/idm-setup-build11/installed-comparison.json), [PE section comparison](../benchmarks/idm-setup-build11/section-diff.json), and [changed string sets](../benchmarks/idm-complete-20260927/build11-string-diff.json).

Two concrete code-level leads from build 11:

1. `IDMNetMon.dll` SHA prefix `51fa2219d9be`, function `0x10045600`, compares a 24-byte field against `manifest.googlevideo.com` and returns early on a match under specific object-state conditions. Its full downstream effect remains unconfirmed.
2. `idmvconv.dll` SHA prefix `9ce7213f6583` exports `ConvertTsAndAACToMp4_withProgress2` at `0x10019eb0`. The export and its body are real additions to the observed interface. Decompiled parameter types and conversion behavior require further validation; the export name alone does not prove a particular media fix.

In build 11 `IDMan.exe`, `Cancel HTTP2` is referenced by function `0x00592ac0` at `0x00593129`. The surrounding branch tests return value `0x80090304` plus connection state, clears the field at connection-object offset `0x180`, then reaches a connection-restart path. This establishes a conditional cancellation/fallback path associated with that diagnostic. It does not prove a wholesale transport rewrite or a download-speed improvement.

Comparison with installed IDMan function `0x00592c10` makes the change more specific: when the next-server helper returns zero under this error condition, the installed version exits that path. Build 11 first checks the `+0x180` field; if it is nonzero, it clears it, logs `Cancel HTTP2`, and proceeds to restart. If the field is already zero, it still exits. This is evidence of an additional recovery attempt, with the HTTP2 interpretation supported by the diagnostic string; the exact field type remains inferred.

The compared branch ranges were independently decoded with Capstone and checked against Ghidra's instruction bytes and boundaries: [comparison record](../benchmarks/idm-complete-20260927/selected-evidence/http2-comparison.json), [installed branch](../benchmarks/idm-complete-20260927/selected-evidence/installed-http2.asm), and [build 11 branch](../benchmarks/idm-complete-20260927/selected-evidence/build11-http2.asm).

## Previously opaque data recovered

### `IDMFType.dat`: file-recognition database

This file is structured recognition data. Native readers in `idmftype.dll` establish its six tagged sections and reversible word transform. The transform resets for each block and XORs little-endian DWORDs with `(word_index + 1) * 0xe395a6c7`, modulo 32 bits.

Recovered contents:

| Table | Entries |
| --- | ---: |
| MIME strings | 557 |
| Extension strings | 523 |
| Description strings | 2,023 |
| Length-prefixed signature blobs | 1,042 |
| 16-bit mapping entries | 1,297 |
| Fixed-width rule records | 3,224 records of 38 bytes |
| Decoded MIME-to-extension mappings | 500 |

Examples include PDF to `pdf`, MP4 to `mp4`/`m4v`, and ZIP-related mappings. Every source byte is consumed and all decoded blocks round-trip exactly. The internal meanings of every field in the 38-byte rules are not yet named.

Native evidence: `idmftype.dll` SHA prefix `3a47dbb1f86f`; transform `0x100013f0`, table reader `0x10001490`, mapping logic `0x100019a0`, 38-byte stride handling `0x10004400`.

Evidence: [database summary](../benchmarks/idm-complete-20260927/filetype-database/summary.json) and [independent decoder](../tools/idm-research/decode-filetypes.py).

### `idmfc.dat`: capture configuration

An independent Python port of the native table-generation and stream-transform routines decodes this file without loading IDM. Its header is `FORMAT=3;VERSION=26021801;LINES=81`; the decoded content has exactly 81 active rules. The transform round-trips byte for byte.

Native evidence: build 11 `IDMNetMon.dll`, custom hash-like table initializer `0x10004e90`, table construction `0x10005080`, stream generation `0x10005210`, XOR routine `0x10005540`, and configuration reader `0x100169c0`.

This exposes capture configuration and site-specific rules. It does not establish a recovered signing key, user password, or license secret. All seven active flags now have a recorded mechanical interpretation in one browser evaluator; their full native/driver effects remain to be established.

The consumer chain is partly reconstructed: the native reader joins active rules with `|` into object field `+0x1240`. `0x10030c20` places this field under tag `0x12` in a driver configuration buffer before `DeviceIoControl(0x12c008)`. `0x1003a910` places the same field under tag `0x12` in a browser message, with format/client-version gating. In bundled Chrome `background.js` (SHA prefix `3590408894c7`), function `yc` compiles message field 18 into matching expressions and `Gc` evaluates them. `G` and `P` filter GET and POST methods; `<F:...>` supplies a filename-like capture from which an extension can be derived; `<T:...>` supplies an extension-like capture; the `T` flag permits an extension fallback from the request path. The [structured rule inventory](../benchmarks/idm-complete-20260927/capture-config/rules.json) preserves every flag and capture.

A supplemental [browser control-flow trace](../benchmarks/idm-complete-20260927/capture-config/browser-rule-effects.json) records exact source excerpts and offsets for the remaining active flags. `S` permits a request classified under `b.h` through a particular rule branch; `D` requires the secure-scheme/proxy-related field `b.J` to be truthy. `V` shares the `A`/`C` branch, sets request classification fields, and returns for subsequent processing rather than taking the caller's immediate capture branch. `H` sets `b.Y`, rejecting that shared branch and clearing one later response candidate. These are qualified field-level effects, not invented expansions of the flag letters or claims about every downstream outcome. The compiler also accepts `A`, `C`, `X`, and `R`, absent from the decoded active rules; `R` has no check in this evaluator.

Evidence: [configuration summary](../benchmarks/idm-complete-20260927/capture-config/summary.json), [local decoded rules](../benchmarks/idm-complete-20260927/capture-config/decoded-config.txt), and [decoder](../tools/idm-research/decode-capture-config.py).

## Driver interface

The native corpus includes five distinct driver contents: x86/x64 legacy TDI drivers and x86/x64/ARM64 WFP drivers. All receive the same full export and disassembly coverage as application binaries.

The x64 WFP driver, SHA prefix `8acffb018114`, creates `\Device\IDMWFP` and the `\DosDevices\IDMWFP` link. Its entry function `0x14000f8a0` installs device-control handler `0x1400108b0`. That handler contains 11 explicit control codes between `0x12c004` and `0x12c030`; `0x12c028` is absent. The [dispatch map](../benchmarks/idm-complete-20260927/selected-evidence/driver-interface.json) records each code's fields and observed minimum-input check.

The configuration code `0x12c008` reaches reader `0x1400165a0`, whose tag table contains the capture-configuration tag `0x12`. The reader also checks a supplied requestor identifier against the located client object when nonzero. Function `0x140004880` calls the Windows filtering-platform callout and filter registration APIs. These observations connect the user-mode configuration path to a driver consumer without invoking it.

Device-creation [assembly](../benchmarks/idm-complete-20260927/selected-evidence/driver-device-setup.asm) confirms a reference to the raw security descriptor string `D:P(A;;GA;;;AU)` before `WdmlibIoCreateDeviceSecure`. The effective runtime access policy was not tested. The decompiler incorrectly labels the control-message buffer `IMAGE_DOS_HEADER`; those generated field names must not be mistaken for the driver's actual protocol. The dispatch map establishes neither a complete message schema nor exploitability.

## Browser and COM interfaces

All 13 distinct archive containers were included, including CRX, XPI, NEX, nested JAR, and CHM files. Archive signatures and help indexes were retained alongside executable or human-readable content. Older browser support was not discarded because it is obsolete.

The bundled Chrome package uses Manifest V3 and a background service worker; bundled Edge and other compatibility packages retain Manifest V2 variants. Their manifests declare native messaging, request/download interception, tabs, cookies, and other integration permissions. These are the versions inside this specimen, not claims about current store releases.

All five distinct `background.js` variants explicitly call `browser.runtime.connectNative("com.tonec.idm")`; source also contains WebSocket/fallback paths, request-header and response-header listeners, and cookie access. `IDMMsgHost.json` and `IDMMsgHostMoz.json` specify the stdio host `IDMMsgHost.exe` and explicit allowed extension identities. The inspected calls establish integration mechanisms; they do not show which path ran on this machine or establish unrestricted access by arbitrary websites.

Six distinct COM type libraries were decoded using `LoadTypeLibEx(REGKIND_NONE)`, without registering components or activating their classes:

| Component | Interface evidence |
| --- | --- |
| `idmantypeinfo.tlb` | `SendLinkToIDM`, `SendLinkToIDM2`, and `SendLinksArray`; URL/referrer/cookie/POST-data/credential/path/filename/flag parameter declarations. These are parameter names, not recovered user values. |
| `idmBroker.exe` | `IOptionsReader` exposes capture types, exceptions, panel settings, translations, file-type checking, and option access. |
| `idmfsa.dll` | File/system-maintenance interfaces: file operations, registry work, site-access repair, and shell-extension recovery methods. |
| `IDMIECC`, `IDMGetAll`, `downlWithIDM` | Browser/COM integration type information, shared across architecture variants where hashes match. |

All seven distinct XPCOM type libraries parse with zero unaccounted bytes and contain 100 locally declared method entries. They describe legacy DOM/window/link access, HTTP request and response metadata, event listeners, stream callbacks, and download actions. Parent interfaces such as `nsISupports` can remain external unresolved declarations; their implementations are not embedded in the XPT files.

Evidence: [browser package manifests](../benchmarks/idm-complete-20260927/package-evidence.json), [COM parsing results](../benchmarks/idm-complete-20260927/type-libraries/status.json), and [complete XPCOM descriptions](../benchmarks/idm-complete-20260927/xpcom-type-libraries/summary.json). The XPT parser follows the [Mozilla format specification](https://www-archive.mozilla.org/scriptable/typelib_file) with the documented [directory-offset correction](https://bugzilla.mozilla.org/show_bug.cgi?id=575343).

## Small files, resources, signatures, and local state

Several easily overlooked native files have identifiable roles from their actual interfaces:

| File | Direct evidence |
| --- | --- |
| `idmindex.dll` | Exports the SQLite API, including prepare/bind/step, blobs, backup, and aggregate functions. `sqlite3_libversion` at `0x1000d340` returns `3.8.11.1`. Both IDMan variants reference this DLL. |
| `idmmkb.dll` | Exports keyboard/modifier and mouse-hook functions: `GetAltState`, `GetCtrlState`, `InstallHook`, `InstallMouseHook`, `NeedForce`, and `NeedPrevent`. |
| `idmvs.dll` | Version/compatibility helper; exports base-build, full-version, date, and browser-extension-version queries. Its file version alone should not be treated as the version of every IDM component. |
| `oldjsproxy.dll` | Exposes proxy auto-configuration initialization, teardown, and proxy-query functions. Version metadata describes JScript Proxy Auto-Configuration; this descriptive metadata is distinct from signature verification. |

Full export lists and original-filename/version metadata are preserved in the PE and signature evidence.

The inventory also includes 89 distinct language-file contents, 82 standalone image/bitmap contents, toolbar descriptors, licenses, exclusion lists, driver INF files, help pages/indexes, package manifests, and logs. A supplemental language pass uses each file's LANGID and Windows ANSI code page where the file is not UTF-8. It corrects 58 Western-codec fallback interpretations; 88 of 89 files round-trip exactly. `idm_iw.lng` contains an undefined Windows-1255 byte, recorded as a decoding warning while its original bytes remain preserved. Forty language-file variants contain duplicate keys; their runtime precedence was not tested. [Language evidence](../benchmarks/idm-complete-20260927/languages/summary.json) is authoritative over the initial generic text decoding. The toolbar descriptor maps six normal/hot/high-DPI bitmap variants. All 1,321 PE resource entries are extracted and hashed, including string tables, dialogs, icons, version information, manifests, registration data, and type libraries. Not every dialog or image was visually reviewed.

Across 59 PEs and two catalogs, Windows reports 60 valid signatures and one unsigned file: `oldjsproxy.dll`. Being unsigned alone is not a malware finding. Signature verification, Sigcheck output, version metadata, and catalog dumps are retained. Valid signatures do not establish behavioral safety or the absence of bugs.

A separate metadata-only pass covers Roaming/ProgramData IDM state: 210 file occurrences totaling 5,106,420,929 bytes and 115 registry keys. It records file hashes, sizes, roles, and registry value names/types/lengths. Download payloads and personal registry/history values were not copied into the research tree. These bytes are **not** included in the 465-content static corpus and are **not** claimed as fully decoded application formats. The legacy browser support subtree was separately included in the executable/static corpus.

The [auxiliary pass](../benchmarks/idm-complete-20260927/auxiliary/summary.json) additionally validates all 71 PNG files' chunk CRCs and compressed image streams, walks five GIF block structures, checks six BMP headers, parses four CHM system streams and eight CHM fixed-record tables, dumps three PKCS#7 signature containers with CertUtil, and parses the COSE blob's CBOR structure. It reports zero parsing errors. GIF pixel decoding, visual review, cryptographic validation of browser-package signatures, and complete CHM index semantics remain unresolved. Twenty-three auxiliary streams retain raw bytes/header/string evidence without a complete format interpretation.

## Tools actually used

| Tool | Work performed |
| --- | --- |
| Ghidra 12.1.3 headless | Native analysis, recognized-function decompilation without a function-count cap, strings/xrefs/call relationships, assembly, retries, and byte coverage. |
| Microsoft DUMPBIN | Independent section disassembly for all 59 binaries, plus PE headers, imports, exports, load configuration, and TLS inspection. |
| Sysinternals Sigcheck | File version, hashes, and signature reporting. |
| Windows Authenticode / CertUtil | Signature checks and catalog/certificate structure dumps. |
| 7-Zip 26.03 | Container probing, CHM integrity checks and extraction. |
| Python 3.12 / pefile 2024.8.26 | Reproducible PE parsing, hashes, resource/overlay extraction, comparisons, and evidence validation. |
| Capstone 5.0.9 | Independent instruction decoding at entry points and byte-for-byte validation of the compared HTTP2 recovery branches. |
| Windows COM type-library API | Metadata-only interface decoding. |
| Independent format decoders | Setup records, file-recognition tables, capture configuration, and legacy XPT metadata. |

IDA, Binary Ninja, x64dbg, Procmon, WinDbg, and runtime instrumentation were not run for this pass. Static findings do not establish installation side effects, live IPC authorization, network behavior, or driver behavior under load. Prior runtime observations are separately documented in [the earlier transport investigation](transport-analysis-2026-09-24.md); they were not reproduced here.

## Remaining work and interpretation limits

- Review and rename application functions using the retained call graph; automatic export is not equivalent to human comprehension of each function.
- Resolve residual decompiler failures and identify indirect-call targets. Generated prototypes, exception paths, and no-return assumptions can be wrong even when a function is marked successfully decompiled. A concrete example is the converter's `_free` wrapper being treated inconsistently in caller pseudocode.
- Classify executable bytes outside recognized functions. Padding, jump tables, literal pools, embedded data, and undiscovered code must be distinguished before calling these bytes "missing functions."
- Map every field of the file-recognition rules and complete the native/driver interpretation of capture-rule flags beyond the browser effects already traced.
- Fully interpret auxiliary CHM indexes/package signature structures and inspect graphical resources where their appearance matters.
- Validate setup/driver/browser/network behavior in a disposable environment if one becomes available. No runtime evidence from the current machine was manufactured to fill this gap.

No recovered original source tree, private signing key, or complete semantic reconstruction is claimed. The result is a much broader reproducible static research corpus, two successfully decoded configuration formats, a reconstructed installer container and installation flow, and an explicit ledger of what remains unresolved.

Reproduction instructions and script responsibilities are in [the research tools README](../tools/idm-research/README.md).
