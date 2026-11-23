# Real offline DASH assembly acceptance on 9.76

The existing 9.76 build passes actual single-Period and two-Period assembly, cached reuse and cross-volume publication. Production code and versions remain **9.76 / extensions 0.73.40**; this is qualification of the preceding ownership change, not another app improvement or version increment.

## Workflow and independent content oracle

The owned origin serves a static MPD with two adjacent one-second Periods and stable track identities. Shared FFmpeg generates ten blue 640×360 H.264 frames for the first Period and ten red frames for the second. The fixture separates each fragmented MP4's initialization and media boxes, then UDM's real manifest parser, resource downloader and recording sealer capture them into three independent recordings: a single Period and two Periods for one joined session.

After capture completes, the controller stops the origin and acknowledges that shutdown. Only then does UDM assemble, restore cached assembly, join the session and publish into a separate C: destination. The native actor checks actual file ownership, exact size, saved completion and SHA-256, and fully decodes both published files. A separate source-frame oracle compares every decoded frame with the original generated videos.

| Published output | Size recorded after assembly | Final file size | Verified frame sequence |
| --- | ---: | ---: | --- |
| One Period | 2,311 bytes | 2,311 bytes | All ten blue source frames |
| Two joined Periods | 3,848 bytes | 3,848 bytes | Ten blue, then ten red source frames |

All **19 native workflow assertions** pass. The independent frame comparison additionally confirms all 30 decoded final frames against the twenty original source frames; the single output deliberately reuses the first ten. Cached single and joined outputs keep the identical assembly path and digest. These tiny compressible fixtures test the pipeline, not transfer throughput or typical public-video sizes.

The successful origin records **six HEAD requests and seven GET requests**: one MPD GET and six media-body GETs for the three independently owned captures. Assembly, cache restoration and publication occur with the origin stopped, so they cannot add a re-download. This is not a universal request-count guarantee for other manifests or failures.

## Corrections retained in the evidence

The initial standalone fixture failed to compile because it omitted `<sstream>`; no origin or native actor had started. The corrected fixture reused the unchanged generated media and frame oracle. Its first runtime capture then correctly rejected the initial server, which omitted strong ETags required for version-bound resources. That attempt reached one assertion and produced no output. The corrected origin supports strong tags, HEAD and byte ranges; it reuses the same executable and media assets. Both failed records remain separate from the successful second runtime epoch.

No production validation rule was weakened. This acceptance uses a static final presentation and calls the real capture/seal/assembly/publication APIs; it does not exercise the full dynamic live-refresh controller, sliding windows, captions, interruption recovery, user confirmation dialogs or browser handoff. In particular, the size comparison is backend assembly metadata versus final bytes, not a new GUI observation.

## Evidence and cleanup

[Successful native results](validation/dash-assembly-9.76/epoch2/results.json), [independent frame oracle](validation/dash-assembly-9.76/frame-oracle-epoch2.json), [request records](validation/dash-assembly-9.76/requests-epoch2.json), [captured inputs](validation/dash-assembly-9.76/inputs.json), [failed first runtime](validation/dash-assembly-9.76/initial/results.json), [final audit](validation/dash-assembly-9.76/final-audit.json). The maintained fixture is [dash_assembly_workflow.cpp](../benchmarks/speed/dash_assembly_workflow.cpp).

Generated payloads and the standalone compiler/executable files are removed after verified sanitized export. The shared media helpers are accessed through a temporary checked junction, then that junction is removed. Current native products and all 42 referenced core objects remain. No installer, installation, commit, push or IDM comparison is performed. The existing speed gate and full inside/outside parity remain open; desktop control still fails to initialize.

Cleanup removed 24 individually path/type/hash-checked files, totaling 7,134,818 logical bytes (about 6.8 MiB). Physical space reclaimed was not measured. All 693 captured normal inputs, five products, 42 referenced core objects and eleven protected installed/catalog/helper files still match their captured hashes.
