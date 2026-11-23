# Verified media ownership through publication

UDM **9.75 / both source extensions 0.73.39** keeps the verified media object protected when passing it to publication. The installed app is unchanged. This correctness improvement does not resolve the existing finishing-speed gate or establish IDM parity.

## Reproduced failure

The prior `VerifiedMediaFile::releaseForPublication` checked the source name, closed its strict verification reader and returned no ownership proof. `publishMediaOutput` later opened the path again. Between those calls a writer could modify the video. Publication could then accept the modified bytes with the digest measured before the modification.

The frozen 9.74 control reproduces this with actual Windows handles and actual catalog publication, in the same folder and after a destination-folder change. All six protection assertions fail: a writer enters the interval, the published bytes differ from the verified digest, and accepted history no longer describes the actual file. The owned files are tiny synthetic inputs; no personal file is involved.

Retained-media reuse had a related interval: it hashed one held object, checked its current name and then let the helper reopen that name. A replacement after the selection recheck could therefore supply a different object.

## Ownership handoff

`PublicationSourcePin::openReadFrom` opens a publication reader while the measured reader remains alive. It compares the actual held object's volume/file index, size and last-write time with the newly opened object. A different file with equal bytes is rejected. No caller-supplied identity snapshot is accepted.

`VerifiedMediaFile` releases its original strict reader only after acquiring that matching publication reader. YouTube assembly and adaptive assembly move the reader into `publishMediaOutput`. Retained-media reuse acquires the same matching reader from its actual hashed handle. The publication helper consumes the passed reader instead of reopening the source path. Its existing hard-link/cross-volume copy and durable publication machinery then owns the file.

The reader excludes writers. It shares deletion so existing publication can rename the file; final name validation and durable catalog acceptance remain responsible for the namespace boundary. The next caller cannot silently recover from a failed handoff by reopening an unrelated name.

No network request, remux, second full hash, body copy or disk-flush barrier is added to the ordinary successful handoff. There are additional reader-open/metadata checks. A cross-volume copy still performs its existing output verification. Mandatory body flushing, recovery journals and exact catalog acceptance remain unchanged.

## Qualification

All five native products build with version 9.75. All **1,532 assertions in 22 affected suites** pass, including ten new verification-handoff checks. The standalone retained workflow passes six checks on 9.75 and reproduces two failures on frozen 9.74. Another **84 media-workflow checks** pass, including exact pre-Start size, reuse, destination changes, cross-volume copying and catalog-failure recovery. Eight completed media files decode through EOF without errors. The 84 include the eight decoding checks; the ten handoff assertions are included in the 1,532, not additional to them.

Build, affected-suite results, frozen negative controls, retained-source replacement controls, real media workflows and cleanup are recorded in [the validation folder](validation/verified-media-handoff-9.75/final-audit.json). The native regression is `VerifiedMediaHandoffTests`; the standalone retained-workflow regression is [retained_media_handoff.cpp](../benchmarks/speed/retained_media_handoff.cpp).

The isolated media workflow uses the existing maintained media checks and the installed shared FFmpeg/FFprobe through a checked directory junction. It creates no helper copies. Full EOF decoding is checked separately from stream metadata. These are controlled backend workflows, not an installed Chrome or live public-video comparison.

After sanitized evidence export, 395 individually verified generated objects, test executables and payloads were removed without recursion: **283,432,126 logical bytes**, about 270 MiB. The temporary helper junction was removed separately; its shared target remains unchanged. Small metadata/results and current build products/core objects remain. Physical space reclaimed was not measured.

## Remaining scope

This closes the verification-to-helper handoff in the changed ordinary assembled and retained-media routes. It does not close final source-name validation followed by pathname-based rename, every media assembly creation/discard path, abrupt-process recovery of unjournaled copied stages or external directory changes. File identity fallback on other filesystems also needs its own qualification. See [earlier relocation limits](relocation-ownership-9.71.md).

The split drainage control remains rejected, and the prior 9.74 sequential performance losses remain open. A fresh IDM comparison and qualified kernel attribution are still needed. No installer, installation, commit, push or release is part of this increment.
