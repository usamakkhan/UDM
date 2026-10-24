# Content signatures checked against the libmagic test corpus (2026-10-23)

UDM's content recognition (`native/ContentSniff.hpp`) is written from public format
specifications; no third-party signature database is included. To find formats it
missed, the public libmagic test corpus (the `tests/*.testfile` inputs of the `file`
project on GitHub, with their `.result` descriptions) was run through the table with
`Udm.NativeTests.exe --sniff-corpus <folder>`. The corpus is test input only and is not
vendored: `python -I tools/fetch-libmagic-corpus.py <folder>` downloads it.

## Added from the corpus run

| Format | Signature | Extension |
|---|---|---|
| PDF behind a UTF-8 byte order mark | `EF BB BF %PDF-` | pdf |
| SVG (UTF-8, with or without BOM or XML declaration; UTF-16LE with BOM) | `<svg` within the first 4 KiB and no `<html` | svg |
| Animated PNG | PNG with an `acTL` chunk before `IDAT` | png (apng compatible), description "Animated PNG image" |
| Zstandard legacy frames v0.2 to v0.7 | `22..27 B5 2F FD` | zst |
| Hangul Word Processor 3.x | `HWP Document File V` | hwp |
| Hangul Word Processor 5.x | compound document with `FileHeader` and `BodyText`/`DocInfo` streams | hwp |
| HWPX | ZIP whose mimetype is `application/hwp+zip` | hwpx |
| DSDIFF audio | `FRM8` and `DSD ` at 12 | dff |
| FIT activity data | `.FIT` at 8 | fit |
| GEDCOM | `0 HEAD` | ged |
| Group Policy registry settings | `PReg 01 00 00 00` | pol |
| UF2 firmware image | `UF2\n 57 51 5D 9E` | uf2 |
| Binary G-code | `GCDE` | bgcode |
| Android vdex | `vdex`/`wdex` and a digit | vdex |
| AMD/Xilinx accelerator binary | `xclbin2\0` | xclbin |
| Xilinx bitstream | `00 09 0F F0 0F F0 0F F0 0F F0 00 00 01` | bit |
| Keyman compiled keyboard | `KXTS\0` | kmx |
| Tarantool write-ahead log / snapshot | `XLOG\n` or `SNAP\n`, a `0.` format version and a `Version: <digit>` line | xlog, snap |
| OpenPGP key (binary) | public or secret key packet, old or new packet format, key version 2 to 6 | pgp (not specific) |
| Linux ext2/3/4 image | `53 EF` at 0x438 | img (not specific) |

The PDF and SVG rows matter most for downloads: before this change a PDF served with a
byte order mark and any SVG were classified as plain text, which the web-page stop
treats as a server error page for binary file names.

## Corpus outcome

The opt-in check holds an expectation for every corpus file: 69 recognized with the
extension above or one already in the table (ARJ, MP3, DSF, ZIP, tar, DOCX, XLSX, AMR,
PNM, RPM, ELF, Zstandard v0.8), 18 that stay text (scripts, JSON, XML without a
signature of its own, a Python snippet that merely mentions `%PDF`) and 14 left
unrecognized on purpose (bcachefs, binary shebang scripts, Apple driver map, EFI
signature list, HDD Raw Copy Tool, Racket bytecode, PCjr cartridge, RTFD, Tarantool
vinyl files, an escape-sequence text) so an accidental match is caught. JSON is not
recognized as its own type: a JSON validator is more than the download path needs.

Run on 2026-10-23 against the corpus as fetched that day: every expectation passed.
The unit checks in `ContentSniffChecks.hpp` cover each added signature from
specification-built bytes and run in the full suite without the corpus.
