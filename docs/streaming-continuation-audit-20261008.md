# Streaming continuation audit — 8 October 2026

Reviewed against source commit 927d4f4. This is a newly identified compatibility risk, not evidence that an observed IDM download uses this response shape.

The UMP interoperability research describes a media part continuing across HTTP responses, with a new header and another part carrying the remainder: https://github.com/gsuberland/UMP_Format/blob/main/UMP_Format.md (Partial parts). Current applicability to the captured SABR endpoint must be established with a bounded transcript or live capture; historical UMP descriptions must not be treated as a complete current SABR specification.

Current implementation has two concrete boundaries in native/Streaming.cpp:

1. Reader::exact throws on HTTP EOF before the declared part length. Reader is recreated for each response, so there is no retained partial-part parser state.
2. End-of-response handling throws when a selected Pending segment remains. Repeated segment headers also fail as overlapping identifiers. The recent side-track fix removes only skipped/unselected entries.

The existing media fixture fragments each response into 13-byte reads. That verifies network-read fragmentation inside a response, not continuation across distinct responses. These are different acceptance cases. Passing the 2,572-check suite does not cover the latter.

Next implementation gate: construct a two-response transcript with explicit identity, offset, declared-length and continuation semantics, establish which request state asks for the remainder, and reproduce current rejection. Then add bounded continuation state with checks for changed format/video identity, mismatched offsets, duplicate bytes, truncated metadata, cancellation, and permanent EOF. Compare final video/audio bytes with an uninterrupted transcript. Do not simply remove the pending-segment check: that can silently reuse a header or concatenate unrelated bytes.

Selected compressed segments remain rejected by field 7. The GoogleVideo MediaHeader reference identifies that field as compressionAlgorithm (https://ytjs.dev/googlevideo/api/exports/protos/interfaces/MediaHeader). Decompression requires independently verified algorithm and length semantics and bounded decoded output; transport-level content decoding is not sufficient.

Browser activation check: the available computer-control inventory exposed only Codex in-app and MCP-app browsers, with no tabs and no Edge connection. Existing Edge extension activation therefore remains unverified. Installed executables and personal browser sessions were not changed by this audit.

## Implemented continuation at complete-part boundaries

A focused two-response fixture reproduced the existing failure: `Streaming response ended inside a media segment.` The selected video initialization segment now keeps its pending file and byte count between responses. Each individual UMP part must still be complete. Duplicate pending headers, changed video identity, declared-length overflow and premature terminators remain errors. Receiving selected bytes counts as progress; eight consecutive responses with neither selected bytes nor completed coverage stop the request loop.

Current GoogleVideo source creates a fresh UmpReader per response while its SabrSegmentBuffer is retained by the stream instance. That supports retaining selected segments across complete-part response boundaries, but does not prove continuation inside a truncated UMP part or establish every historical repeated-header convention. Sources inspected: https://github.com/LuanRT/googlevideo/blob/main/src/core/SabrStream.ts and https://github.com/LuanRT/googlevideo/blob/main/src/utils/SabrSegmentBuffer.ts. UDM's implementation and fixtures were written locally; no reference implementation was copied.

Validation: 12 new checks plus 86 existing media, parallel-stream and audio-only checks passed, 98 total. Output bytes match the uninterrupted video/audio fixture. Regression also covers the existing side-track handling, media assembly, requested quality verification and parallel cancellation paths. A harness compilation attempt needed its existing test-header dependencies added; the corrected build and run passed. Portable evidence: [validation/streaming-continuation-20261008.json](validation/streaming-continuation-20261008.json). Raw baseline and passing results: `candidates/stream-continuation-20261008`.

This source candidate is not installed. Full native suite and public-site acceptance were not rerun for this change. Partial UMP parts, repeated continuation headers and selected compressed segments remain unsupported. The installed desktop remains the previously validated 8 October build.
