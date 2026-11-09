# YouTube capture freshness in UDM 5.43

Page observations record the request time. A late response cannot replace a newer accepted capture in either observer. Handoff compares the page capture with the latest eligible browser capture for the same video, frame, document and visit. If both observers recorded the same request, the identity-verified page capture takes precedence. A ready page capture bounds the optional browser lookup to 150 ms.

## Qualification

Each 0.69.7 browser bundle passed 21 browser capture, 7 UMP observer, 68 transport handoff and 31 streaming audio checks. The regressions cover an older response finishing late, a return to the same video on a different visit, a newer browser request, and fallback when the browser lookup stalls. All 264 native streaming and recovery checks and 163 native presentation checks passed. Five production executables and both browser bundles are staged. [Native build receipt](sabr-fresh-capture-5.43-build.json)

## Public control and candidate

Each isolated Edge run started from the same 1080p checkpoint at 127.200 seconds and reused all 22,025,832 verified bytes. Both received the matching replacement from one panel click. Both selected a browser-observed request. Neither run supplied a page capture for comparison.

| Public recovery observation | 5.42 | 5.43 |
|---|---:|---:|
| Age of selected request at handoff | 1,779 ms | 488 ms |
| Verified timeline after recovery | 189.707 s | 189.707 s |
| Verified bytes after recovery | 30,841,739 | 30,841,739 |
| Next attestation challenge | Request 4, window 1 | Request 4, window 1 |

The two request ages are individual observations from separate browser profiles. Both transfers remained held, and the temporary host registrations were removed.

## Page observer follow-up

In the loopback HTTPS Chrome fixture, the page observer lost the response when the caller consumed it immediately. The diagnostic reported that `Response.clone()` reached a body that was already in use. The page observer succeeded when the caller waited. The loopback fixture did not supply both usable sources together, so the tests that exercise preference between observers are controlled regressions. The next implementation step is to take the observer's response clone before awaiting the request body while continuing to return the original response to playback.

The ordering fix leaves the source's fourth-request renewal requirement unresolved. Full cold completion and a paired installed IDM comparison remain open.
