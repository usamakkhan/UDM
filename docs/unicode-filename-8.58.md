# UDM 8.58: Unicode-safe filename truncation

Native source is **8.58**, and both extension source manifests are **0.72.22**. Browser JavaScript is unchanged. Five native products built. Installed desktop remains **8.32**; installation remains deferred, with no installer creation, extension deployment, push or release.

## Reproduced failure

The qualified 8.57 core rejects a valid filename during actual `Manager::add` admission when a supplementary Unicode character crosses the 140-unit stem cutoff before an extension. The new workflow fixture reproduces `Invalid Unicode text.` in its first manual-admission case against the old engine. This is separate from the earlier 8.56 exploratory probe. [8.57 control](validation/unicode-filename-8.58/baseline-8.57.json).

Windows represents these characters with two UTF-16 units. The previous code concatenated `name.substr(0,140)` and `extension.substr(0,19)`, then removed a trailing high surrogate only at the end of the combined string. A half-character at the stem cutoff sat before the appended extension and escaped that final check. Converting the result back to UTF-8 failed.

## Implementation

Each clipped part now removes an incomplete trailing high surrogate before concatenation. The name prefix and extension use the same bounded helper independently. The first part still comes from the normalized full name's first 140 units, preserving the existing prefix selection. A pair fully inside a boundary is preserved; a pair crossing the boundary is omitted as a whole.

The existing 160-unit trigger, 140-unit name-prefix budget and 19-unit extension budget remain. Ordinary ASCII outputs, path stripping, unsafe-character replacement, device-name prefixing, invalid UTF-8 refusal and the 8.57 cached regex remain. No public Job/Manager interface or catalog schema changed. Clipping preserves complete UTF-16 pairs; multi-scalar grapheme segmentation remains outside this increment.

## Qualification

All **2,286 checks in thirteen final-binary-matched runs** pass:

- **290 filename checks on each drive**, comprising the original 194 checks and 96 additional expected-result/idempotence checks. New cases surround the stem cutoff, preserve exact 160-unit names, cross extension boundaries, cross both boundaries in one name and account for the extra reserved-name underscore. Four supplementary scalars exercise different surrogate values, including U+10000 and U+10FFFF. The existing twelve-worker simultaneous first-use checks still pass.
- **84 Unicode workflow checks on each drive**, across six independent filename cases. Actual manual admission chooses the exact target; Windows creates/reads an owned sentinel at that path; Properties accepts the normalized name and preserves its bytes; native browser-message receipt normalizes both suggested and reported server names; Download Later remains paused in the owned destination. A real refused catalog write rolls back admission without a partial row or changed saved catalog.
- Workflow save/reload preserves manual/browser records and existing file hashes. Raw catalog parent and nested video/audio names normalize without broken surrogates, retain identity/metadata, preserve the original first-load backup byte for byte and survive another save/load.
- Retained catalog loading, Content-Disposition names, Properties decisions, catalog checkpoints, recovery catalog transactions, publication recovery and synchronization replacement checks pass. Catalog loading and recovery catalog cases also run on D:.

[Final suite records](validation/unicode-filename-8.58/suite-results.json), [validation notes](validation/unicode-filename-8.58/validation-notes.json).

The first fixture attempt incorrectly used `DuplicatePolicy=Add` with the validated settings API, which correctly refused it. The fixture was corrected to the existing supported `Numbered` policy; production settings policy was unchanged. Its initial result is retained separately. The corrected 8.57 control then reproduced the Unicode error. Initial focused candidate passes of 290 and 84 checks are also excluded from the final aggregate. [Initial fixture error](validation/unicode-filename-8.58/initial-fixture-error.json).

## Native build and real download

Five products and nine backend fixtures built from **516 frozen inputs and 42 core objects**. Core and Bridge were rebuilt; twenty-two qualified native units and eighteen external C units were retained after include-closure verification. Every candidate file version is 8.58. The baseline fixture used the qualified 8.57 core/resource bundle; its objects and resource remain unchanged. The exact seven-input delta reconstructs both sides, including raw line endings. [Build evidence](validation/unicode-filename-8.58.json), [source reconstruction](../benchmarks/speed/experiments/unicode-filename-8.57-to-8.58.json).

A real source-app command-line download received a filename consisting of 139 `a` characters, an emoji, 30 `b` characters and `.bin`. The command passed that original name to UDM; it did not substitute the expected normalized name beforehand. The complete **4 MiB** output was saved as 139 `a` characters followed by `.bin`, matching the expected whole-character cutoff. One origin request supplied the exact independently generated length/hash. The saved catalog records Complete with that digest and filename, the process exits normally after its owned work, a separate completed-output flush succeeds, and no publication intent remains. Its payload was deleted immediately. [Actual download record](validation/unicode-filename-8.58/app-4m-unicode-smoke.json).

Native browser-message receipt is exercised directly. This is not installed Chrome toolbar, File Info dialog or installed desktop acceptance. The single real transfer establishes integrity/lifecycle behavior for this filename case; it is not an IDM comparison or a finishing-speed result. No new performance benchmark was needed for the clipping correction, and no overall speed claim is made.

## Cleanup and remaining gaps

Removed **768 generated files / 133,914,094 logical bytes**, plus the separate 4 MiB payload, and 757 empty owned directories. Retired fixture executables, entry objects, synthetic catalogs and sentinel files stayed within explicit owned roots. Cached canonical boundaries, reparse refusal, file hashes and complete hard-link-group checks constrain deletion. Five products, Core/Bridge objects, forty shared core references, resources, compact evidence, qualified prior references, shared tools, personal catalogs and installed executable remain. Logical bytes do not establish physical reclaimed allocation. [Cleanup receipt](validation/unicode-filename-8.58/cleanup.json).

The original finishing/flush stalls and their kernel cause remain unresolved. Further work still includes installed/browser acceptance, constructor IO/parsing/recovery attribution, external namespace/caller audits and the other IDM parity gaps. The filename correction is qualified in the source build; the broad inside/out parity objective remains incomplete.
