# File access after durable publication acceptance — 9.60

Release ordinary publication file pins after the exact completed catalog proof is accepted, before journal cleanup, while retaining publication ownership and directory protection through cleanup. Keep pre-acceptance write/rename exclusion and all existing flush/checkpoint work. Build five native 9.60 products and pass 994 affected assertions. All 27 matched download outputs verify correctly. The IDM first-availability gate and the all-pairs no-regression gate against 9.59 still fail, despite faster process-exit medians. Source browser manifests advance to 0.73.24; installed 8.68 remains unchanged.

## Confirmed blocking owner

The exclusive-read benchmark cannot open a file while PublicationPreparation retains a GENERIC_READ handle. Before this increment, both the measured source and the pinned published name stayed open until publishFile returned, including time spent removing an already-accepted publication journal. A normal shared reader could read the complete file sooner; path existence, shared access, exclusive access, GUI completion and process exit are distinct events.

The frozen 9.59 core control holds publication-cleanup after the completion record is durably accepted. Its earlier cases verify correct bytes and retained write/rename exclusion before acceptance. At cleanup, the saved record is Complete with the correct hash and the recovery journal still exists, but exclusive open fails with a sharing violation. The expected control failure is retained with all 21 executed assertions and its exact core/fixture hashes.

A separate small D: file-sharing probe rejects a tempting alternative: using only FILE_READ_ATTRIBUTES lets exclusive readers in but also permits writes, rename and deletion while the handle is open. That would weaken pre-acceptance protection. It is not used by the production change. The initial incomplete probe is excluded; the retained repeated observation is an ad hoc diagnostic, not part of the 994 regression assertion total.

## Ownership change

PublicationPreparation::releaseAcceptedFile closes the source and published-name handles. Manager::publishFile calls it only after the catalog proof resolves successfully and the exact current completion/commit identity is accepted. The Manager lock is released before closing those handles. Directory pins, the publication lease and the existing journal cleanup remain alive through the normal function lifetime.

Before durable acceptance, the published bytes and name remain protected from mutation. After acceptance, external readers and writers no longer wait for UDM's journal cleanup. This removes UDM's own file handles; it does not change filesystem permissions or guarantee that other applications hold no handles. The replacement publication path is unchanged.

No flush is removed or moved, no required hash or receipt check is bypassed, no worker is detached and no completion endpoint is substituted in the comparison. Internal pause/remove/shutdown guards remain owned until the publication operation finishes. The ordinary worker's active lifetime and CLI quit policy still include cleanup and background work.

An accepted journal that cannot be removed is retained as before. If the user modifies the accepted file while that journal remains, recovery preserves the modified bytes and requests manual resolution. It does not silently replace the user's file or revoke the acknowledged completion merely to make recovery pass.

## Regression evidence

All 994 assertions pass across eight suites: availability 52, preparation 87, commit 347, ownership 186, recovery 73, early-publication recovery 156, silent-launch failure 49 and CLI/queue completion 44.

The new six-scenario fixture covers held stages before catalog work, during catalog work and after exact acceptance, including a project directory, actual journal-delete sharing failure and user modification after acceptance. It verifies normal readers, exclusive readers, write and rename exclusion before acceptance, durable completion before cleanup, exclusive access after acceptance, fresh Manager reopening and non-destructive handling of the modified accepted file. Existing commit/ownership/crash-recovery checks retain their original assertions.

The old five-case fixture is retained separately; the current fixture adds the user-edited-file case and its recovery branch. Reconstruction patches expose that difference instead of claiming the fixtures are byte-identical. The old control uses verified 9.59 core objects. All current products use the same qualified production source, without a test-only behavior define. Only Duplicates and the version-bearing Bridge core units require recompilation.

[Validation and source identities](validation/publication-availability-9.60/status.json), [control failure](validation/publication-availability-9.60/baseline/result.json), [all backend assertions](validation/publication-availability-9.60/suites.json), [rejected metadata-only probe](validation/publication-availability-9.60/rejected-metadata-pin-probe.json).

## Real download comparison and failed gates

After compilation stopped, the actual 9.60 App, frozen 9.59 App and installed IDM ran three rotating client positions for each of three cases: 4 MiB sequential, 64 MiB sequential and 64 MiB split. All 27 outputs have the expected independent size/hash and were removed immediately after verification. The original observer and its exclusive-open definition are unchanged.

