# Selected context-switch prefix analysis, 8.65

The retained version-5/28-byte context-switch records can now be analysed offline for four selected fields. This does not restore the complete TDH schema, qualify the original full-schema capture, explain the long App flush or establish IDM parity. No new download or elevation request was needed.

## Evidence and explicit inference

Microsoft's [version-pinned PerfView parser](https://github.com/microsoft/perfview/blob/3d89ee8570a943290c4b517663f87493cfbaa35b/src/TraceEvent/Parsers/KernelTraceEventParser.cs) uses prefix offsets for the new/old thread IDs and the old state/wait-reason bytes, with a minimum length check for versions above 2. Applying those offsets to the captured version-5 records is an **inference from that version-tolerant parser**, not a newly discovered complete version-5 TDH definition. [Pinned source identity and selected offsets](validation/scheduling-inventory-8.65/prefix-source.json).

The normal-token TDH probe additionally verifies 48 selected-field reads over twelve known-schema controls: versions 2–4, 24/28-byte payloads, two distinct patterns and the actual capture's header flags. This independently checks the prefix positions through Windows property decoding; it does not make TDH understand version 5. [Control data](validation/scheduling-inventory-8.65/known-prefix-tdh-controls.json), [build identity](validation/scheduling-inventory-8.65/known-prefix-tdh-build.json).

`analyze_cswitch_prefix.py` accepts only the observed Thread-provider/opcode-36/version-5/flags-848/length-28 form. It reads the thread IDs and two state/reason bytes. It does not interpret priorities, flags, processor mapping, wait mode, other prefix fields or the extra trailing bytes. It leaves all original unknown-event counters and failed qualification artifacts intact.

## Validation scope

The analyser requires independent PID/image identity, complete normal-token fixture/native call intervals, zero reported trace losses, actual logger shutdown/absence, bounded exact inventory accounting, ordered in-lifetime raw records and consistent switch-in/switch-out pairing for the measured thread. Each flush needs positive off-CPU overlap. Missing or foreign evidence is rejected; absent ReadyThread notifications do not invent scheduling latency. Extra trailing-byte changes deliberately cannot affect the selected-field analysis.

Forty-four [policy checks](validation/scheduling-inventory-8.65/prefix-policy-checks.json) cover these failure modes. The actual retained capture passes this selected-prefix model: all 6,883 records are accounted for, including 308 records involving the native fixture's issuing thread. [Analysis and input hashes](validation/scheduling-inventory-8.65/selected-prefix-analysis.json).

## Actual small-control observations

| Native flush | Native call duration | Observed off-CPU overlap | Interval duration not classified off-CPU | Overlap after first ReadyThread notifications |
| --- | ---: | ---: | ---: | ---: |
| First | 17.1845 ms | 16.8899 ms | 0.2946 ms | 0.0628 ms |
| Second | 22.0140 ms | 21.7452 ms | 0.2688 ms | 0.0498 ms |

These are two 8 KiB fixture calls, not App download performance trials. Off-CPU overlap is formed from matching switch-out/switch-in intervals clipped to native QPC call boundaries. State/reason values are retained numerically. ReadyThread notifications come from separately TDH-decoded events; the table reports their overlap, not a general estimate of scheduler latency.

The dominant measured intervals are off-CPU, while the measured post-notification overlap is small. This does not identify the disk queue, filesystem or filter work responsible. The remaining interval is not exact CPU time because interrupts and other unmodelled activity can occur without switching the tracked thread. A deschedule state does not classify every instant until the next switch-in. No fields or model are used to waive a durability barrier.

## Next gate and cleanup

Apply the observation model to the actual slow App finishing path before changing product IO policy. The current collector retains all missing context-switch events during the process lifetime: it already recorded 6,883 in a 189 ms fixture lifetime. Its 16,384-record cap must not be raised blindly or ignored for a longer transfer. Prepare a target-thread filter based on the independently checked ID prefix, preserving explicit cap/accounting failures and known thread lifetimes, before a larger trace.

The original strict scheduling/full-schema qualification remains failed; selected-prefix analysis is a separately named result with narrower evidence. The goal still requires actual App/IDM completion and background-work comparisons. Product/native and browser versions remain 8.65 / 0.72.29; no installer, deployment, default-write-path change or release is included. The finished TDH probe's executable/object pair is removed after verification; source, compact controls and the raw diagnostic evidence needed for further analysis remain.
