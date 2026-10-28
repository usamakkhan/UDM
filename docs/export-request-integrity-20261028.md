# Preserving request meaning during export

Source inspection found two cases where IDM-format output could not round-trip safely:

- Separately stored POST bodies use `bodyFile`, `bodySize` and `bodySha256`, with no inline `body`. The export path read only `body` and could therefore emit an empty `pd:` field for a nonempty stored form. It now rejects that case before writing the destination.
- The serializer allowed non-UTF-8 bytes in request fields, while the EF2 importer requires valid UTF-8 unless explicitly reading the legacy format. Record validation now rejects invalid UTF-8 URL/header/form text instead of producing an export that cannot be imported unchanged.

Actual empty inline POST bodies remain supported, including the explicit empty `pd:` field. Existing limits, field validation and plain-text cookie/form opt-in remain unchanged. The write path still constructs and validates the complete output before `atomicText`, so unsupported requests do not replace an existing destination.

The export dialog now accurately distinguishes address-only text output from catalogs. It states that separately stored large form bodies require browser recapture after catalog import. Catalogs already omit those external bodies and retain a recapture marker; this change does not pretend that selecting encrypted credentials packages them.

This fixes data loss within the implemented export subset. It does not add large-form EF2 support or prove interoperability with the installed IDM version. Those require separate implementation and runtime evidence.

## Build scope

`validation/export-request-integrity-build-20261028.json` records the production build and installer. Runtime tests and real IDM round trips remain deferred at the user's request. Browser 0.63.55 is unchanged. No installed profile or downloads are modified.
