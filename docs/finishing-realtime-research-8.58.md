# Direct realtime finishing-trace research (source 8.58)

This is diagnostic tooling work. Native source remains **8.58**, both extension source manifests remain **0.72.22**, and installed UDM remains **8.32**. All 516 qualified source hashes, 42 core objects, five products and three protected personal/installed files remain unchanged. No installer, installation, push or release was performed.

## Why another capture path

The [8.53 finishing observations](finishing-research-8.53.md) localized a D: split tail to the final flush syscall, while explicitly leaving disk, filesystem/filter and scheduler attribution unresolved. The two subsequent WPR captures timed out during stop/export and decoded only one event each. Their collectors were eventually confirmed off. Repeating that export strategy is not evidence of the underlying cause.

The new [probe](../benchmarks/speed/finish_realtime_probe.cpp) tests direct realtime ETW delivery without WPR, ETL export or merge. Microsoft documents realtime consumption through `OpenTrace`/`ProcessTrace`, with the session logger name supplied to the consumer. [Consuming Events](https://learn.microsoft.com/en-us/windows/win32/etw/consuming-events). Session creation and system-logger permissions are documented separately. [StartTraceW](https://learn.microsoft.com/en-us/windows/win32/api/evntrace/nf-evntrace-starttracew).

## Current implementation

The administrator mode first starts a delivery-only session for this probe's own provider. It emits sequences 1, 2 and 3 with a fixed sentinel, decodes their TDH properties and verifies exact delivery plus session shutdown. Only a passing delivery control proceeds to a separate system-logger session.

The kernel feasibility phase creates one fresh owned file, writes 4 KiB twice and calls the real file-flush API after each write. This first gate uses the elevated collector process itself as the writer. It must not be treated as normal-token UDM behavior or a full target-process capture. The reader maps only files under the owned fixture, pairs flush/operation-end events, checks successful status and rejects ambiguous or unfinished pairs. A direct session query must confirm absence after stop. Callback failures, lost events/buffers, schema failures and reader capacity limits reject acceptance.

Sessions use fixed descriptive names. A preexisting name/GUID is refused; the helper never adopts or cancels it. The kernel session requests at most 256 buffers of 64 KiB; actual buffer counts are reported. Each phase waits up to four seconds for its small delivery target and accepts only an observed elapsed duration of at most fifteen seconds. Stop and consumer closure are synchronous OS calls: this is an acceptance deadline, not a guarantee that a malfunctioning API can be forcibly interrupted. A still-running helper must remain supervised until its actual session state is known.

Kernel events are delivered system-wide, but the report retains owned relative paths, target task counts and schema metadata rather than raw system events or unrelated filenames. No raw ETL is saved. The old offline reader remains byte-identical and is reused by inclusion; its full-lifetime coverage gate remains distinct from this feasibility gate.

## Verified before elevation

- The helper builds against the pinned Windows toolchain. [Build hashes](validation/finish-realtime-8.58/build-result.json).
- All **20 existing normalized reader safety checks** pass. These are synthetic policy checks, not live kernel-decoder validation. [Checks](validation/finish-realtime-8.58/reader-self-tests.json).
- The explicit access check succeeds without starting a trace. A nonadministrator request for administrator mode returns 77 with `sessionStarted=false`. [Access check](validation/finish-realtime-8.58/access-check.json), [refusal](validation/finish-realtime-8.58/unelevated-refusal.json).
- An initial process-private realtime configuration was rejected with Windows status 87 before starting a session. That option was removed from the maintained helper. The rejected result is retained separately; it is not a successful delivery test. [Rejected configuration](validation/finish-realtime-8.58/rejected-private-mode.json).

## Pending Windows approval

At the recorded checkpoint, the fresh elevation request was still waiting. Windows exposed a consent process in the current session; no elevated helper PID or administrator-probe report had appeared. The existing launch is being polled rather than restarted. [Checkpoint](validation/finish-realtime-8.58/status.json).

The administrator run has not yet established event delivery, kernel decoding, a stopped kernel session or a causal trace of the slow flush. No UDM workload, installed app or IDM process was modified or stopped. Existing processes in other Windows sessions are left alone. The helper binary and necessary evidence are retained while the request is pending; it must not be replaced or deleted during approval.

## Independent acceptance checks and account-profile repair

The new [read-only validator](../benchmarks/speed/validate_realtime_probe.py) adds a second acceptance boundary. It checks the independently observed helper PID, elevated self-process scope, exact delivery sequence, API statuses, event/buffer loss, actual buffer allocation, elapsed deadline and stopped-session query. Each of exactly two successful owned-file event intervals must lie inside its corresponding real native flush-call interval, with consistent issuing-thread identity. It separately verifies the 8 KiB zero payload by length and SHA-256. Actual report reads are bounded to 1 MiB plus one rejection byte; payload reads are bounded too.

All **42 synthetic policy checks** pass. They include malformed/missing records, wrong PID/thread/path, events outside native call intervals, lost buffers, invalid types/counters, failed calls, buffer/deadline excess, unsupported scope claims and wrong payload bytes. Both existing real non-capture artifacts are rejected. These results validate rejection policy, not live kernel decoding. [Checks](validation/finish-realtime-8.58/validator-self-tests.json), [artifact rejections](validation/finish-realtime-8.58/validator-negative-artifacts.json).

A path-preflight comparison also identified an account dependency: the original helper derives its allowed folder from the elevated process's LOCALAPPDATA. This Windows session's token contains neither the Administrators nor Performance Log Users group. If a different administrator's profile is used for UAC, the original guard can reject the intended developer folder. A controlled access check reproduces the dependency by changing only LOCALAPPDATA; it exits 2 without creating its fixture or starting a session.

A replacement is **staged separately**, leaving the pending executable and repository probe source unchanged. It derives the allowed fixture directory from its own canonical executable location and permits only a fresh direct child of that development directory. Four actual preflight cases pass: ordinary profile, different LOCALAPPDATA, nonadministrator kernel refusal and rejection of a foreign fixture root. Its twenty reader safety checks pass too. No second elevation request has been opened. Use the existing launch's real outcome first; if it fails, the staged replacement is ready without modifying the original request in flight. [Staged build](validation/finish-realtime-8.58/staged-ownerroot-build.json), [profile comparison](validation/finish-realtime-8.58/owner-profile-comparison.json), [reviewable root-guard patch](../benchmarks/speed/experiments/realtime-owner-root-fix-8.58.patch).

Two obsolete tool files, including a compiler object, were individually hash/link/reparse checked and removed: **4,778,472 logical bytes**. The staged helper's compiler object was also retired after hash verification: 4,769,841 additional logical bytes. Both helper executables remain unchanged; no payload or active-session file was deleted. The rejected private-mode source can be reconstructed from a bidirectional tool delta. [Cleanup](validation/finish-realtime-8.58/tool-cleanup.json), [retirement proof](validation/finish-realtime-8.58/tool-retirement-proof.json).

One current-session development UDM process was observed without altering it. It used zero measured CPU time in a short sample; its on-disk image version is 8.39. This is not loaded-image identity, a controlled benchmark or evidence about disk/filter/scheduler causality. No existing app needs to be forced closed for the small feasibility probe. [Observation](validation/finish-realtime-8.58/existing-app-observation.json).

## Next gates

1. Observe the existing launch to a real outcome. Windows approval belongs to the user. If it is canceled or fails, retain the exact result; a polling timeout alone is not terminal.
2. Inspect the delivery control and actual stopped-session query. If the small control fails, repair the capture path before running a download attribution workload.
3. Inspect both owned flush pairs, native call intervals, schema fields and loss counters. A passing self-process feasibility test still does not prove normal-token process lifetime coverage or a kernel cause.
4. Prepare a fresh normal-token workload whose PID/image, thread identities and complete lifetime are captured. Correlate actual file identity and IO intervals. Validate any additional scheduling/stack providers separately before attributing the D: split wait.
5. Only then repeat a bounded real case and the ordinary uninstrumented comparison, retaining slower outcomes and the symmetric post-exit output flush. Preserve the final barrier after successful writes and metadata changes. Delete owned payloads after retaining compact evidence and confirming process/session termination.

The original finishing delay, kernel cause and broad IDM parity goal remain unresolved. This research changes the available diagnostic path, not UDM's completion or durability behavior.

## Historical blocked checkpoint (superseded by resumed captures)

After three consecutive goal turns, the same Windows approval request still has a live waiting launcher and consent process in session 2. No probe process or administrator report exists. The goal is blocked on this external action; the broad parity objective is not complete. Capture preparation, the independent validator and the staged account-independent replacement are ready. Resume by observing the original launch, not by opening a duplicate request. [Authoritative checkpoint and resume action](validation/finish-realtime-8.58/blocked-audit.json).


## Resumed capture results and shared elevation session

The original pending launch ended without an administrator report. The corrected owner-root helper was subsequently approved and ran. Its delivery control passed, but the first kernel probe recorded 174 undecodable target events and was rejected. A retained baseline/focused experiment reproduced schema failures in the baseline; the focused NO_SYSCONFIG self-process probe passed the independent validator, including two successful owned flushes inside their measured native call intervals, exact 8 KiB payload integrity, zero losses and confirmed logger shutdown. This is a feasibility result, not attribution of the D: finishing delay.

Three fresh unelevated process fixtures then captured one matching process start and end, two successful owned flush pairs, no unmatched flushes and zero reported event/buffer loss. All three were **rejected** because 26 target events lacked TDH metadata. The inventory identifies FileIo provider `90cbdc39-4a3e-11d1-84f4-0000f80464e3`, opcode 84, version 3, 40-byte payload, TDH status 1168. The local WMI classes did not supply that event type. No undocumented payload layout is assumed. The precondition prevented the 64 MiB UDM workload from running. No new installed UDM/IDM timing result or kernel cause is claimed.

At the user's request, a temporary elevated diagnostic worker now services repeated trace requests after one Windows approval. It pins the existing collector's SHA-256, validates owned roots and controller account, serializes traces, refuses arbitrary commands and accepts at most 20 requests. It expires after 30 minutes when idle; an active collector retains responsibility for its own safe stop. There is no service, scheduled task, stored password or UAC policy change. A live unsupported-command request was refused and a real trace launched through the worker without an additional prompt. Product changes, installation and new arbitrary tools are outside this worker's scope. See [worker operation](diagnostic-session-worker.md).

All stopped fixtures' 8 KiB payloads and six temporary target executable copies were individually bounded, hash/link/reparse checked and removed: 13 files, 37,222,400 logical bytes. Compact reports, qualified products and the active worker remain. [Sanitized results](validation/finish-realtime-8.58/live-capture-followup.json), [cleanup receipt](validation/finish-realtime-8.58/live-capture-cleanup.json).

Next: obtain an authoritative schema for the opcode-84 event or separately establish a defensible, explicitly narrower observation model. Keep the current rejection gate intact until then. Only after the normal-process gate passes should a bounded real download run, followed by an uninstrumented UDM/IDM comparison and separate post-exit durability measurements. These diagnostic changes leave product versions at native 8.58 / extension 0.72.22; no installer was created or installed.


## Independent normal-lifetime interval validation

A separate Python validator now passes 34 policy checks for loss, lifetime, identity, normal-token provenance, callback limits, session shutdown and exact native/ETW flush containment. Applying it to the three completed normal fixtures rejects all three for their undecoded events, as required. Independently, each of the six decoded flush intervals is inside its successful native FlushFileBuffers call, on the same issuing thread, for the exact owned `probe.data` file and within the observed process lifetime. The native call durations were 36.121/33.209 ms, 37.9363/20.0698 ms and 29.6597/26.0801 ms. These are instrumented 8 KiB D: fixtures, not UDM/IDM performance trials, a controlled volume comparison or proof about the 64 MiB stall. Payloads were hash-checked before the separately recorded cleanup; the interval validator does not claim to reverify deleted payloads.

The installed Windows build is 26100. Its FileIo WMI classes cover opcodes through the documented operation/path families but contain no opcode 84 event. The checked public [Microsoft PerfView MOF](https://github.com/microsoft/perfview/blob/main/src/TraceEvent/Parsers/kerneltraceeventparser.mof) also provides no such declaration. That negative lookup does not identify the event. No registry/WMI schema registration or guessed decoder was installed.

[Independent results and native call records](validation/finish-realtime-8.58/normal-lifetime-independent-validation.json), [qualified-product preservation verification](validation/finish-realtime-8.58/normal-lifetime-followup-verification.json). Native 8.58 and both extension manifests remain unchanged. All 516 frozen source, 42 core object, five production product and three protected personal/installed file hashes still match the qualified build.


## Read-only storage context

WMI disk/partition associations map C: to physical disk 0 (KINGSTON SKC600/256G) and D: to disk 1 (WDC WD2500AAKX-753CA1). D: is a Logical Disk Manager data volume with 128 KiB NTFS allocation units; C: uses 4 KiB units and Windows reports its medium as SSD. The simpler Get-Disk/Get-Partition listing omitted D:, so it was not used to infer that the second disk was absent. The WMI physical-device medium type for the WDC device is Unspecified.

The documented seek-penalty query succeeds for C:/physical disk 0 and reports no seek penalty. D: and physical disk 1 reject that query with errors 1 and 31 respectively; their seek capability remains unknown through this interface. All queries used zero desired access, wrote no payload and changed no cache/durability policy. This context is not a causal explanation of an individual flush, nor an explanation of the difference between two applications measured on the same volume. Retain same-volume, symmetric background-work measurements.

[Reproducible read-only probe](../benchmarks/speed/inspect_finish_storage.py), [sanitized storage context](validation/finish-realtime-8.58/storage-context.json), [Microsoft descriptor definition](https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ns-winioctl-device_seek_penalty_descriptor).


## Parallel observation route

The existing user-mode observer's unrelated-path retention and indiscriminate pathname queries were corrected and verified with real owned C:/D: fixtures. A hook/teardown shutdown failure was reproduced separately from successful empty-script/unobserved controls; retaining the script until target normal exit qualified the short-process route. This does not cure or waive the kernel opcode-84 gate. See [native observer evidence and exact limitations](native-io-observer-8.58.md).


## Actual backend follow-up

A controlled native entry point now provides a snapshot boundary after qualified core shutdown/destruction. The real 64 MiB split case reproduced a 564 ms native final flush and verified exact observed write intervals, hash and normal exit. Both sparse experiments lost the whole-backend promotion gate and were retired; see [all measurements and retained failures](finishing-flush-attribution-8.58.md). This neither repairs the kernel opcode-84 decoder nor proves installed IDM parity. The elevated worker has expired normally; no replacement request was opened.
