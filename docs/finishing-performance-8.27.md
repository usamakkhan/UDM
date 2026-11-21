# Reusing pinned publication input: 8.27

Desktop 8.27; Chromium/Firefox manifest files 0.71.91. The [8.26 comparison](finishing-performance-8.26.md) improved to eight of nine finishing tails and nine of nine full elapsed times, but sequential 64 MiB trial 2 still spent 236 ms in publication and lost its finishing comparison. The old 16.7-second split stall did not recur and its original blocking operation remains unproved.

## Source pin reuse

Publication preparation already opens the staged source read-only, denies concurrent writers, validates its type and size, and retains that handle throughout publication. Unknown-digest publication previously opened the same pathname again solely to obtain its stable file identity or fallback hash. The second open was redundant and could invoke filesystem/filter work again.

Reuse the prepared source handle for identity and fallback hashing. Keep source-name identity/length revalidation, destination-directory pins, destination-state/membership checks, intent-before-rename, durable catalog acceptance and recovery journals. Hashing resets/reads the existing handle; it does not trust a cached pathname. Known/required digest paths retain their existing checks. Five additional tests cover measured identity/size, writer exclusion, correct fallback digest, continued pinned reads across a rename, and rejection of the missing old staging name.

## Publication attribution

Manager-owned transfer workers record PublicationPhases for PrepareSource, VerifyIdentity, PrepareIntent, WriteIntent, Rename, PrepareCatalog, WriteCatalog, AcceptCatalog, RemoveJournal and ReleasePins. ReleasePins includes final local object/handle teardown after the last mark. Timing is advisory and bounded to these calls; it is not a kernel I/O trace. No operation is skipped or moved after the measured process exit.

Publish diagnostics only while the actual transfer cancel token still owns its active entry, before that worker's existing final durable checkpoint. Standalone publication keeps its already-acknowledged catalog proof and does not become dirty solely to append diagnostics after its save. Timing-record failures are swallowed and cannot turn an accepted publication into a failed download. The comparison runner exports these counters alongside settlement/background timings without changing its primary metric or acceptance thresholds.

## Build and validation gates

Build Duplicates and Bridge against the unchanged static/optimized toolchain and canonical core, retaining the other forty core hashes. Reuse target objects only when recursively resolved local inputs are unchanged, and record/verify each input-object hash. Recompile affected targets, relink every binary with the new version resource, and run the maintained native suites before installation. This avoids duplicating source/tool trees or recompiling unchanged test code solely for versioned resources.

Installation must preserve catalog backups, verify all changed files and update version registration. Comparisons retain all three trials, original failures, full elapsed times and actual process exit after background work. Verify final bytes and hashes, restore IDM settings, then remove only owned temporary outputs. No universal speed claim or visual dialog timing follows from a short loopback run.

## Installed outcome

All **1,827 checks passed**; 25 binaries, 490 inputs and 42 canonical core objects qualified. Duplicates and Bridge rebuilt; 40 core hashes retained. Every target relinked with new resources and every reused target object was hash/dependency checked. The preparation-test source received a final EOF normalization, fresh rebuild and 74-check retest; all other frozen input hashes stayed unchanged. Installed and hash-verified seven files and version registration, with personal catalogs unchanged. [Qualification](validation/finishing-8.27.json) · [Installation](validation/finishing-8.27/installed.json)

All eighteen output/final-record hashes matched and IDM settings were restored. UDM won all nine paired full elapsed times but eight finishing tails. Median UDM/IDM tails were 163.656/510.926 ms (4 MiB), 174.535/376.288 ms (sequential 64 MiB), 327.517/437.798 ms (split 64 MiB). Median full elapsed seconds were 1.018/2.165, 8.897/9.843 and 1.984/3.723. The target remains unmet. [All results](validation/finishing-8.27/comparison.json)

Sequential trial 2 lost at 522.237 versus 376.288 ms. The new counters measured **114.739 ms WriteIntent** and **64.502 ms WriteCatalog**, with source preparation 0.103 ms, identity 0.002 ms, rename 0.573 ms, final data flush 63.139 ms and total transfer background 248.602 ms. Full exit remains the primary bound and includes the subsequent final checkpoint/shutdown. This locates current cost in durable metadata writing rather than source identity or hashing, but does not yet separate the individual intent-write APIs. All three split tails stayed below IDM; the historical 16.7-second event did not recur and its exact cause remains unproved.

The eighteen independently rechecked benchmark payloads were individually removed, 830,472,192 logical bytes. [Cleanup](validation/finishing-8.27/comparison-cleanup.json). Subsequent work prepares publication storage before body reception and splits atomic-intent observation further; this failed trial remains retained.
