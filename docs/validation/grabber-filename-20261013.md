# Grabber filename follow-up, 2026-10-13

Grabber metadata now uses the same bounded Content-Disposition filename parser as
captured original responses. This handles quoted names, UTF-8 `filename*`,
malformed escapes, duplicate parameters, and header-line injection consistently.
The existing Grabber integration fixture covers a local download endpoint with
`filename*=UTF-8''guide%20book.pdf`, including transfer to the named file.

Focused results from the consolidated backend runner:

| Case | Result |
| --- | --- |
| `ContentDispositionNameTests` | 7 checks passed |
| `OriginalResponseTests` | 203 checks passed |
| `SyncProbeTests` | 83 checks passed after correcting its initial-response fixture |

The test runner now omits its data-directory argument for
`OriginalResponseTests`, which instead accepts a host argument. These two
fixture repairs address failures recorded in
`backend-matrix-20261012.md`. `RecoveryIoTests` and `LiveSubtitleTests` remain
unresolved from that matrix.

The changed Grabber object compiled during focused backend builds. A desktop
candidate link was attempted but did not complete: the linker returned LNK1140
while drive D had very little free space. This record does not claim a new
installed app build or a passing full native suite.
