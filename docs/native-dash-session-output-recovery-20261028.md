# Recovery of joined live DASH recordings

The final joined MP4/M4A now has a protected `session-assembled.json` receipt, separate from the first Period's `assembled.json`. Retrying after interruption between assembly and publication can reuse the complete joined file instead of running the final FFmpeg concat again.

The receipt binds the selected output contract, ordered Period folders, digest of every reconciled recording index, absolute capture interval and probed stream signature. Recovery still reconciles each Period and verifies its assembled output and stream compatibility. It then requires the saved joined file to match its recorded length and SHA-256 through an open handle that excludes writing and deletion during verification. Changed selection, timeline or resources invalidates the receipt. Missing or invalid output falls back to assembling a fresh file; cancellation propagates normally.

The joined output is probed for stream layout and duration, flushed and hashed before the encrypted receipt is atomically committed. Receipt paths reject reparse points, generated output and concat-list names are constrained, and the existing size bounds apply. Per-Period receipts remain independent. No source recording or previous candidate is deleted.

This avoids repeating the final join, but recovery still performs source verification and probing. It does not make a claim of decoded-frame completeness, and a crash before receipt commit can still require reassembly. Runtime interruption, playback and browser testing remain deferred at user request. The build is staged only, with no installation or publication.
