# Browser video prefetch from the admission record

UDM 5.73 selects the temporary media folder when it accepts a fresh browser video offer with File Info prefetch enabled. The offer, folder and receipt are saved together. File Info and the worker can start from that write when the saved offer still matches. Start still controls publication. Recovery uses the saved folder and opens the unconfirmed job paused. Changes to a job and media that needs to select a staging path at startup still use the existing admission write.

The native streaming suite passed 268 checks, including unchanged catalog bytes through both prefetch entry points and paused recovery with the same custom staging folder. A separate isolated Edge profile for each output exercised the extension, native messaging, File Info and full decode. The trace counted catalog writes between the File Info prefetch pulse and the first media segment.

| Selected output | Writes before first segment | Bytes shown before Start and saved | Click to actionable File Info |
|---|---:|---:|---:|
| 720p MP4 with Spanish audio | 0 | 6,560,076 | 1,241 ms |
| 360p TS with English audio | 0 | 1,961,216 | 449 ms |
| M4A with Spanish audio | 0 | 98,791 | 548 ms |

Each run matched the staged hash to the published file, decoded the complete eight-second output, and selected only the requested audio. The M4A run fetched no video segments. All three temporary native host registrations were removed. The results and timing traces are under `candidates/durable-media-prefetch-5.73-edge-mp4`, `-edge-ts` and `-edge-m4a`. These are single local observations per format. Public CDN, installed-profile and paired IDM timing remain open. [Native build receipt](durable-media-prefetch-5.73-build.json)
