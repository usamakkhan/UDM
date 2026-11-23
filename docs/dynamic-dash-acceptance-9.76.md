# Dynamic DASH controller acceptance on 9.76

The unchanged **9.76 / source extensions 0.73.40** build completes a changing two-Period DASH presentation through the real adaptive admission, public Resume/queue dispatch and download worker. Eleven native assertions pass. Every decoded final frame matches the independently generated source, and exactly one final file is published across volumes.

## Controlled live presentation

The owned origin initially announces only the first one-second Period. After its media response is sent, the next manifest adds an adjacent second Period. After the second media response, the origin ends the presentation with a static manifest. Dynamic responses provide explicit segment timelines, stable representation identities, server UTC timing, strong resource tags and finite update intervals. The video consists of ten blue frames followed by ten red frames at 640×360, with no selected audio or captions.

The actual worker progresses through recording, waiting for segments, recording the continuation, saving and publication. Its saved session contains the ordered folders `.` and `period-0001`. Both complete segments and two captured seconds are accounted for. It seals and finishes naturally; no manual Stop flag is set. `Manager::stop` then joins the remaining owned work before final verification.

The byte size remains unknown while the stream grows, which is necessary for an unfinished live presentation. Once assembled, the retained-output receipt records **3,848 bytes** before publication. The final C: file is also **3,848 bytes**, matches accepted history's SHA-256 and fully decodes. All twenty decoded frame hashes match the generated blue/red source sequence.

The publication observer records one preflight and the job reaches Complete. Each initialization and media resource is fetched once. The successful trace contains four manifest GETs, four media GETs and four HEAD requests; observed manifest stages are `1, 2, 2, 3`. The extra stage-2 refresh is retained in the evidence rather than hidden. These counts describe this healthy fixture, not universal exactly-once behavior after failures or reconnects.

## Fixture corrections

The first standalone compile incorrectly invoked private `Manager::start`; it failed before starting an origin or actor. The corrected fixture uses the public Resume path. Its first runtime omitted the normal queue-dispatch call, so the job stayed Queued, sent no requests and was safely closed. The next attempt dispatched correctly but the test MPD used duration-only SegmentList addressing; UDM correctly rejected it because live SegmentLists require an explicit timeline.

The final origin adds the required `SegmentTimeline`. It reuses the same dispatcher-enabled executable, generated assets and independent frame oracle. The production build is unchanged throughout these fixture corrections. Failed records are retained separately and are not counted among the eleven passing assertions. Protected job fields in tracked evidence are replaced with hash/length summaries.

## Scope and evidence

This qualifies one real video-only dynamic-to-static Period transition and cross-volume publication. It does not qualify browser/UI behavior, subtitles, selected audio, sliding-window eviction, outages, changed validators, manual Stop, sleep, abrupt-process recovery or all live addressing modes. It also does not establish IDM speed parity. Those remain separate gates.

[Native results](validation/dynamic-dash-9.76/success/results.json), [frame oracle](validation/dynamic-dash-9.76/frame-oracle-epoch3.json), [request trace](validation/dynamic-dash-9.76/requests-epoch3.json), [captured inputs](validation/dynamic-dash-9.76/inputs-epoch2.json), [queued fixture](validation/dynamic-dash-9.76/queued/results.json), [timeline rejection](validation/dynamic-dash-9.76/timeline-rejection/results.json), [final audit](validation/dynamic-dash-9.76/final-audit.json). Maintained fixture: [dynamic_dash_workflow.cpp](../benchmarks/speed/dynamic_dash_workflow.cpp).

No production edit, version increment, installer, installation, commit, push or IDM comparison is part of this qualification. Generated media and standalone compiler/executable files are removed after sanitized evidence export. Current products, their 42 core objects, personal catalogs and shared media helpers are preserved.

Cleanup removed twenty individually verified generated files totaling 18,544,251 logical bytes (about 17.7 MiB), and removed the temporary media-tools junction. Physical space reclaimed was not measured. All 693 captured normal inputs, five native products, 42 referenced core objects and eleven protected installed/catalog/helper files still match their captured hashes.
