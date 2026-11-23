# One final mux for a single captioned DASH Period — 10.2

The previous caption-session path first assembled media without captions, then opened that output through a concat list and muxed again to add captions. Even one Period took both passes. The intermediate could not safely carry the final AAC padding declaration through the second pass, so 10.1 explicitly excluded this path from its exact-tail correction.

For exactly one Period with selected captions, 10.2 now uses the complete retained recording in the existing final assembler. Captions are decoded and clipped to the captured interval there, and AAC final padding is declared after that one mux. The result uses the existing verified source, durable assembly receipt and publication mechanism. No final flush or checksum barrier is removed. Multiple Periods keep their existing join path and strict boundary admission.

The session still validates its Period identity, captured interval, selected streams, output dimensions, duration and verified file ownership. A later presentation start is returned as the absolute recording start while the reusable per-Period receipt retains its local capture clock. An empty caption interval can legitimately produce no caption stream; its zero cue count remains part of the receipt.

Selected audio/caption names and languages are applied at the final mux. Those four values are included in the assembly digest, so changing metadata cannot reuse a file carrying the old values. Original capture metadata is not rewritten. The new optional parameter belongs to the private assembler; public native interfaces and retained core-object layouts are unchanged.

## Qualification

All five native programs build as 10.2. Both source browser manifests advance to 0.73.66; browser scripts are unchanged. The bounded compiler driver reuses 40 hash-verified core objects and rebuilds Bridge and DashManifest. All 738 pinned source inputs and 11 produced executables verify. Tool discovery, MIDL generation and the complete packaging entrypoint are not exercised.

The 20 AAC structural and 42 strict retained-boundary assertions pass. Four owned loopback cases exercise actual capture, the session assembler, receipt reuse in a new process and final publication:

| Caption session | Exact decoded audio samples | Final bytes | Result |
| --- | ---: | ---: | --- |
| 48 kHz, one second | 48,000 | 47,664 | Captions preserved; changed track names invalidate the receipt |
| 44.1 kHz, one second | 44,100 | 47,929 | Selected audio/caption names and English language preserved |
| Empty caption interval | 48,000 | 47,089 | Zero cues retained in receipt; no empty caption stream |
| Period starts at five seconds | 48,000 | 47,672 | Absolute start remains five seconds across receipt reuse |

In every case, complete decoded PCM equals the retained source prefix. All 25 encoded video packets match the source payloads, normalized PTS/DTS and durations, as do decoded frame hashes. Caption text survives where present. Final file sizes and SHA-256 values equal the verified pre-publication values. No additional origin request or session concat file is created during assembly/reuse/publication. This does not prove one request per resource during capture.

The frozen control uses the retained **10.1 assembler object**, with the same current fixture actor and other build inputs. It produces 48,128 samples rather than 48,000 for the one-second caption case. The new path ignores its obsolete session receipt, assembles correctly from retained resources and reuses its own receipt after restart. Audio and caption packet payloads are identical to the frozen control. The combined media payload is not identical: the old concat pass adds 36 bytes of SPS/PPS configuration NAL units to the first H.264 packet. The new packet matches the original source directly. An initial whole-payload equality hypothesis was rejected and its measurement retained; it is not reported as an equality pass.

Five direct-recording regression cases also match source PCM, decoded video/captions and final publication sizes/checksums. Three negative controls reject a wrong requested height, an incompatible audio selection and a missing selected caption track, preserving the previously accepted files; a valid selection works afterward.

The first continuous-recording attempt failed in its initial audio/video case with `DASH manifest expired while synchronizing its clock`, after retaining 1.024 seconds and before publication. The manifest advertises a 200 ms update interval. The desktop compiler was active during that attempt; **a causal relationship is unproven**. The process and origin terminated. With compilation finished, a fresh run using unchanged sources, actor, fixtures and timing limits passes all seven existing cases: audio/video, video-only, audio-only, three strict negative cases and Save during held refresh. Both attempts are retained. Passing the retry does not resolve the observed expiry failure. The held-response cancellation also logs the expected connection-reset diagnostic on the local fixture server.

See [scoped verification](validation/single-caption-session-10.2/verification.json), [caption workflow](validation/single-caption-session-10.2/caption-workflow.json), [packet oracle](validation/single-caption-session-10.2/encoded-payload-control.json) and [first continuous failure](validation/single-caption-session-10.2/continuous-first-failure.json). Installed program and extension files remain unchanged (nine protected hashes). No browser interaction, installed captioned live-Save test, actual IDM comparison or cross-player acceptance is claimed. Owned media and binaries stay local; tracked records contain metadata, logs and exact source snapshots.

## Remaining order

1. Investigate the retained manifest-expiry failure with monotonic receive/parse/synchronization timings and frozen controls. Preserve stale-manifest rejection; establish a bounded refresh/recovery route rather than relaxing validity or blindly retrying.
2. Exact video presentation/decode ownership and necessary random-access preroll for non-aligned cuts.
3. Interior AAC overlap, priming and independent-encoder continuity. Final-file padding does not solve these.
4. Complete capture/Save/restart qualification for newly supported boundaries and investigation of repeated resource GETs during capture.
5. Installed browser/desktop and cross-player acceptance, followed by controlled IDM timing comparisons when the persistent lab can be operated reliably.

No installer, installation or release is part of this candidate.
