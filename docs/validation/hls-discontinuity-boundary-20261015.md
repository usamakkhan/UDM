# HLS delta discontinuity boundary

Fixed delta merging when the first skipped segment carries a leading discontinuity. Retained segments now preserve the pre-tag discontinuity value, and saved recordings serialize it as optional metadata. Validation accepts either that retained boundary or the segment epoch when a rolling playlist has absorbed the tag into its base; unrelated bases remain rejected. Legacy saved records remain readable.

Validation: a focused native executable compiled the current HlsPlaylistChecks and HlsRecordingChecks, plus explicit recovery round-trip, legacy-read and malformed-boundary cases. It passed 124 checks with zero failures on 2026-10-15. The equivalent new recovery assertions are also saved in HlsRecordingChecks.hpp. The probe linked existing core objects and is not a full rebuilt-app or live-CDN validation.

At the initial focused validation, the previously built desktop executable predated this fix. Installed-app verification remains required.

## Full rebuild and native suite follow-up

On 2026-10-15, `native/build.ps1` succeeded from this checkout in a fresh temporary output root, producing the desktop app, native host, monitor, and test executable. The newly built `Udm.NativeTests.exe` ran against a separate fresh temporary data root with FFmpeg/FFprobe available: **2,628 passed, 0 failed**. The suite included the leading-discontinuity delta cases, conflicting-base rejection, recovery round-trip, live output decoding, and broader transfer checks. This verifies compilation and fixture behavior of the current source, but it does not replace installed-app or live CDN acceptance.
