# Advisory hash and publication overlap: 9.73

UDM 9.73 / source extensions 0.73.37 is a built, functionally checked **unpromoted candidate**. It lets an ordinary split download's optional checksum run alongside publication-intent persistence. It fails the matched no-regression performance gate and must not be presented as an IDM speed-parity fix or an installation-ready performance release. Installed UDM remains 8.68 and installed extension files remain 0.72.32.

## Implementation and ownership

The [9.71 pilot](current-finishing-pilot-9.71.md) identified a late-hash wait before the transfer could hand publication to its Manager owner. For eligible fixed-volume ordinary downloads with a complete budgeted write buffer, the hash task now owns the existing buffer allocation and moved hash state. No second body allocation or additional hash pass is introduced. It can outlive the transfer stack without borrowing that stack's hasher or cache.

The prepared completion owner adopts the task only after it has an actual body-writeback pin. A registered ordinary publication can persist an early intent with an empty advisory digest, the captured source identity and `FlushBeforePublish`. It then joins body writeback and hashing, revalidates the source/destination, renames, and prepares the final catalog snapshot with the joined valid digest. The durable intent retains its original empty digest; it is not rewritten with an undocumented in-memory acceptance token. Existing recovery independently hashes the identity-bound file and can recognize the completed record's actual digest.

No completion snapshot is accepted while either task remains pending. Cancellation and exceptional unwind stop/join owned hashing before staging cleanup and ownership retirement. Failure of an optional hash leaves its digest empty, as before. Required server/user/replacement verification remains on the existing earlier path. Required or already-known digests cannot be handed to this advisory policy. Private/ephemeral publication retains its digest-bound receipt format rather than adopting the new identity-only early-intent path.

9.72 / extensions 0.73.36 built five products and passed 1,506 affected assertions, including the initial 48-check ownership fixture. Review found a stale-recorded-hash handoff problem: consulting the job's old `Sha256` while the new task was pending could supply the wrong intent digest. 9.73 avoids that lookup for a pending advisory task and explicitly rejects required/known-digest misuse. It expands the fixture to 64 assertions. Neither version was installed.

## Functional evidence

- All 1,522 assertions in twenty-one affected native suites pass, including the 64 advisory ownership checks, ordinary publication/recovery, gzip staging, destination relocation, interruption ownership and launch checks.
- The additional 119 buffer/cache checks pass, covering allocation budgeting, lifetime transfer and reuse.
- Seventeen actual HTTP checks pass across ordinary sequential, required matching checksum, required mismatching checksum and ranged transfers. The mismatch rejects publication. These checks call the standalone transfer path; the normal-product comparison below separately exercises the Manager-owned advisory handoff.
- All eighteen normal-product comparison downloads match the independent size/SHA-256 oracle and saved Complete records. Each run observes the app's actual process exit and performs the same explicit post-exit output-file flush.

Early prechecks are retained separately and not added again to the registered-suite total. The initial-source snapshot contains seven exact 9.72 files; together with unchanged current inputs they verify all 690 captured initial inputs. All 690 final inputs also verify. A descriptive baseline label in the final source-inventory JSON was inherited incorrectly; its actual driver loads 9.72. [The correction record](validation/advisory-intent-9.73/provenance-corrections.json) preserves the raw label and identifies the actual prior build.

## Matched normal-product performance: gate failed

Three trials of 4 MiB sequential, 64 MiB sequential and 64 MiB split alternate 9.73 and frozen 9.71. There is no compiler running during measurement, no private product instrumentation, no pacing change, no killed process and no discarded losing trial. This matrix contains **no IDM run**: current-session IDM remained running and the requested normal exit was still pending. Windows control had failed to initialize, so it could not safely inspect or close that process. IDM settings were neither read nor modified by the explicitly derived UDM-only driver.

Medians, in milliseconds:

