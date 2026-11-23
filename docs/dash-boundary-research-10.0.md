# DASH boundary clipping investigation — unchanged 10.0 products

Native products remain **10.0**, and both source extension manifests remain **0.73.64**. This investigation adds assembly probes and a read-only evidence validator. It does not relax capture admission, alter a download, install software or establish IDM behavior.

## Question and result

The current retired-Period proof requires retained tracks to end together. Allowing a successor boundary anywhere within their common coverage looks sufficient at the segment level. It is insufficient at the encoded-sample level: the existing assembly path stream-copies each chapter using an output cut and then concatenates those chapter files. Cuts may intersect AAC packets or video decode dependencies.

An owned continuous-encoder fixture generated a small 25-fps H.264 video and 48-kHz mono AAC audio. The three assembly windows read the same continuous encoded source with the corresponding timestamp offsets. The oracle compares the joined result with the source's complete decoded PCM prefix and original decoded video frame hashes and container packet timestamps. No source media comes from a website or a personal file.

| Per-chapter boundary | Expected / actual decoded audio samples | Expected / actual video frames | PCM and frame-hash/PTS oracle |
| --- | ---: | ---: | --- |
| 1.28 s, aligned packet grids | 184,320 / 184,320 | 96 / 96 | Both exact |
| 1.00 s, crossing AAC packet grid | 144,000 / 144,128 | 75 / 62 | Both fail |
| 1.01 s, crossing both packet grids | 145,440 / 146,112 | 76 / 65 | Both fail |

All three outputs decode without the probe's error check failing. A decodable file is therefore insufficient evidence of complete selected content. The aligned continuous control is important: the command sequence can preserve a fully aligned continuous bitstream, rather than every case failing because the probe cannot assemble media.

Post-run packet inspection localizes the 13 missing frames in the 1-second case. The second chapter retains the source suffix beginning at the key packet at 1.28 s, rather than the requested 1.00 s; the third begins at 2.24 s rather than 2.00 s. Seven and six expected packets are omitted. The retained packet hashes match those source suffixes exactly. This identifies cut-point/dependency handling in this assembly probe, rather than missing resource bytes. [Packet-level observations](validation/dash-boundary-research-10.0/native-guard/packet-facts.json).

A separate `-copypriorss:v 1` candidate uses identical generated source bytes and the same media-tool hashes. It leaves the two crossing cases unchanged and is rejected for production. The option's existence does not establish suitable preroll semantics for this workflow; FFmpeg distinguishes input and output seeking during stream copy. [Candidate and retained failures](validation/dash-boundary-research-10.0/copy-prior-assessment.json), [official seek documentation](https://www.ffmpeg.org/ffmpeg.html), [official option definitions](https://www.ffmpeg.org/doxygen/trunk/ffmpeg__opt_8c_source.html).

A separate first experiment uses three independently encoded chapter sources. Its packet-aligned joined PCM also differs, despite equal sample counts and matching video. Those results remain preserved. They add an encoder-history/priming question; they do not, by themselves, isolate one cause. The first experiment's video timestamps came from the frame-hash output time base. The subsequent continuous control measures timestamps from container packets, avoiding that observation's clock quantization. The two methods and source corpora are not treated as identical trials.

## What the native check establishes

A compiled actor invokes the actual current `dashAssemblyDuration` function from `native/DashPeriodAssembly.hpp`. Its unchanged coarse duration check accepts all six original joined probes. Five of those six fail at least one decoded oracle. The later three-case flag candidate is separate from that six-case native invocation. The actor links the retained current core objects and records the actor/header hashes before compilation. It verifies this individual native guard, not native MPD admission, downloads, recovery or publication.

The header already describes duration as a coarse guard rather than frame/sample proof. Shrinking its tolerance alone would not implement packet ownership or decoder preroll. Codec configuration, a plausible duration and a successful decoder exit cannot replace the stronger content checks.

The reproduction mirrors the relevant cut/concat arguments, but it supplies owned local source movies directly. It does not drive a full recording through the native resource planner. In particular, these results are not a claim that all six cases are currently admitted or that an installed user's recording is corrupt. They establish a concrete gate before expanding non-aligned capture/transition support.

## Implement next

1. Retain the current strict transition proof while introducing an explicit boundary plan for verified resources. An explicit successor start must establish the requested boundary; the shortest retained track must not invent a Period end. Keep presentation identity, initialization, cancellation, restart and checkpoint ownership checks.
2. Map that requested interval to encoded sample clocks and decode dependencies. Record presentation versus decode start, required video random-access preroll, audio priming/overlap and complete packet ownership using exact time bases. Segment duration alone is insufficient. Bound metadata and retain source identities throughout planning and assembly.
3. Implement the lossless copy route only where the boundary plan can represent the interval correctly. Preserve required preroll and handle interior overlap without adding samples or losing frames. Do not silently transcode, discard a partial packet or treat equal codec configuration as proof of encoder-state continuity. Preserve unsupported recordings and report the unresolved boundary explicitly.
4. Qualify complete native recording workflows against independent source oracles: visible and retired Periods, known and newly established durations, pause/restart, held refresh and Save, positive aligned controls, partial AAC/video boundaries, missing coverage and changed identities. Check every frame's hash/PTS/duration, packets, decoded sample count and complete PCM digest. Retain all failing outputs and request accounting.
5. Extend the evidence by selected codec/container and encoder configuration before expanding admission. Keep exact final-size claims tied to the verified assembled output, then repeat installed browser and actual IDM comparisons separately.

## Reproduction and evidence

`benchmarks/speed/dash_boundary_clipping.py` requires a new output root and an explicit directory containing the existing FFmpeg tools. `--continuous-source` selects the continuous-encoder control. It generates bounded media, launches those tools and preserves every result. The analyzer `analyze_dash_boundary.py` is read-only; `--require-media` rehashes all retained source/clip/join artifacts. It does not launch the commands recorded inside a report.

Ten validator tests preserve the positive control and scientific failures while rejecting corrupt sample deltas, false exactness claims, invalid digests, duplicate chapters, unsafe artifact names and incomplete native-guard joins. The private artifacts were rehashed before export. Public records omit media payloads; reading those exported records does not rehash absent payloads. The same native products, protected installed files and shared media-tool binaries remain unchanged. No cleanup or new IDM comparison is claimed.

[Raw sanitized experiments, compiled-guard results and source snapshots](validation/dash-boundary-research-10.0/native-guard/audit.json).
