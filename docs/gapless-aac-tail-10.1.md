# AAC final-tail correction — 10.1 / browser sources 0.73.65

This candidate addresses the final partial AAC packet in a qualified zero-origin, single-Period DASH recording. It does not relax retired-Period admission or solve interior joins, video preroll, encoder-state continuity or nonzero leading edits. Qualification results are recorded separately below; source and build versions do not establish installed acceptance.

## Implementation

The assembler first checks the retained fragmented input and its probe: one AAC LC stream, a matching integral sample clock from 8,000 through 65,535 Hz, no initialization edit list, and a zero first decode timestamp for the audio track. Only a positive interval ending inside a 1,024-sample packet requests correction. Unsupported inputs retain the existing assembly path.

For this qualified path, the ordinary stream-copy mux reserves a fixed 512-byte encoding-tool value in the existing global movie metadata. After muxing, a bounded parser checks the single audio track, zero leading edit, sample timing and sample-size tables. Their counts and media duration must agree. The difference between encoded samples and requested samples must be positive and less than one packet. A whole packet cannot be disguised as padding.

The writer replaces only that reserved metadata slot with the normal UDM version and an iTunSMPB final-padding declaration. The tag is global, after the initialized audio track and before any later caption track. A track-local prototype produced the wrong decoded count and was rejected. The new movie metadata has the exact original length: media payload, file size and chunk offsets remain unchanged. The existing assembly checkpoint still flushes the completed file, calculates its checksum and commits its receipt before publication. No durability barrier is removed.

The single-Period receipt includes the tail policy version so that a previously verified but padded output cannot silently pass as corrected. Corrected receipts remain reusable after restart. Multi-Period intermediates keep the original policy. Caption **sessions**, which perform another mux after their intermediate assembly, also retain the original policy until that final mux is qualified; direct single-recording caption assembly is a separate check.

## Boundaries and outstanding work

- This is an MP4/M4A AAC LC metadata path, not a general exact-sample editor. Metadata reads are bounded to 64 MiB and supported box/table counts.
- A successful decoder exit and the coarse duration guard are insufficient evidence. Compare complete decoded PCM with the retained source interval, and preserve all failing controls.
- An eligible mux that loses its reservation or produces unsupported timing fails before a successful receipt. It does not silently publish an uncorrected file.
- Exact bytes at verified assembly/publication do not prove browser File Info size or dialog timing.
- Other players and remuxers may handle padding metadata differently. Cross-player acceptance remains open.
- Interior Period cuts, independently restarted encoders, leading trim and caption-session final muxing still need explicit sample ownership and decode-dependency work.
- No installed browser test, IDM timing comparison, installer, installation or release is implied by this change.

## Next implementation order

1. Carry a validated final-tail plan through the caption-session final mux, rather than relying on padding metadata on an intermediate file. Qualify its final output and restart receipt before enabling it.
2. Establish exact video presentation/decode ownership and required random-access preroll at non-aligned boundaries. Preserve source frame hashes and container packet timestamps; a duration tolerance is insufficient.
3. Resolve interior AAC overlap, priming and encoder continuity explicitly. The final-tail metadata rule cannot repair interior cuts or prove that independently restarted encoders share decoder state.
4. Extend the complete native capture/Save/recovery/publication fixtures to those newly supported boundaries, while retaining strict failures for missing coverage or changed identities. Investigate the separately observed repeated resource GETs during capture; this change proves no refetch during assembly/publication, not one request per captured resource.
5. Qualify the complete installed browser and desktop flows, cross-player behavior and a controlled IDM comparison in the persistent lab once its display/control and storage prerequisites are usable. Keep local sample accuracy separate from dialog timing, pre-Start exact size and total-time parity.

## Qualification

All five native programs link with version 10.1. Both source extension manifests advance to 0.73.65; browser scripts are unchanged. This bounded build reuses 40 hash-verified core objects, recompiles Bridge and DashManifest, and links the normal five programs plus focused actors. It does not exercise the complete build entrypoint, tool discovery, MIDL generation or installer packaging.

The final current-core suites pass 20 structural AAC checks and 42 strict retained-Period boundary checks. The earlier 20-check run is a repeated execution, not another 20 distinct checks. The following owned loopback cases pass capture, assembly, independent PCM comparison, receipt reuse in a new process and actual publication. Publication performs no further origin requests.

| Case | Decoded samples, expected and actual | Published bytes | Additional check |
| --- | ---: | ---: | --- |
| 48 kHz audio/video, 1 second | 48,000 | 47,063 | All 25 source video frames preserved |
| 48 kHz audio-only, 1.01 seconds | 48,480 | 9,870 | Complete source PCM prefix |
| 44.1 kHz audio/video, 1 second | 44,100 | 47,320 | All 25 source video frames preserved |
| Direct audio/video/caption recording | 48,000 | 47,601 | Video and caption text preserved |
| Already aligned audio-only, 1.024 seconds | 49,152 | 9,370 | Existing path remains sample-exact |

Each file's accepted checksum and byte count match the verified values before publication. The frozen **assembler** control uses the retained 10.0 DashManifest object with the same fixture actor and other current build inputs; it is not the complete old installed app. It decodes 48,128 samples where 48,000 are requested. The current policy invalidates that legacy receipt, reassembles from retained resources with no new origin requests, then reuses its corrected receipt after restart. The old and corrected files contain identical 44,599-byte encoded media payloads. The corrected file is 500 bytes larger because it includes reserved padding metadata; unchanged length refers to the in-place metadata replacement, not equality with the old assembler's file.

The existing seven-case continuous DASH suite also passes: complete audio/video, video-only and audio-only multi-Period recordings; rejection of gaps, short audio and changed presentation identity; and Save while refresh headers remain held. Its full PCM, audio-packet and video timing checks pass. A server-side connection-reset diagnostic occurred when the intentionally held Save response was abandoned; all actors terminated and the origin closed. This is recorded rather than treated as another product failure or hidden from the result.

The initial product build driver missed its Windows entry-point flag and failed to link. The corrected driver uses the existing entry-point and executable-hardening flags and reuses the compiled App unit. The first media workflow passed its three initial audio cases but rejected the fixture's standalone text/vtt caption format. A fresh second run uses a self-contained MP4 WebVTT fixture and passes all five cases. These attempts are retained separately. The noncompiled Python fixture changed after compilation; the build and validation recipe hashes record that distinction.

See [verification](validation/gapless-aac-tail-10.1/verification.json), [five-case workflow](validation/gapless-aac-tail-10.1/workflow.json) and [encoded payload control](validation/gapless-aac-tail-10.1/encoded-payload-control.json). Owned media and executables remain local; the tracked evidence contains metadata and exact source snapshots. Nine protected installed files remain unchanged. No setup, installation, release or explicit temporary-file cleanup was performed.
