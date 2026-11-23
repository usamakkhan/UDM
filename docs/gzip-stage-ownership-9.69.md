# Gzip staging ownership: UDM 9.69

UDM 9.69 protects the decoded and prefix-stripped temporary outputs of ordinary gzip HTTP downloads. Their creation is exclusive, and cleanup requires the identity captured from the actual new-file handle. Both source extension manifests advance to 0.73.33. Five native products build; all 1,425 assertions in nineteen affected suites pass. The same 57 ownership checks also pass on C:, and a separate 43-check gzip workflow run passes with decoded output on C: and the catalog/encoded spool on D:.

This increment does not install an update or claim a new IDM speed comparison. Installed app 8.68, installed extension files 0.72.32, personal catalogs and shared helpers are unchanged.

## Confirmed before/after behavior

The same self-contained HTTP fixture links against frozen 9.68 core objects and current 9.69 objects. It sends independently framed valid gzip data containing a small PDF-signature test document, with or without a completed HTML notice before the document. It exercises actual decoding, prefix recognition/removal and publication.

The 9.68 control fails seventeen of 57 assertions. A pre-existing decoded or stripped file can be truncated, and an empty directory can be removed during unwinding. After either output has been created, a preflight interruption can delete a different file or directory placed at its old name. Even successful plain decoding can delete a pre-existing stripping file that this attempt never used. The control retains its successes and failures; none are discarded.

With 9.69 all 57 pass. Occupied paths prevent publication without modifying the existing item; failed publication removes only unchanged owned temporary files, preserves replacements and retains encoded recovery bytes. Successful plain and stripped output match the exact expected document hash and decoded size. A saved Complete record survives, and an unused stripping pathname no longer grants deletion ownership.

## Implementation

Both CreateFile calls use CREATE_NEW and request attribute-read access on the existing writer handle. Each successful creation immediately captures a separate `CreatedStagingFile` proof. The proof tracks volume, full file ID and creation time, excluding mutable output size and last-write time. An unopened or unused output has no valid proof and cannot be removed by this guard.

On cleanup, the existing publication-recovery check still takes precedence. Without a pending recovery transaction, the guard delegates each output to its own proof: open the named object with delete/attribute access, exclude competing writers/deleters, compare identity, then request deletion through that same handle. Unknown identity, name replacement, reparse/directory substitution or failed access conservatively retains the item. The regular-transfer implementation introduced in 9.68 supplies this shared primitive; no new identity format or global cleanup registry is added.

Decoded-byte verification, gzip CRC validation, encoded receipt invalidation, prefix selection, confirmation waiting, durable publication and final wire/output accounting retain their existing order. The additional capture queries run on newly created output handles. No measured zero-cost, speed or power-loss claim follows.

## Validation

The nineteen affected suites pass 1,425 assertions, including 57 gzip ownership assertions and seventeen decoder assertions. A separate invocation of the same ownership binary passes 57 assertions on C:. Earlier D: precheck assertions are retained but are not added to the registered-suite total.

The existing Node gzip server and native Manager integration fixture pass 43 checks with X-GZip on initial responses and canonical gzip on range resume. They verify known/unknown lengths, concatenated members, empty output, malformed data rejection, exact decoded size before Start, final size/hash after Start, a changed destination, Download Later reuse and encoded-offset pause/resume. Output resides on C: while native state and wire data reside on D:. The server independently hashes its 512 KiB payload and records requests.

Each of the seven routes that must reuse one response makes exactly one request: known, unknown, concatenated, empty, prefetch, relocated and deferred. The pause test resumes at a nonzero encoded offset with its strong validator. This establishes the tested native workflow and request counts; it is not a new live browser/dialog test or a claim about every server or media format.

[Build receipt](validation/gzip-stage-ownership-9.69/build/build-result.json), [all affected suites](validation/gzip-stage-ownership-9.69/build/suite-results.json), [same-source controls](validation/gzip-stage-ownership-9.69/controls.json), [before failure](validation/gzip-stage-ownership-9.69/before-968/negative-control.json), [C-volume ownership](validation/gzip-stage-ownership-9.69/c-drive/results.json), [workflow checks](validation/gzip-stage-ownership-9.69/integration/results.json), [recorded requests](validation/gzip-stage-ownership-9.69/integration/requests.json), [final audit and cleanup](validation/gzip-stage-ownership-9.69/final-audit.json).

## Remaining work and cleanup

The shared `publishMediaOutput` relocation helper still has independent staging/source cleanup to audit. This change protects the gzip decoding guard, not every operation performed by that helper when confirmation changes the destination. Media assembly staging and process-loss recovery of unjournaled copied stages also remain open. Unsupported identity queries retain temporary data; no unsupported-filesystem or hostile-system guarantee is made.

There is no new installed-app, GUI, browser or successful-download timing comparison against IDM. Sequential/split finishing and exclusive first-availability gates remain unresolved. Broad IDM parity is unproven.

After evidence export, 47 generated binaries/objects and 337 selected test payloads were removed individually using verified file hashes and literal paths, without recursive deletion. Their combined logical length is 246,772,097 bytes (about 235 MiB); physical free space was not measured. Five native products, 42 referenced core objects, sanitized records and small private directories remain.
