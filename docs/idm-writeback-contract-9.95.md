# IDM writeback investigation on the 9.95 baseline

November 22, 2026. **Research only:** UDM remains **9.95**, both source browser manifests **0.73.59**. No production storage change, version increment, installation or release is admitted by this investigation.

## What is now established

The current installed IDM binary matches SHA-256 `03CC62E9ADB77A380F9DC12F67CCAAEE5106F12844AA73CE32C914DDD16D607C`, also matching the retained September analysis. Its recognized file-part path creates a synchronous read/write file with read sharing, `CREATE_ALWAYS`, and **flags 0**. Independent parsing of the actual PE import table and source bytes verifies the create arguments, import call and part-flush helper's handle offset. Fresh REA/Ghidra queries agree with that retained analysis.

Flags 0 leave the normal Windows cache policy enabled; they do not request write-through or no-buffering. Cached writes and a successful close are distinct from an explicit application flush. These are Windows API contracts, not an inference about this disk's physical completion time. [Microsoft file caching](https://learn.microsoft.com/en-us/windows/win32/fileio/file-caching), [CreateFileW](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew), [FlushFileBuffers](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-flushfilebuffers).

The observed application part-flush helper is at `0x570290`, reached through the part-list traversal at `0x55ac20` and a rate-update path at `0x5bb350`. That branch uses an elapsed-window field compared with 40,000 and a computed rate field compared with 512,000. The clock-field construction uses the runtime's seconds/milliseconds time routine. Those units/field roles are inferred from static code; the slow runtime probe below independently exercises the branch. The numeric thresholds are **not copied into UDM**.

There are four recognized direct `FlushFileBuffers` callers: the file-part helper, an unrelated application path, MFC's `CFile::Flush`, and CRT commit support. This is a direct-call inventory, not proof that every indirect or kernel path is understood. The file-part close helper directly closes its handle and performs decoder cleanup; it contains no direct flush call. That static observation alone was insufficient, so runtime collection was required.

## Working REA session and observer qualification

REA 6.5.0's default 330-second Ghidra startup budget was the earlier obstacle. A child-only **900-second** allowance permits default Ghidra 12.1.3 analysis to finish, followed by eight bounded read-only queries. No analyzers are disabled, provider files or global settings are changed, or existing IDM instance is attached. The session closes normally and the source binary remains unchanged. Ghidra 12.1.3 is accepted by REA; its bridge reports verification against 12.1.4, retained as a tool limitation.

The runtime observer uses REA's local Frida spawn workflow. It preserves native arguments/results and records only the owned filename scope plus one candidate part opened by the fresh test process. Opaque cache filenames do not contain the requested output token: the first IDM capture therefore had correct output bytes but **inadequate body coverage**. Its integrity receipt is retained with a separate rejected coverage assessment; zero captured flushes in that attempt prove nothing.

The corrected observer provisionally follows the first `DwnlData` part in this owned process, then requires a successful move from that same part to the **exact token-named output**, full successful body-write totals, matching native/Win32 write envelopes, a successful writer close, and the correct final SHA-256. Without that association its opaque-path adoption is a hypothesis, not acceptance. It is a bounded single-part diagnostic, not a general-purpose personal-download observer.

An owned x64 8 KiB writer positively qualifies create/write/flush/close collection, including the nested native write and flush. Its first invocation passed the executable twice because REA prepends it to `argv`; the helper rejected the invalid argument count without creating output. That attempt is unqualified and retained. Corrected and later updated controls both produce the exact expected bytes and six I/O events. The slow IDM probe additionally provides the 32-bit flush positive observation.

## Actual IDM transfers

All qualified runtime probes use a fresh current-session IDM spawn and a deterministic loopback origin. The two connection settings are restored and verified after each run. The tracked target exits, all expected hashes match, captures have no script errors or truncation, and the binary remains unchanged. An already-running IDM process in another Windows session is neither attached nor stopped and retains its identity. The inspector does not record file contents or browser traffic.

| Qualified IDM probe | Actual part/output volumes | Successful body writes, each with native envelope | Explicit flush observed in the writer session |
| --- | --- | ---: | --- |
| 8 KiB | C: / D: | 1 | None |
| 4 MiB | C: / C: | 368 | None |
| 1 MiB at 20,000 bytes/s | C: / C: | 92 | One, while transfer continued |

In the slow probe, the explicit flush occurs **42.336 seconds after part open**. The final write ends at **52.434 seconds**, about 10.1 seconds later. The native flush, Win32 wrapper and file-part helper are three nested views of **one operation**, not three flushes. No additional explicit writer-session flush is observed after the final write. This confirms that the observer can see the 32-bit flush path and that, in these probes, the part-writing flow does not impose UDM's mandatory final body-flush barrier.

The investigator's separate output flush after observed process exit takes approximately **30.015 ms**, **0.948 ms** and **1.728 ms** respectively, including open/close overhead. These instrumented single observations are not comparative speed scores. A small post-exit flush does not prove prior absence/presence of physical writes; Windows may already have drained cache data.

## What this changes in the plan

UDM's earlier observed finishing intervals include a real final body flush and durable publication intent/catalog work. The investigated IDM path uses cached writes, ordinary part close and a move, with a conditional periodic flush during the longer transfer. This is evidence of a **different observed completion policy**, not evidence that UDM can safely remove its barriers or that this explains every 64 MiB stall.

1. Keep visible availability, observed application exit and symmetric explicit output-flush endpoints separate. Retain UDM's losing availability samples; do not relabel them wins.
2. Preserve UDM's durable body/publication/recovery contract. Do not emulate cached close by reporting durable completion early or removing a final flush.
3. Attribute UDM's long body/intent flushes with a qualified file-role/kernel timeline before changing storage ownership. The disk queue/filesystem/filter/scheduling cause remains unresolved.
4. Evaluate any overlap or storage-layout design against complete normal-product lifetimes, crash recovery and both physical volumes. Earlier barrier-order, journal write-through and destination-cache regressions remain rejected.
5. Repeat split and larger installed comparisons when storage permits. These are single-part diagnostic traces, not a new normal UDM/IDM timing matrix, installed acceptance or full parity proof.

The observer does not exhaustively trace native opens inside OS move/copy, mappings, indirect code or kernel/device writeback. The same-volume 4 MiB case avoids relying on the cross-volume move's hidden copy implementation, but still does not establish the whole application's durability policy. No claim that IDM “never flushes,” that all background work is measured, or that UDM is universally faster is supported.

## Evidence and preservation

`benchmarks/speed/analyze_idm_io.py` replays the bounded capture checks, refuses missing native envelopes, unscoped parts, mismatched output association, truncation and wrong process/source identity, and counts native bytes as validation rather than another copy of the body. **Eight read-only verifier tests pass**, including a real-positive-shaped periodic-flush case and corrupted captures that must not establish flush absence.

[Sanitized probe records, qualified and rejected attempts, static-verification facts, timing scope and source snapshots](validation/idm-writeback-contract-9.95/). Proprietary decompiler output and the IDM executable remain in the existing local research area; they are not copied into tracked UDM code or documentation.

UDM's five 9.95 native programs and installed program/manifest/tool identities are preserved. There is no new installer, installation, push, release or lab start. The four IDM probes download about **5.016 MiB** total; two positive-control writers create another 16 KiB. These logical payload totals are not physical write/wear measurements. Temporary files remain because the earlier automatic approval review rejected cleanup with “blocked by policy”; no alternative deletion route is used.
