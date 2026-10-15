# HLS delta discontinuity boundary

Fixed delta merging when the first skipped segment carries a leading discontinuity. Retained segments now preserve the pre-tag discontinuity value, and saved recordings serialize it as optional metadata. Validation accepts either that retained boundary or the segment epoch when a rolling playlist has absorbed the tag into its base; unrelated bases remain rejected. Legacy saved records remain readable.

Validation: a focused native executable compiled the current HlsPlaylistChecks and HlsRecordingChecks, plus explicit recovery round-trip, legacy-read and malformed-boundary cases. It passed 124 checks with zero failures on 2026-10-15. The equivalent new recovery assertions are also saved in HlsRecordingChecks.hpp. The probe linked existing core objects and is not a full rebuilt-app or live-CDN validation.

The previously built desktop executable predates this fix and has not been installed. A full rebuild and installed-app verification remain required.
