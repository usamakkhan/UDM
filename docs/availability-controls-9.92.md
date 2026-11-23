# Availability follow-up on 9.92: retained losses and rejected controls

November 22, 2026. Native remains **9.92**, source Chromium/Firefox remain **0.73.56**. This is research and comparison tooling, not a new application improvement, installation or release. Normal products are unchanged. The earlier [completion claim fix](completion-claim-9.92.md) remains qualified within its stated scope; universal IDM parity is **not** established.

## What was measured

Five groups contain **57 complete 4 MiB outputs** (228 MiB of saved payload, not a measurement of physical device writes). Every output has the expected length and SHA-256. UDM's synthetic saved catalogs report Complete and the expected digest. Tracked application processes exit; IDM's temporary connection settings are restored and checked. Other-session programs are left alone. IDM launcher code 1 is retained despite correct files; its semantics remain unestablished.

All comparisons retain total process time, last origin send to exclusive file availability, process exit, and a symmetric explicit output flush after exit. They include observed app-owned shutdown work, not every system service, actual GUI paint or power-loss durability. No compiler runs during comparisons. Medians never override losing trials. A retained harness `terminal.json` with `passed: true` means transfer/integrity checks completed; its median summary is not the all-endpoint parity gate.

| Group | Trials / outputs | Result |
| --- | ---: | --- |
| Normal 9.92 / frozen normal 9.91 / IDM, C: cache and output | 3 / 9 | 9.92 loses availability to IDM in two trials and exit tail in one; total and total-through-flush win all three |
| Same private binary, serialize body flush before intent versus original overlap, plus IDM | 5 / 15 | Serialization loses availability in 4/5, total in 4/5, exit tail in 5/5, total-through-flush in 4/5 and flush tail in 5/5; rejected |
| Same private binary, journal-only write-through versus original buffered journal, plus IDM | 5 / 15 | Write-through loses availability in 3/5 and total in 4/5; rejected |
| Normal 9.92 / normal 9.91 / IDM, C: catalog/cache and D: output | 3 / 9 | 9.92 beats IDM total, exit tail and total-through-flush all three; loses availability in trial 3 |
| Same normal 9.92 binary, existing temporary-folder preference on D: versus default C:, plus IDM | 3 / 9 | D: placement exercises hard-link staging but loses all five endpoints in trial 3; automatic placement change rejected |

The first C: 9.92 availability tails are **35.834, 405.991 and 28.488 ms**, versus IDM **20.277, 21.219 and 35.404 ms**. The slow second UDM trial records body flush **188.838 ms** and intent temporary-file flush **250.911 ms**, inside a **253.709 ms** intent operation. Directory creation is **1.454 ms**, temporary open **0.243 ms**, byte write **0.104 ms**, and intent publish **0.992 ms**. Body and intent overlap, so their durations must not be summed. The slow interval is within flush calls; the responsible disk queue, filesystem/filter or scheduling component is not yet established. Frozen 9.91 also has large stalls, so these samples do not attribute them to the UI claim change.

Write-through does not remove that work. In its third trial the journal write takes **118.79 ms** versus buffered **0.10 ms**; its flush falls to **58.28 ms** versus **113.08 ms**, while the complete intent rises to **180.12 ms** versus **145.44 ms**. Comparing only the last flush would hide a regression. Microsoft's [file caching](https://learn.microsoft.com/en-us/windows/win32/fileio/file-caching) and [FlushFileBuffers](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-flushfilebuffers) contracts explain why write and flush endpoints both matter; they do not diagnose this machine's particular pauses.

## Storage ownership matters

C: and D: are separate physical devices: Kingston SKC600 SSD and WDC WD2500AAKX disk respectively. D:'s association was obtained through Win32 logical-disk/partition relationships, not inferred from an absent modern storage-provider entry. This is an observation, not a storage-health finding.

The normal mixed-volume run proves UDM uses C: profiles with D: output and `write-buffer` staging. Its 9.92 availability tails are **44.083, 39.156 and 119.753 ms**, versus IDM **192.685, 57.264 and 42.336 ms**. Trial 3 remains a **77.417 ms availability loss**. UDM total-through-flush times are **1.176, 1.032 and 1.040 s**, versus IDM **4.099, 2.616 and 2.239 s**. Those total wins do not erase the availability loss.

