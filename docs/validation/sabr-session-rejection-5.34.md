# YouTube session renewal and request location

UDM 5.34 treats both the explicit renewal response and the attestation challenge as a request for a fresh browser session. It keeps verified media and holds the job so the queue cannot replay the rejected session. The rejection record contains the protocol part, one-based window, cursor in milliseconds and request number within that window. It contains no URL, request body or credential. Saving a matching replacement clears the old rejection with the hold in the same catalog transaction.

The public continuation uses the original 1080p job and a verified copy of its existing cache. The browser fixture follows the app-generated page, receives the matching offer through native messaging, then makes an explicit Save and Resume decision in the test driver. Full-file qualification checks final bytes and hash against the catalog and decodes both tracks separately through their expected ends. These checks cover completed output independently of exact size before Start.

## 5.34 qualification

The native streaming suite passed 248 checks on rerun. It exercised both renewal signals, queue and prefetch holds, persistence of the rejection location and its removal when a matching replacement commits. An initial outer timeout occurred while a separate public fixture was also running. Chromium and Firefox 0.68.98 each passed 19 streaming capture and 31 audio checks. All five native executables, 82 recorded inputs and 95 staged browser files matched their hashes. [Build receipt](sabr-session-rejection-5.34-build.json)

An isolated 5.34 Edge run continued the public job from 369.429 to 429.337 seconds. It reused all 73,403,535 saved bytes and advanced the verified cache to 88,521,232 bytes. The native record identified part 58 on request four in window one at cursor 429,337 ms. The temporary host registration was removed.

## Full public publication with 5.33

Two preliminary refreshes advanced the original job from 309.522 to 429.337 seconds. A bounded chain of eleven isolated browser sessions then completed the same 1080p download. Each attempt preserved the job identity and reused the entire verified checkpoint. The final two requests reused 256,470,307 bytes and completed the 1,073-second selected timeline. UDM published one MP4.

| Published file check | Result |
| --- | --- |
| Size | 265,982,413 bytes, equal to the saved catalog size |
| SHA-256 | `6446732df2fa58d59e66a1bcf79f14bea1c0c4370373de83ac502ad9ba8f7bfc`, equal to the saved catalog hash |
| Container duration | 1,072.994104 s |
| H.264 video | 1920×1080, decoded to 1,072.800 s |
| AAC audio | Decoded to 1,072.994104 s |

The first decode hit the fixture's 90-second limit. A separate run decoded both tracks through their ends and rechecked the file hash. The browser fixture now allows up to 300 seconds per track.

## Full public publication with 5.34

A fresh Edge session using extension 0.68.98 followed the 5.34-generated recovery page from the same last prepublication checkpoint at 1,038.396 seconds. It reused all 256,470,307 retained bytes, completed in two native requests and cleared the rejection. The published MP4 matched the 5.33 output byte for byte. Both tracks decoded through the ends shown above within the expanded fixture deadline, and the temporary host registration was removed. The verified 5.34 file, last prepublication checkpoint and both sets of reports are retained. Duplicate output and earlier cache files were removed after hash verification.

This result establishes completion for this serial, refreshed 1080p workflow. An installed IDM comparison, other public selections, fewer refresh interventions and exact size shown before Start still require their own evidence.
