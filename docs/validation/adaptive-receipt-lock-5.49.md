# Adaptive receipt contention and File Info size timing

This follow-up starts from the [5.48 File Info and range evidence](completion-file-info-range-followup.md). It keeps dialog readiness and verified exact size as independent measurements.

## Measurement

The Edge fixture records each selected segment request's start and response-finish events relative to the format selection. Its local HTTP server reports when it finished writing the response. Client reads, receipt writes, merging, output verification and the next dialog refresh can still occur afterward. The trace contains only synthetic segment paths and elapsed times.

Two baseline 5.48 MP4 runs showed the correct 6,560,076 bytes before Start. Each used nine segment responses. The origin finished the last response at 1,506 and 2,031 ms after selection. File Info showed the verified size at 2,815 and 3,675 ms. Those differences were 1,309 and 1,644 ms. Individual origin response times were 0–8 ms in these runs. The first four video requests arrived close together. The next audio requests were separated by larger gaps, so origin response duration alone cannot explain the timeline.

## Change in 5.49

Static adaptive transfers write one durable receipt per part. The part workers each own a distinct index, and the journal's generation changes only after they join. Previously the worker held the catalog mutex during receipt serialization, flush and rename. 5.49 writes the durable receipt outside that lock. It still publishes the in-memory receipt and completed-segment count only after the durable write succeeds. SHA-256, retained-cache validation, final media verification and the existing generation checkpoint remain in the path.

The native contention fixture holds an old segment receipt open for reading without delete sharing. It watches for the replacement scratch file, probes catalog access while replacement is pending, and checks that the uncommitted segment is not counted early. It releases the handle and verifies the published file against the final SHA-256. The recovery fixture covers the same HLS and DASH paths used for address refresh and pause.

The final control linked the new fixture against the frozen 5.48 core. It passed 42 checks and failed only the catalog-availability assertion. Releasing the receipt still allowed publication of the exact file. The fixture uses a generation-bound journal for recovery cases so a legacy migration does not change the bytes of their checkpoint independently of the behavior being tested.

The production candidate contains five hash-verified native executables and 95 browser and setup-guide files. Chromium and Firefox use version 0.69.13. Each bundle passed 130 focused checks. [Native build receipt](adaptive-receipt-lock-5.49-build.json)

## Candidate qualification

- The native adaptive refresh suite passed all 44 checks. The locked-receipt case could read the catalog, saw that the blocked segment had not been counted, and verified the file after releasing the reader.
- All 264 adaptive prefetch and recovery checks and 163 presentation checks passed. They cover address replacement, reuse after restart, preserving partial data, phase transitions and safe publication.
- Two isolated Edge runs matched the byte count and staging SHA-256 shown before Start to the published file and decoded it through the expected end. TS showed **1,961,216 bytes** and M4A showed **98,791 bytes**. Their exact-size labels appeared 1,313 and 1,235 ms after the origin finished sending the last segment. The main window stayed hidden, and the fixture removed its temporary host registrations.

## Alternating MP4 comparison

Three runs per version used the same controlled 720p MP4 selection in fresh Edge profiles. One attempt timed out before the page became ready for selection. Its host registration was removed and the trial was repeated in a fresh profile. All six measured selections fetched nine segments, showed **6,560,076 bytes** before Start and produced the same verified size and staging hash.

| Median across three runs | 5.48 | 5.49 |
| --- | ---: | ---: |
| Selection to enabled Start | 396 ms | 340 ms |
| Selection to first segment request | 861 ms | 628 ms |
| Selection to exact size | 2,670 ms | 2,858 ms |
| Origin's last response finish to exact size | 1,147 ms | 1,169 ms |

The median exact-size interval did not improve in this sample. The product change specifically keeps catalog readers responsive while a receipt is blocked, as demonstrated by the frozen-core control. The remaining interval includes native reads, durable receipt and catalog commits, merging, verification and dialog refresh. The next measurement should time the joined-workers checkpoint and the catalog save before assembly separately. These runs cover controlled HLS and leave public streaming and an interactive IDM comparison open.

