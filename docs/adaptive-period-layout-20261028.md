# Period-aware adaptive transfer layout

`AdaptiveTransferLayout.hpp` centralizes segment enumeration for recorded adaptive transfers. The existing worker loop now consumes its part records, track descriptions and resource bindings. Legacy selections preserve track/segment ordering, numbered part filenames, size limits, normalized selection identity and per-part binding JSON.

For the new DASH Period contract, enumeration is ordered by Period, track and segment. The layout keeps the mapping from Periods to tracks and from tracks to parts, resolves validators from the owning Period, collects selected origins, and creates a selection digest from the complete Period structure. Per-part bindings add Period identity/timing, track index and media clock. The aggregate segment limit remains 10000. The layout describes paths but does not create or delete files.

This supplies the worker layer with the information required for Period-specific downloading without duplicating the retry, pacing, cancellation and byte-validation loop. It does not enable multi-Period admission: the current transfer entry still rejects that plan until independent assembly and final joining are implemented. Credential scoping and address-refresh admission also still need to consume the nested structure before capability advertisement.

## Build

All production compilation and setup results are recorded in `validation/adaptive-period-layout-build-20261028.json`. Browser 0.63.59 is unchanged. Runtime and recovery testing remain deferred by the user's instruction. The candidate is unsigned, uninstalled and unpublished. Full multi-Period and live DASH behavior remains incomplete.
