# Disk/scheduling diagnostic profile preparation, 8.65

The real MFC-app trace identifies a 722 ms final native body-flush interval in one D: split sample. The earlier full kernel collector remains unqualified because target FileIo opcode 84/version 3 events lack TDH metadata. No decoder is guessed, registered or substituted, and the full-capture rejection gate remains intact.

`benchmarks/speed/finish_scheduling_lifetime.cpp` now selects a separate compile-time profile of the existing lifetime collector. It enables Process, Thread, DiskIO, DiskIOInit, CSwitch, Dispatcher and NoSysConfig, excluding FileIo and FileIoInit. It uses a distinct session name, retains the same deadlines, buffer/callback/report bounds, normal-controller provenance, owned-root validation and logger cleanup. Every report declares its exact profile, enabled flags and whether file-operation attribution is supported. Reports additionally expose a count of all events missing metadata, including events outside the target process header.

This profile is intended to answer a narrower diagnostic question alongside independently measured native flush intervals: whether the issuing thread is waiting versus runnable and whether decoded target-associated disk activity overlaps. It cannot map arbitrary filesystem operations to the body file, attribute system-worker IO, identify filter-driver causes or prove complete FileIo coverage. It is an additional measurement route, not a replacement for the final performance/parity requirements or a reason to remove durability barriers.

The independent `validate_scheduling_lifetime.py` requires the exact provider profile, zero unknown-metadata/lost events, bounded buffers, successful logger shutdown and absence, a single matching fresh process lifetime, two successful normal-token native fixture flush calls inside that lifetime, and positive disk/context-switch evidence. At least one context switch must involve a measured native flush's issuing thread during that call; unrelated process scheduling is insufficient. It rejects unexpected FileIo flush pairs and out-of-lifetime events. The full `validate_realtime_lifetime.py` still requires its two correctly attributed FileIo flush pairs; its rules are unchanged.

Both collector variants compile with the current toolchain. Both refuse an unelevated, argumentless invocation with exit 77. The normal-token 8 KiB fixture also builds. The focused validator passes 26 offline policy checks; the existing full validator passes 34. These checks qualify the preparation and policy behavior only. **One restricted eight-hour worker elevation request has been opened. No ready record or live capture exists at this checkpoint; neither profile is newly live-qualified.** Do not launch another request because observation times out. Inspect the original launch and worker lifecycle records first. [Build identities and policy checks](validation/scheduling-profile-8.65/build-and-policy.json).

The checkpoint above is historical: the original request subsequently succeeded. See the live qualification below; its failure does not invalidate successful worker transport.

Next, launch only the focused 8 KiB fresh-process qualification through a hash-pinned temporary worker. Inspect real stop/absence, loss, schema, process, scheduling and disk records before using it around a 64 MiB App transfer. Failed qualification remains a failure; do not run the larger workload or infer a cause. Any live fixture must retain exact payload validation and scoped cleanup. Product/native and browser versions, installed files, ordered-write activation and release status are unchanged.

## Independent REA follow-up while approval is pending

Read-only REA/Ghidra analysis classified the three remaining direct FlushFileBuffers import references from the 8.33 byte-verified inventory. The installed binary still matches SHA-256 `03CC62E9ADB77A380F9DC12F67CCAAEE5106F12844AA73CE32C914DDD16D607C`.

| Direct call preferred VA | Observation |
| --- | --- |
| `0x456025` | Enclosing `0x455d10` opens/maps an existing file, operates on mapped contents, unmaps, flushes the handle, conditionally updates timestamps and closes it. The purpose and download-path reachability remain unverified. |
| `0x61e784` | REA labels the enclosing procedure as MFC `CFile::Flush`; the observed flow guards the handle and sends a failed flush through OS-error exception handling. This is a decompiler library-match label, not an independent source match. |
| `0x655132` | REA labels the enclosing procedure as CRT `__commit`; the observed flow validates/locks a descriptor, obtains its OS handle, flushes and maps errors. The same library-label limitation applies. |

The previously examined `0x570294` reference belongs to the conditional application wrapper described in the 8.44 audit. Framework/CRT entry points mean a static caller count cannot establish all completion/background flush behavior. The owned-path native observer remains useful across those entry points, subject to its recorded scope limitations. No new UDM pacing rule, flush suppression or durability change follows from these static results.

[Compact facts and REA evidence identifiers](validation/scheduling-profile-8.65/idm-remaining-flush-sites.json) retain receipt hashes without publishing proprietary pseudocode. The binary was never executed or changed by this audit. REA closed the binary successfully, and the owned MCP/Java processes exited. The existing scratch area was reused; no additional download payload was created. At that audit checkpoint the original elevation request was pending and had not been replaced; its later success is recorded below.

## Original approval succeeded; live qualification rejected

When the user requested a retry, inspection found the original worker already running, with its ready record, successful launch result and matching live PID/start time. No duplicate prompt was opened. Its approval window ends at `2026-11-18T21:50:35Z`; it remains restricted to the fixed hash-pinned collector rather than arbitrary administrator commands.

The existing worker launched exactly one normal-token 8 KiB fixture capture. The fixture exited normally and its bytes matched all 8,192 expected zero bytes. The collector stopped successfully, independently confirmed its logger absent, and reported zero lost events/buffers. The payload and generated fixture executable copy were deleted.

The independent scheduling gate **rejected** the capture: 86,672 events lacked TDH metadata, no usable CSwitch event was captured, and no context-switch overlap with a measured native flush could be established. ReadyThread and target-associated disk events were observed, but cannot substitute for missing context-switch evidence. Unknown events outside the target process header still invalidate this profile's gate. There is no new flush-cause claim or live qualification pass, and the larger App workload was not started.

[Outcome and scope](validation/scheduling-profile-8.65/live-qualification.json), [redacted trace](validation/scheduling-profile-8.65/fixture-3c908702-trace.json), [native fixture intervals](validation/scheduling-profile-8.65/fixture-3c908702-native.json), [independent rejection](validation/scheduling-profile-8.65/fixture-3c908702-validation.json), [payload/copy cleanup](validation/scheduling-profile-8.65/fixture-3c908702-cleanup.json). Kernel IRP addresses in the public trace are replaced with stable opaque tokens; the private trace hash is retained.

Next repair the schema-observation path and obtain authoritative scheduling schemas before repeating this small gate. A changed collector requires separate approval under the fixed-tool worker policy. Do not replace it silently, relax the gate, run 64 MiB, or infer a cause from this failed capture.

The subsequent [bounded missing-schema inventory](scheduling-inventory-8.65.md) identifies the actual missing Thread/CSwitch version-5, 28-byte records and retains them privately for offline analysis. Inventory acquisition succeeds; this earlier qualification failure remains intact.
