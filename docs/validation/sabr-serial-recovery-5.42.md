# Single-connection YouTube recovery

The recovery page uses the continuous verified timeline beginning at zero. Adjacent verified windows extend that prefix. A gap stops it, even when later windows contain verified data. Invalid or inconsistent range metadata leaves the original playback address in use.

When a secondary window requests fresh attestation, saving a matching replacement records a single-connection recovery policy on that download. The saved checkpoint still goes through the existing stream identity and segment validation. Transport retries use the same effective count, and the configured connection limit remains available for other downloads. A failed replacement save rolls back the policy with the hold.

## Validation scope

The native checks cover a gap followed by verified data, adjacent ranges, an invalid boundary, failed replacement persistence and serial output equality. The public test starts from the actual held 5.41 checkpoints and requests each fresh replacement with one real Edge panel click. The isolated driver applies the saved replacement and measures reuse, verified progress and peak requests.

All 264 streaming and recovery checks and 163 native presentation checks passed. Both 0.69.6 browser bundles passed 66 transport and 31 streaming-audio checks. Five production executables and both browser bundles are staged. [Native build receipt](sabr-serial-recovery-5.42-build.json)

## Public recovery from the cold checkpoints

| Selection | Verified prefix before | Verified prefix after | Bytes reused | Verified bytes after | Peak simultaneous requests |
|---|---:|---:|---:|---:|---:|
| 1080p H.264 MP4 | 3.000 s | 65.234 s | 317,574 | 10,712,377 | 1 |
| 360p H.264 MP4 | 9.985 s | 69.892 s | 257,592 | 2,353,150 | 1 |

Both runs reused every byte in their starting checkpoint and retained the original download record and configured limit of eight connections. Each fresh session advanced on window one. The origin requested fresh attestation again on the fourth request in each run. No exact final size or publication was available for these held jobs. The first 1080p run then timed out while waiting for playback in its second recovery tab, before it submitted a second replacement.

The retry used a new Edge profile and the saved 65.234-second checkpoint. One panel click requested a fresh replacement. The driver reused all 10,712,377 bytes and advanced the same 1080p download to **127.200 seconds** and **22,025,832 verified bytes**, again with one simultaneous request. The next challenge arrived on request four in window one. The fixture saved the checkpoint and removed all temporary host registrations.

The single-connection policy avoids the immediate secondary-window challenges seen in 5.41. The tested source still requires renewal after roughly one additional minute per fresh session. Full completion from a cold start and a paired installed IDM comparison remain open.
