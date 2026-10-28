# Recovering an assembled live DASH output

Build status: all five production executables and the installer compiled successfully. The candidate is unsigned, staged, uninstalled and unpublished. Runtime behavior remains unverified under the testing deferral.

The live DASH assembly path now records a completed output for reuse after an interruption between assembly and publication. A DPAPI-protected `assembled.json` binds the full reconciled sealed recording index to the generated output filename, byte count, SHA-256, capture start/duration and generated temporary-input names. Only sealed recordings are eligible for reuse.

On retry, the normal recording reconciliation still runs. The receipt must match that exact index, use supported schema versions and contain bounded, generated local filenames. Its interval must match the sealed capture interval. The output is opened without write/delete sharing, rejected if it is a directory or reparse point, checked for the saved size and hashed through that same open handle. A successful match returns the existing output without invoking FFmpeg again. The job records `AdaptiveAssemblyReused` for diagnostics and continues through its existing expected-hash and publication paths.

Missing, stale, malformed or unverifiable cached output falls back to assembly from retained media. Cancellation propagates rather than triggering a rebuild. Receipt or temporary-file redirection is an explicit error. No old output, source segment or temporary assembly input is deleted by cache recovery. A successful replacement assembly receives a new generated filename and atomically replaces the receipt.

New output still passes the existing stream and duration probes. It is then opened under a write/delete-excluding lock, flushed and SHA-256 hashed using that handle before the protected receipt is atomically committed. The output is hashed once for checkpoint creation. Unsealed direct assembly callers receive the verified hash but do not create a reusable receipt. Receipt envelopes are limited to 64 KiB and decoded recovery content to 32 KiB.

This completes the reusable single-Period output stage. Automatic capture of subsequent Periods, a session journal coordinating them and joined final publication remain unfinished. Runtime interruption/recovery and media checks are deferred at the user's request. Browser 0.63.62 is unchanged. Compilation and packaging evidence is recorded in `validation/native-dash-assembly-recovery-build-20261028.json`; the candidate remains uninstalled and unpublished.
