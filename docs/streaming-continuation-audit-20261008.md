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
