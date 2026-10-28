# Native DASH URL template expansion

The native MPD module now expands RepresentationID, Bandwidth, Number and Time identifiers. Escaped dollar signs are handled in a single pass, so literal dollars and replacement values are never interpreted again as template syntax. Numeric values retain signed 64-bit integer precision without a floating-point conversion. Decimal, octal and upper/lower hexadecimal zero-padding are supported with a bounded width of 32 characters.

Missing values, negative numeric values, unknown identifiers, malformed format expressions and formatting on RepresentationID are rejected. An omitted value remains distinct from zero. Input and expanded references are limited to 32768 bytes and control characters are rejected. Representation IDs are inserted literally without URL encoding, preserving their manifest spelling; URL resolution and request validation remain separate responsibilities.

The addressing model is described in the [DASH-IF timing guidelines](https://dashif.org/Guidelines-TimingModel/#addressing). This implementation is original UDM code and does not copy IDM implementation code.

## Remaining integration

This utility is compiled into the native MPD module alongside timeline expansion. Live downloading is not enabled by this change. Inherited BaseURL/SegmentTemplate selection, availability calculations, refresh scheduling, durable recording and browser admission still need integration. Dynamic browser manifests continue to be rejected. The current recorded-DASH browser path is unchanged.

Production build and packaging evidence is recorded in `validation/native-dash-template-build-20261028.json`. Runtime and regression tests are deferred at the user's request. The staged installer is unsigned, uninstalled and unpublished; successful compilation does not establish complete-file or IDM parity acceptance.
