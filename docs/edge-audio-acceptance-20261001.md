# Isolated Edge audio/video acceptance — 1 October 2026

The current native 0.82 candidate and browser 0.53.1 passed 58 checks in a real isolated headless Edge profile, producing ten complete files. Every output hash was independently rechecked. The prior audio-only timeout did not reproduce and remains unresolved; these results do not establish general public-site or IDM parity.

Two iterations used actual panel controls for nested 720p MP4 with Spanish audio, audio-only M4A, English TS, MP4 with Spanish subtitles and Back navigation, and a cyclic catalog's valid 720p branch. Each selection made exactly one adaptive native call and created exactly one job. Full decoding, track dimensions, 440/880 Hz audio selection, language tags, subtitle timing/text and exclusion of unselected/overridden tracks passed.

| Stage | First audio-only selection | Second audio-only selection |
|---|---:|---:|
| Preparation to native submission | 13 ms | 19 ms |
| Native acknowledgement | 100 ms | 66 ms |
| Click routine to observed completion | 1,093 ms | 1,439 ms |

Preparation and acknowledgement timestamps come from observers around real functions. Click-to-complete uses a monotonic timer and a 100 ms job-poll interval; it includes click automation and polling overhead. It is not a network throughput benchmark. Observers do not replace player or native responses. Final screenshots were inspected for readable selection controls.

## Historical evidence

The original paired-edge-hls catalog contains only its first completed video. Its server log ends after fetching the subsequent Spanish audio playlist, with no second native job or audio-segment requests. This narrows the historical investigation to preparation/admission or related persistence, but it does not identify which stage failed: the old run did not retain a native-request trace. The newer deadline correction was independently reproduced earlier and must not be called the proven cause of this historical failure.

## Reproduce

Use tests/hls-catalog-traced.native-live.cjs with Node and Playwright, providing a fresh output-directory argument. UDM_TEST_PROJECT selects the project, UDM_APP_EXE selects the built desktop executable beside its native host, and UDM_EXPECT_NATIVE_VERSION selects the expected diagnostics version (default 0.78.0 for older fixtures). UDM_TEST_BROWSER defaults to msedge. The fixture uses a unique native-host name, private browser profile, instance tag and download catalog, and deletes its registration on completion.

The observer now records preparation, native acknowledgement and completion timing per selection, and asserts one native call/job. The prior fixture source is backed up. Runtime production code was not changed.

The test registration was removed, and a fresh process inspection found no matching isolated browser/app/host/Node process. The personal catalog stayed byte-identical. The installed application remains 0.78; 0.82 is still an uninstalled candidate.

Remaining: the original timeout's cause, recovery after lost media acceptance, broader actual-site/ad/player association, and personal activation. [Evidence](evidence-edge-audio-20261001/verification.json)
