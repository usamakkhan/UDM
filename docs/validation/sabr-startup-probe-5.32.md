# YouTube stream startup in UDM 5.32

The 5.30 public 1080p handoff opened eight overlapping streaming requests before the origin required a fresh browser session. It received zero media bytes. [5.30 public run](sabr-attestation-recovery-5.30.md#public-1080p-run)

UDM 5.32 starts the first timeline window that still needs media. When the parser receives selected payload for that window, it starts the remaining windows while the response is open. Cached repeats and unselected payload leave the other windows waiting. A cached first window lets the next incomplete window lead. A complete verified cache can still assemble offline. An immediate rejection keeps the existing refresh hold and sends no requests from the other windows.

The Chromium and Firefox 0.68.96 stages each passed 19 streaming capture and 31 audio selection checks. These exercised the staged browser sources in controlled fixtures.

## Native qualification

The final native streaming suite passed 238 checks. It verified one request on immediate rejection, exact healthy output with four overlapping responses, the eight-request cap, selection of the first missing window, full-cache offline assembly, ignored seek fallback, and cancellation before and after the other windows open. Video with audio and audio-only paths both passed. The first test run's cancellation counter missed responses destroyed between reads. The final fixture counts open responses by their lifetime. The production receipt separates that fixture-only change from the original build inputs. [Build and fixture receipt](sabr-startup-probe-5.32-build.json)

## Public 1080p runs

Both fresh Edge profiles delivered the 1080p selection to UDM 5.32. The serial fixture set the existing connection preference to one in its private catalog. The two sessions were captured separately, so these results establish observed behavior per run. They cannot establish a controlled throughput comparison.

| Configuration | Requests | Peak concurrent | Bytes received | Verified retained bytes | Verified timeline |
| --- | ---: | ---: | ---: | ---: | ---: |
| Eight windows | 8 | 8 | 841,862 | 317,574 | 3.000 s |
| One window | 4 | 1 | 10,712,377 | 10,712,377 | 65.234 s |

The eight-window run retained media from the opening window before the attestation rejection. All four retained segments matched their cache hashes. The serial run also stopped at an attestation rejection after media started. All 23 retained pieces matched their cache hashes. Concatenating the retained pieces per track produced a 1080p H.264 video prefix and an AAC audio prefix that FFmpeg decoded through their retained ends. The initialization metadata advertises the full source duration, so timeline coverage comes from the verified segment index.

Both runs recorded the session refresh hold, kept the size unknown, and published no final file. Both temporary native host registrations were removed. These runs establish partial public decoding. Public completion and exact size before Start remain open.
