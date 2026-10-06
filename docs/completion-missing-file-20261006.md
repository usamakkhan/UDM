# Completion-dialog missing-file verification — 2026-10-06

The standard completion-dialog runner now passes 51 checks, including opening a downloaded file that was subsequently moved. It observes the warning, dismisses it automatically, and verifies that the completion dialog remains open. Existing geometry checks at simulated 96/144/192 DPI, read-only source/destination fields, button availability, suppression persistence, and explicit reopening also pass.

The repeated stall was in the fixture's dismissal logic. The warning's button was labeled OK but its actual control ID was 2. The observer posted IDOK (1), which did not dismiss it. A native stack capture showed the UI thread waiting normally in the message-box loop and no observer thread remaining. Posting the actual control ID with its button handle released the stalled diagnostic run. That externally assisted run is not the acceptance run.

The fixture now discovers the actual button and posts its ID and handle. It also executes after the MFC message loop starts, via OnIdle, rather than during InitInstance. Moving to OnIdle alone did not resolve the failure. The standard PowerShell runner enables the missing-file case and restores its caller's environment afterward.

A fresh, uninterrupted run passed all 51 checks in approximately three seconds. The 96-DPI render was visually inspected. This is a test-harness fix and verified existing product behavior; no production GUI change was needed. Actual OLE drag/drop, external shell action launches, scanner-expanded layout, and physical mixed-monitor DPI remain unverified.

[Portable validation receipt](validation/completion-missing-file-20261006.json). Local diagnostic stacks and both assisted/unassisted runs are retained under `candidates/completion-message-loop-20261006`. No fixture process remains running.