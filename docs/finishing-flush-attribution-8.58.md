# Finishing attribution and rejected sparse experiments, 8.58

Production UDM remains native 8.58 / extension 0.72.22; the installed app remains 8.32. These investigations identify waits and reject two private experiments. Neither candidate was promoted, installed or released.

## A working observation boundary

A diagnostic console entry point links the exact 42 qualified 8.58 core objects and uses Manager's public resume/tick path. It runs an isolated loopback download, waits for workers, calls normal stop and destroys Manager. It then records a QPC endpoint and waits for the observer to save a snapshot before ordinary process exit. This boundary includes backend work through Manager destruction; it is not the production App/GUI entry point or proof about every process-global or kernel background activity.

The first controller omitted tick pumping and ended with Queued status, without completing a transfer. Its private failure is retained. The corrected controller passed an 8 KiB smoke and a 64 MiB D: split transfer. No installed binaries or personal catalogs were used for these downloads. A rejected RtlExitUserProcess stream/terminal hook stalled its separate 8 KiB fixture; it was retired after exact owned-process identity/hash checks and a recorded forced cleanup. It was never used for the 64 MiB workload. [Failure receipt](validation/finish-realtime-8.58/rejected-exit-stream.json).

The observer's QPC call now retains the JavaScript lock while updating its short shared scratch buffer. This avoids another callback overwriting that buffer between the native call and the JavaScript read. Only this short counter call is exclusive; blocking path queries retain their existing scheduling. Frida documents these [NativeFunction scheduling options](https://frida.re/docs/javascript-api/#nativefunction). The corrected observer passed the real multithreaded backend observations with ordered intervals and no script errors.

## Actual 64 MiB split observation

The completed D: transfer matched the independent 64 MiB SHA-256 and exited normally. Its 637 captured events had zero drops and no remaining tracked files/mappings. The 270 observed successful native body-write intervals cover byte zero through 67,108,864 exactly once, without gaps or overlaps. This positive interval check does not prove absence of every possible unobserved write mechanism.

Eight native flush operations each nest inside exactly one same-thread/same-handle Win32 flush wrapper; they are eight operations, not sixteen. The optional in-flight body flush took 343.154 ms. The final native body flush took **564.189 ms**, enclosed by UDM's independently recorded **564.348 ms** PublishedFileFlush phase. The output open took **0.147 ms**, sparse normalization was unnecessary, and advisory hashing used the existing write buffer and overlapped the flush. Thus the measured final wait is inside the native flush in this run, rather than its output-open operation or a serial advisory hash pass. This does not distinguish disk queueing, filesystem/filter work and scheduling inside that wait.

One 32 KiB body write at offset 32 MiB took 248.969 ms; another at offset 48 MiB took 122.586 ms. That motivated a controlled examination of regular-file gap extension. The retrieval-pointer extent query failed with error 50 and was not used to assert fragmentation. [Sanitized baseline and complete phase records](validation/finish-realtime-8.58/backend-split64-observation.json).

## Small gap-extension control

On D:, two new 16 MiB logical files each received the same 32 KiB block at offset 8 MiB. Both had identical logical content and SHA-256 after flush/close. The regular file had 16 MiB allocated and took 60.224 ms for its write and 84.014 ms for flush. The sparse file had 128 KiB allocated and took 0.136/25.031 ms. This is one regular-then-sparse pair, not a balanced performance qualification or complete attribution of the longer production wait. It establishes a concrete avoidable allocation/gap path to investigate. No privileged valid-data-length change was made. [Control, hashes and allocation sizes](validation/finish-realtime-8.58/vdl-gap-control.json).

## Rejected private candidates

The first candidate changes only the bounded-range regular-file reservation rule, keeping incomplete ranges sparse. It leaves all final barriers intact. Its final body flush fell to 13.532 ms, but a preceding publication-intent flush took 261.058 ms, and its instrumented total backend interval was 2,759.094 ms versus the baseline's 2,338.181 ms. The checkpoint strategy also changed because the existing prepared path excludes sparse files. A smaller final phase therefore did not prove an overall improvement. [Retained first result](validation/finish-realtime-8.58/sparse-first-prototype.json), [source delta](../benchmarks/speed/experiments/rejected-sparse-regular-8.58.patch).

A second private candidate clears the sparse attribute only after body completion, verifies the resulting attribute and retains the prepared final checkpoint plus final flush. Its instrumented final native body flush was 292.025 ms, but the overall interval was still worse. This was followed by uninstrumented AB/BA trials, using the same core entry point, origin size/rate/latency/seed and separate post-exit output flush. All four outputs matched their hashes, exited normally and were deleted immediately.

| Variant, two trials each | Median backend tail | Median process-exit tail | Median through post-exit flush | Median backend wall time |
| --- | ---: | ---: | ---: | ---: |
| Qualified baseline | 564.698 ms | 567.896 ms | 607.932 ms | 2,183.670 ms |
| Sparse + prepared checkpoint | 819.828 ms | 823.174 ms | 854.239 ms | 2,719.570 ms |

The process-exit fields use a single FILETIME/performance-clock anchor; post-run clock drift was not rechecked, so those fields are not tight clock-error-qualified acceptance metrics. The monotonic backend and durability endpoints remain separate evidence. The candidate has one tail win and one loss; total backend wall time is worse in both trials. It is rejected for promotion. These small samples do not establish universal regressions, and neither isolated-core observations nor these trials replace an installed UDM/IDM comparison. Origin reported send-byte totals were also higher for the candidate; those totals are not measurements of acknowledged wire bytes. [All four uninstrumented rows](validation/finish-realtime-8.58/sparse-balanced-uninstrumented.json), [second observation](validation/finish-realtime-8.58/sparse-prepared-prototype.json), [source delta](../benchmarks/speed/experiments/rejected-sparse-prepared-8.58.patch).

## Cleanup, preservation and next work

Both rejected prototype executables, their private Transfer source copies/objects and generated fixture files were individually scoped, reparse/link checked, hashed and removed. The cleanup removed 55 files / 287,153,802 logical bytes, in addition to the four immediately removed 64 MiB balanced-test outputs (268,435,456 bytes). Qualified products/core objects, compact records and the single reusable baseline diagnostic controller remain. Logical totals do not measure physical reclaimed allocation. [Cleanup receipt](validation/finish-realtime-8.58/backend-experiments-cleanup.json).

The temporary elevated worker expired normally after its 30-minute idle session; its shutdown receipt and direct process-absence query were checked. No new approval prompt, service or scheduled task was used for this investigation. [Worker shutdown](validation/finish-realtime-8.58/session-worker-shutdown.json).

The next storage change needs to avoid gap work without losing the current checkpoint strategy or trading a small final phase for a longer transfer. An ordered write path would require separating volatile received bytes from disk-written resume credit, bounded buffering, cancellation/error propagation and a writer join before final verification/publication. That is a storage/recovery design change, not another flush pacing threshold. It needs its own fault and whole-lifetime evidence before promotion. Kernel opcode-84 decoding and fresh installed UDM/IDM comparisons remain separate unfinished gates.