| Case | Exclusive availability, 9.73 / 9.71 | Process-exit tail, 9.73 / 9.71 | Full elapsed through exit, 9.73 / 9.71 | Post-exit flush completed tail, 9.73 / 9.71 |
| --- | ---: | ---: | ---: | ---: |
| 4 MiB sequential | 121.200 / 114.311 | 169.252 / 157.281 | 1,028.917 / 1,009.310 | 193.402 / 184.194 |
| 64 MiB sequential | 170.188 / 177.478 | 218.063 / 227.001 | 8,947.837 / 8,942.166 | 253.485 / 248.352 |
| 64 MiB split | 272.510 / 255.218 | 322.687 / 301.704 | 2,402.591 / 2,627.631 | 334.151 / 323.951 |

9.73 wins five of nine pairs for exclusive availability, process-exit tail and full elapsed; it wins four of nine for both post-exit-flush tail and full elapsed through that flush. Thus **all-pairs no-regression fails**. Even the better sequential process-exit median does not establish better durable elapsed time. Three samples per case do not establish statistical equivalence, and the machine was not isolated from every other application/system service.

Every new split sample records the actual commit handoff and intent-overlap flags. Hash-task durations are 58.5–68.2 ms; owner hash joins are 3.04–3.14 ms. Body flushes are 121.8–203.7 ms and intent writes are 123.7–205.4 ms. Moving the earlier hash dependency therefore does not guarantee a shorter storage barrier. In the first split pair, final exit tail regresses from 301.704 to 366.878 ms despite better full elapsed; this loss is retained.

Exclusive availability includes release of the file pins after catalog acceptance. Process exit includes observed app-owned work and `/q` shutdown. The post-exit output flush is an additional symmetric file barrier, not proof that all system services or directory metadata are durable. No GUI/dialog timing or hardware power-loss result follows.

## Next work and release gate

Separate the remaining owner hash-join time into future retrieval, completed-buffer release and digest validation before assigning a cause. The value-owned task currently retains its buffer in its capture; releasing completed hashing storage inside the task is a candidate to investigate, not a measured fix yet. Preserve the budget token's exact lifetime and include any release cost in the task and total process lifetime.

The larger sequential/split waits still require evidence about actual body/intent/catalog storage operations. Do not substitute another pacing setting, omit a flush, move cleanup outside the measurement, or select a better rerun. A new candidate needs interruption/recovery checks and a new complete matched matrix. A fresh clean-session IDM comparison, installed validation and UI parity remain open. The source-entry ownership and pathname-rename intervals described in [9.71](relocation-ownership-9.71.md) also remain open.

## Records and disk cleanup

[Final build](validation/advisory-intent-9.73/9.73/build-result.json), [1,522 affected assertions](validation/advisory-intent-9.73/9.73/suite-results.json), [64 advisory checks](validation/advisory-intent-9.73/9.73/suites/AdvisoryPublicationTests.json), [buffer checks](validation/advisory-intent-9.73/body-budget/results.json), [HTTP checksum checks](validation/advisory-intent-9.73/9.73/required-hash-results.json), [all eighteen normal runs](validation/advisory-intent-9.73/comparison/results.json), [losing pairs and medians](validation/advisory-intent-9.73/comparison/analysis.json), [driver transformations](validation/advisory-intent-9.73/drivers/owned-pair-9.73-provenance.json), [final audit](validation/advisory-intent-9.73/final-audit.json).

The comparison deletes all eighteen verified outputs immediately: 830,472,192 logical bytes (792 MiB). After export, another 122 disposable binaries/objects and 570 selected payloads were removed individually with path/type/hash checks, totaling 718,514,226 bytes. Combined logical length is 1,548,986,418 bytes (about 1.44 GiB); physical free space was not measured. Metadata and sanitized evidence remain. Current five native products, 42 referenced core objects and the five 9.71 comparison products are retained. Unneeded intermediate 9.70/9.72 products and unreferenced 9.71 core objects are retired. No shared ffmpeg/ffprobe copy was created or removed, and personal catalogs are unchanged.
