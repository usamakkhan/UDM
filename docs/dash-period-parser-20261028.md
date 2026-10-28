# DASH Period parsing groundwork

The browser parser now separates presentation layout from individual Period catalog expansion. Existing single-Period downloads use this layout resolver; the new `dashPresentation` entry point produces separate catalogs and timing records for native multi-Period integration. It is not yet wired to browser handoff.

The layout preserves each source Period index and optional ID, resolves omitted starts from preceding known ends, and resolves omitted durations from a following explicit start or the presentation end. Empty Periods are ignored rather than replacing their zero duration with the whole presentation length. Duplicate IDs, invalid durations, unresolved multi-Period timing and inconsistent bounds are rejected. The implementation currently rejects gaps and overlaps pending explicit native timeline assembly. Limits are 128 source Periods, seven days of timing, and 4 MiB of serialized multi-Period catalogs. Existing XML and individual segment limits remain.

The timing model follows the [DASH-IF timing guidelines](https://dashif.org/Guidelines-TimingModel/): Period boundaries can be explicit or inferred from adjacent boundaries, and zero-duration Periods are ignored. This implementation does not claim full DASH-IF conformance. Address inheritance and initialization/index parsing run independently for each Period. Native handoff continues to require one nonempty Period so unsupported assembly cannot silently omit later media.

The retained reference parser recognizes `OnDemand` as recorded media; UDM now accepts that exact legacy spelling alongside `static`. Dynamic and Live values remain unsupported until the native refresh path is implemented.

## Build and remaining work

Both browser families are version 0.63.58. JavaScript syntax checks passed, and media.js mirrors match byte for byte. Native source is unchanged; packaging reuses the compiled native 1.0.4 production payload. The installer is staged, unsigned, uninstalled and unpublished. Runtime tests are deferred at the user's request.

Native Period selection matching, resource checkpointing, separate assembly, final joining, address refresh and live DASH remain unfinished. The new catalog API is groundwork for that work, not completed multi-Period download support. Build evidence is in `validation/dash-period-parser-build-20261028.json`.
