# Relocated output ownership: UDM 9.71

UDM 9.71 and both source extensions 0.73.35 bind publication and cleanup to the actual opened file, including when confirmation changes the destination folder. Five native products build. All 1,458 assertions in twenty affected suites pass. This is a correctness improvement; no new IDM timing result or installation follows.

## Behavior and implementation

The previous relocation helper could accept a substituted staging pathname or delete a replacement at an old source name. The identical relocation fixture against frozen 9.69 code reproduces 24 failed assertions out of 66. Current tests preserve equal-byte replacements, different-byte replacements and directories, reject substituted publication sources, and still complete healthy relocation with exact expected bytes.

A same-thread, single-use scoped handoff binds the publication source to its Manager, job and normalized stage path. Publication adopts the actual read handle and records its stable identity when available. Wrong-path calls cannot consume the valid handoff; replay and unrelated-job attempts cannot reuse it. Recovery continues to hash and verify bytes.

Source and stage handles remain alive through cleanup. A copied stage retains its creator handle until a temporary guard protects its name; a strict reader then takes over before the name guard is released. Cleanup compares the named object with its creation proof and retained live reference, and deletes through that checked handle. Unknown identity or failed access retains the item. Legacy file IDs require a live reference that excludes competing writers; an unleased legacy snapshot cannot authorize deletion.

Cross-volume copy verification remains one full hash pass, now on the retained reader. Same-volume relocation does not add a full-file hash. Extra handle and metadata operations have a cost that has not been measured here. The read factory also needs performance qualification of its access hints.

## Versions and corrections

9.70 / extension 0.73.34 introduced scoped handoff and identity-aware cleanup, built all five products and passed the affected checks. It was never installed. Its legacy cleanup proof could outlive the referenced object, so it is superseded by 9.71's retained object lifetime and explicit live-reference requirement. The earlier seventeen-check fallback component result does not qualify the final design.

The first 9.71 compile attempted assignment to a handle type without move assignment. The corrected code uses optional holders with explicit construction/reset; no global handle API change was made. The first scope fixture omitted its output directory; its failed setup is retained separately from the corrected five-check result. A later attempt to reconstruct the intermediate 9.70 header did not match its recorded hash and was rejected; no reconstructed header is presented as original evidence.

## Validation and evidence

- Twenty affected native suites: 1,458 assertions passed, including the D: relocation cases.
- Separate relocation invocation: 66 passed across D: hard links and C:-to-D: copies. These overlap the registered relocation cases and are not an additional unique-suite total.
- Real Manager scope fixture: five passed, covering binding, replay and scope unwinding.
- Standalone legacy identity fixture: twenty passed. Only the 128-bit identity query is forced unsupported; fallback uses actual Win32 handles. Both tested volumes are NTFS. This is not qualification on FAT, ReFS or a network filesystem.
- Gzip/native workflow: 43 passed with decoded output on C: and state/wire data on D:. It checks exact decoded size before Start, final bytes/hash, destination changes, Download Later and encoded-offset resume. Seven reuse routes each made one request; one resume request used a nonzero encoded offset.

[Build receipt](validation/relocation-ownership-9.71/build/build-result.json), [suite results](validation/relocation-ownership-9.71/build/suite-results.json), [same-source controls](validation/relocation-ownership-9.71/controls.json), [cross-volume checks](validation/relocation-ownership-9.71/cross-volume/results.json), [scope and legacy checks](validation/relocation-ownership-9.71/probes-corrected/results.json), [workflow](validation/relocation-ownership-9.71/integration/results.json), [requests](validation/relocation-ownership-9.71/integration/requests.json), [intermediate evidence](validation/relocation-ownership-9.70/export-manifest.json), [final audit](validation/relocation-ownership-9.71/final-audit.json).

After sanitized export, 113 generated binaries/objects and 695 selected test payloads were deleted individually with path, type and hash checks, without recursive deletion. Their combined logical length is 604,515,246 bytes (about 577 MiB); physical free space was not measured. The current five products and 42 referenced core objects remain, along with five intermediate products selected as a comparison baseline. Installed 8.68, installed extension files 0.72.32, personal catalogs and shared helpers remain unchanged.

## Remaining gates and next work

Pinning starts at helper entry; caller verification and helper entry are not yet one continuous ownership interval everywhere. Final source-name validation followed by pathname-based MoveFileEx still has an interval to audit. Media assembly creation, retained-output discard and process-loss recovery of unjournaled copied stages remain open. No hardware power-loss or hostile-system guarantee follows.

The next performance task is a paired successful-download comparison against IDM, retaining losing results and measuring publication, process lifetime and remaining background work. Sequential/split finishing and exclusive first-availability gates remain unresolved. There is no new live UI/browser result, installed comparison or claim of complete IDM parity in this increment.

A subsequent [three-case normal-product pilot](current-finishing-pilot-9.71.md) verifies current completion and removes another 132 MiB of payloads. It identifies a pre-handoff advisory-hash dependency in the split path and a remaining body-flush join in the sequential path. There is one sample per case and no new IDM pair, so it does not change the failed performance gate status.