The existing TemporaryFolder preference already persists a per-job parts folder. The isolated placement control observes D: parts and `hard-link` staging for the proposal, C: parts and `write-buffer` for the default. In trial 3 proposed availability is **584.918 ms**, versus default **303.348 ms**. No real user preference is changed. IDM's configured TempPath points to C:; actual runtime IDM temporary-file ownership has **not** been traced.

## Harness amendment and provenance

`benchmarks/speed/compare-completion.py --catalog-root <fresh-absolute-folder>` separates synthetic UDM profiles from outputs. It rejects existing, equal or nested roots before launching applications or changing IDM settings. Defaults retain the earlier layout. Each result records the actual catalog and destination drives. Five read-only layout tests and six barrier-order analyzer tests pass; a real CLI negative preflight additionally rejects an existing profile root without creating output.

The first mixed-volume attempt stopped before application launch/settings changes because a catalog-root initializer was missing. Its failure receipt and original inputs are retained. The corrected runner was captured before the successful run. The exact pre-amendment runner is retained alongside the new source: this is the sole amendment to the qualified 9.92 application build-input inventory, not an unnoticed rebuild.

Private controls recompile only Duplicates.cpp or Core.cpp and link the qualified normal objects. Their mode markers prove the requested branch ran. They keep explicit flushes, file identity pins, acceptance and recovery barriers. Their source snapshots and input hashes are separate from normal products. No diagnostic switch enters production. In particular, the journal control's parent-folder-name selector is a diagnostic device, not a proposed general storage API.

## Next implementation gate

1. Establish IDM's actual open/write/flush path with local static analysis and a valid bounded runtime trace. Static imports alone cannot identify which runtime file role uses a call. A timeout is not evidence of missing code.
2. Attribute UDM's slow flushes by file role and volume before another scheduling/default change. Preserve the full source-operation timeline and final background work.
3. Admit only a bounded change with repeated alternating normal-product results at every endpoint. Repeat 64 MiB sequential/split and installed/browser/UI checks when storage permits; these 4 MiB controls do not satisfy those gates.
4. Continue the independent completion UI backlog: countdown model ticks/eligibility waits, automatic folder/shell work and secondary-window modal ownership. At this 9.92 baseline both countdown pulses called `manager.tick()` synchronously and file action eligibility blocked on the model. The subsequent 9.93/9.94 admission/continuation fixes and 9.95 completion wake are now qualified separately; shell and secondary-modal work remain open. Do not relabel those as completed by the armed-claim fix.

Preparing today's completed-publication journal before network EOF is unsafe: its source pin excludes writers and includes mutable last-write identity, and recovery can publish the recorded staging source. A future transfer-time intent needs a distinct pending format ignored by old recovery and an immutable EOF proof. No early-intent redesign is introduced here.

## Read-only REA attempt

REA 6.5.0 opens the exact installed IDM binary used by the comparisons; its SHA-256 remains unchanged. Existing local Ghidra 12.1.3 hits its **330,000 ms startup deadline** on the initial procedure search and two narrower caller queries. No caller/flag result is obtained, and no running IDM instance is instrumented. The final queued request is withdrawn to avoid a fourth identical initialization; the private orchestrator reports the missing queued file and its finally block closes the binary and server. This failed analysis is retained as failure, not a pass. No matching owned provider process remains at the final check.

The installed REA source exposes `REA_GHIDRA_STARTUP_TIMEOUT_MS`; a future bounded session can increase that child-process budget without changing provider code or global configuration. That retry still needs time/storage limits. The current attempt does not establish IDM's open flags, flush behavior or durability contract. [Attempt and closure receipt](validation/availability-controls-9.92/rea-static-attempt.json).

[Sanitized complete samples, losing trials, control inputs, source snapshots and export hashes](validation/availability-controls-9.92/). Synthetic catalog contents and payload files are excluded from tracked evidence. The host installation remains 8.68 / 0.72.32 and the lab remains off; no new approval request or installation occurs in this research follow-up.

Temporary files remain: automatic approval review rejected the earlier cleanup, including the narrower payload-only action, with **“blocked by policy.”** No alternative deletion route is used and no cleanup is claimed.

## Subsequent writeback investigation on 9.95

[Fresh REA analysis and qualified runtime part traces](idm-writeback-contract-9.95.md) now establish cached IDM part opens and a real periodic flush during a slow transfer. The observed part flow has no additional end-of-part flush in these samples. Whole-app/kernel/device behavior and UDM’s actual long flush cause remain unresolved; none of the rejected controls above is promoted.