The primary comparison includes actual application exit; a symmetric explicit output flush is also measured after exit. It includes app-owned worker and shutdown work, not every Windows service. The first-availability probe polls at 2 ms intervals and does not establish a universal 1 ms bound. This is a development-binary comparison, not an installed 9.60 test or a visual completion-dialog timing test.

| Median after last server body byte | 9.59 control | 9.60 | IDM |
| --- | ---: | ---: | ---: |
| 4 MiB sequential, first availability | 135.9 ms | 128.6 ms | 33.5 ms |
| 4 MiB sequential, through process exit | 185.6 ms | 178.9 ms | 388.2 ms |
| 64 MiB sequential, first availability | 183.1 ms | 140.6 ms | 71.7 ms |
| 64 MiB sequential, through process exit | 227.9 ms | 192.5 ms | 548.5 ms |
| 64 MiB split, first availability | 287.8 ms | 234.9 ms | 154.6 ms |
| 64 MiB split, through process exit | 339.6 ms | 282.3 ms | 409.3 ms |

Against IDM, current UDM wins all nine process-exit tails, total elapsed times and post-exit durability comparisons, but loses every first-availability tail. The first-availability gate fails. Against 9.59, current UDM wins seven of nine first-availability and process-exit tails and six of nine total elapsed times. Two finishing tails and three full elapsed runs are slower; the strict all-pairs no-regression gate fails. All samples and outliers remain in the raw evidence.

The controlled ownership test proves that accepted files no longer wait for journal cleanup. The rotating end-to-end observations do not isolate a universal code-only speedup or establish that this small release change closes the larger availability delay. Faster medians must not conceal the slower individual pairs.

IDM returns exit status 1 in all nine complete, hash-correct transfers, matching earlier retained controls; the meaning is not independently established. UDM and its 9.59 baseline return 0. Raw exit codes are retained, not reinterpreted as zero. Connection values/types are restored afterward, and the other Windows session's IDM is left alone.

[All 27 raw results](validation/publication-availability-9.60/comparison/runs.json), [every pair, median and explicit gate](validation/publication-availability-9.60/comparison/summary.json).

## Next work

The remaining critical path precedes durable acceptance. Trace final body/hasher retirement, writeback, metadata, durable intent, rename, final catalog checkpoint and body-handle release on a common clock. Preserve the original exclusive-open endpoint and additionally distinguish a normal reader and the actual GUI event. Recorded phase durations alone do not prove which subsystem caused a slower pair.

Investigate safe overlap only after identifying independent work. PublicationPreparation currently denies writes, so acquiring its read pin before final writeback would itself block the write-only flush handle. A metadata-only substitute is not safe. Any prepared-intent protocol must bind source identity, destination inputs, exact completion/receipt identity, cancellation and joined ownership, retain intent durability before rename, and preserve copied/replacement/removable/network/error-recovery semantics. Do not remove a flush, publish a false completed record or add pacing to make a graph green.

The next improvement must recheck first availability, total elapsed, process exit and output durability together against both the prior build and IDM. No installed promotion or full parity claim follows from this batch. Live browser/visual comparisons, toolbar discovery, remaining settings paths and clean-checkout GUI build profiles also remain open.

## Build and cleanup

Five native products are built at 9.60: app, browser host, monitor, setup support helper and automation host. Both source manifests advance to 0.73.24 as paired version bookkeeping; this increment does not introduce new extension runtime capture logic. The build records 677 source inputs, required core identities, resource versions, source-dependency auditing and compiler logs. Existing compiler warnings and the private-wrapper build portability gap remain.

The 27 verified payloads were removed (1188 MiB logical total), followed by 630 owned test/compiler files (139.6 MiB logical size). Current and previous products, referenced core objects, both personal UDM catalogs, installed files and shared ffmpeg/ffprobe are preserved. No installer, installation, commit, push or release is created. Installed app 8.68 and extension files 0.72.32 remain unchanged.

## Subsequent common-clock investigation

The [critical-path follow-up](publication-critical-path-9.60.md) records eight additional diagnostic transfers, shows the three serial durability regions, and rejects the optional-flush suppression experiment. Product sources and the performance gates above are unchanged.
