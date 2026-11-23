# DASH assembled-output ownership

UDM **9.76 / source extensions 0.73.40** carries the actual hashed DASH output into publication. The change also replaces the session's second full Period-output hash with a check against that continuously owned object. There is no new IDM timing result or installed-app change.

## Cause and implementation

Both fresh assembly checkpointing and cached assembly restoration used local verification handles. Their returned `DashRecordingOutput` contained a path, size and digest, so its bytes became writable again before the caller confirmed or published them. A session then reopened each Period and hashed it again. That recheck did not protect the final returned output during confirmation.

The output now carries a shared owner of `PublicationSourcePin`. Fresh checkpointing owns its existing read/write file object through the mandatory full flush, hash and receipt publication. Restoration owns the read object used to check its cached digest. Both exclude external writers. Ownership follows the output through single-Period return, multi-Period joining and confirmation.

The session still pins each Period name against deletion while probing and joining. Its name guard shares the owned write access; the source pin itself excludes external writers. It compares the named file with the original held identity and measured size. The protected original's hash can therefore be reused instead of making the former redundant full content pass. Metadata probing remains necessary.

Live DASH clones the actual file object for its existing publication handoff and releases the output's extra reference before publication. This keeps file pins bounded by accepted publication rather than extending them past the helper's return. The clone preserves captured identity and access modes; it never opens a new pathname or accepts caller-supplied identity metadata.

A duplicated Windows handle shares its file position. Because the DASH hash leaves that position at EOF, a cross-volume copy must seek to zero before reading. `publishMediaOutput` now does so explicitly. Copies still verify their digest, and no durability or catalog barrier is removed.

## Qualification scope

The maintained [DASH output fixture](../benchmarks/speed/dash_output_ownership.cpp) includes the real checkpoint and restoration implementation, linking the other 41 core objects. Its synthetic tiny payloads isolate ownership and publication; they are not a public-video or complete live-capture test. A frozen-source control exercises the previous implementation. Fresh and restored outputs are checked for writer exclusion and exact completed publication on D: and a separate C: destination.

The maintained media workflows additionally use real generated MP4/M4A files, exact-size confirmation, destination changes, failure recovery and complete decoding. A real video is probed and decoded while its write-capable source pin remains held, then copied across volumes after its hash has reached EOF. These checks qualify the handle/read/copy mechanics; full multi-Period live DASH capture, caption transitions and installed browser behavior remain separate gates.

Results, frozen controls, build identities and cleanup are recorded in [the final audit](validation/dash-output-9.76/final-audit.json). Five native products are required to carry 9.76. The previous finishing-speed regression remains unresolved; fewer hash reads alone do not prove a lower total download time.

## Remaining boundaries

Final pathname-based rename, assembly creation, retained-output discard and abrupt-process recovery still require their own ownership audits. The new retained file owner does not establish protection against every external directory or namespace change. No power-loss qualification, universal filesystem guarantee or complete IDM parity claim follows.

No installer, installation, commit, push or release is part of this increment. Temporary test payloads and compiler artifacts are removed after verified evidence export; current products and their referenced core objects remain.

## Results and cleanup

All five native products carry 9.76. The 22 affected suites pass 1,532 assertions. Fresh/restored DASH ownership and publication pass sixteen checks across D: and C:; the frozen 9.75 control reproduces four failures. Real media workflows pass 88 checks, including nine complete EOF decodes and a write-capable pinned-video cross-volume copy after hashing reaches EOF. These counts overlap where indicated; the nine decodes are included in the 88.

406 generated test/compiler files and superseded core objects were individually verified and removed: 502936428 logical bytes. The temporary media-tools junction was separately removed. Current products and all 42 referenced core objects remain; eleven protected installed/catalog/helper files are unchanged. Physical space reclaimed was not measured.
