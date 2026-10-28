# DASH live and Period reference investigation

## Reference evidence

The local `IDMVMPrs64.dll` and `IDMNetMon64.dll` hashes match the retained Ghidra analysis. Exact identities and function addresses are in `reference/idm-dash-parser-20261028.json`. This is static semantic analysis, not a live recording result. No proprietary implementation is copied.

The media parser recognizes OnDemand/static and Live/dynamic values. Its root parser stores availability start/end, presentation duration, minimum update period, buffer depth, suggested delay and maximum segment/subsegment duration. It iterates Period children; the Period parser appends linked records and reads start, duration, bitstream switching, segment addressing and adaptation sets.

The network monitor separately detects fbcdn.net URLs containing /live-dash/ and passes a flag into catalog construction. The observed flag is not sufficient to prove recording behavior. Parser support does not establish which live streams IDM successfully captures or how its final output handles every Period transition.

## UDM boundaries at the initial investigation

The current browser parser rejects dynamic MPDs and more than one Period. The native admission and live dispatcher support HLS only. Recorded DASH assembly joins parts within each selected track before muxing; it has no Period-level output assembly. Removing these rejections alone would not implement the requested behavior.

This paragraph records the investigation baseline, not the latest implementation. Subsequent work added recorded multi-Period assembly, native single-Period live DASH capture and [browser handoff](browser-live-dash-handoff-20261028.md). Live recordings now have native refresh, availability checks, concurrent track capture, Stop/finalization, revision recovery and reusable assembled-output receipts. These changes have build evidence but no runtime acceptance under the current testing deferral. [Next-Period selection](native-dash-period-transition-20261028.md) exists; automatic multi-Period live session capture and joined publication remain unfinished. The browser restriction on live multi-Period catalogs remains.

## Implementation work required

1. Preserve explicit Period identities, start/duration and per-Period initialization/index resources in the capture contract. Resolve inherited addressing within each Period, and retain selected language, role, quality and codec identity across transitions.
2. Download and checkpoint resources by Period and representation. Assemble each Period independently, align its selected audio/video timestamps, then join the ordered outputs. Define explicit handling of gaps, overlaps, changed track layouts and incompatible codecs; do not silently select only the first Period or concatenate initialization boxes as media.
3. Extend refresh compatibility and recovery to include Period structure. Changed signed URLs must not invalidate equivalent resources, while changed identities or byte ranges must prevent stale-byte reuse.
4. Add a native dynamic-MPD refresh path with bounded parsing, timeline expansion, segment availability and deduplication. Keep recording after the browser closes; reuse native cancellation, origin-scoped credentials, speed limits and durable recovery. A browser-only refresh loop would not provide the requested desktop behavior.
5. Integrate stop/finalize and dynamic-to-static transitions with existing recording UI. Distinguish a growing recording from a finite exact-size download; a changing presentation cannot truthfully claim a fixed final size in advance.
6. When runtime testing resumes, verify complete output across multiple Periods, independent audio/video timing, changing initialization data, sliding windows, refresh errors, interruption recovery, manual stop and presentation end. Include reference comparisons before claiming behavioral parity.

## Delivery status at the initial investigation

This investigation changes the implementation plan, not the application payload. The successful native 1.0.4 / extension 0.63.57 installer remains current; no redundant rebuild or installation was performed. Runtime testing remains deferred. Full parity is not complete.
