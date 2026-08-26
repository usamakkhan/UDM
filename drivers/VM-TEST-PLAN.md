# WFP monitor validation plan

Status: **not executed**. Compilation, static analysis, INF validation and user-mode tests passed; they do not exercise kernel callbacks. Use a disposable Windows x64 test VM with a recovery snapshot, debugger and the appropriate development signing setup. Preserve the production host's security settings.

Record OS/kit version, driver SHA-256, signature, verifier settings, application PIDs/start times, traffic fixture sizes and outcomes. Use local fixture servers and generated files.

| Area | Required experiment | Acceptance |
|---|---|---|
| Package | Validate INF, produce/sign catalog through chosen signing process, verify before loading | Correct x64 binary/hash; installation failure leaves no partial service |
| Initialization | Repeated load/unload; fail device, BFE session, transactions, each registration and symbolic link | Complete rollback; no residual filter, handle, device or callback |
| Device access | Normal user, elevated client, SYSTEM, second client, device subpath | Correct identities; exclusive handle; no subpath bypass |
| IOCTLs | Truncated/oversized buffers; unknown version/code; count 33; reserved bits; PIDs 0–4; empty watch | Correct failure, no out-of-bounds access, empty watch stops collection |
| TCP | New scoped IPv4/IPv6 connections, known bytes both ways, close/reset | Correct attribution/endpoints/direction; continued transfer; coherent lifecycle |
| UDP/QUIC | Scoped IPv4/IPv6 UDP echo and controlled QUIC session | Flow association/direction work; document indicated-byte accounting; no plaintext claim |
| Scope | Selected/unselected processes, preexisting flows, new browser process, PID reuse | Unselected traffic excluded; stale identity removed; limitations visible |
| Capacity | Over 128 simultaneous flows; closed-row reuse; large counters; rapid watch changes | Bounded records; visible loss; no duplicate IDs/counter corruption |
| Concurrency | Client exit during IOCTL/traffic, process death, flow-delete race, stop during classify | No crash, stale callback, use-after-free, deadlock or leak |
| System changes | Sleep/resume, VPN/proxy, IPv6 link-local, network switch, BFE restart, other WFP filters | Traffic passes; restart behavior documented; no interference |
| Recovery | Crash dump, verifier failures, failed update, uninstall/reboot | Diagnosable failures; snapshot recovery; clean removal |

Run relevant Driver Verifier checks in the disposable VM and fix failures before any production installation. Review callback teardown against [flow-context removal](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/fwpsk/nf-fwpsk-fwpsflowremovecontext0) and [callout unregistration](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/fwpsk/nf-fwpsk-fwpscalloutunregisterbyid0).

These are pending criteria, not passed checks. Production signing, rollback installer, BFE restart recovery, service/broker design and a successful live browser download remain separate work.
