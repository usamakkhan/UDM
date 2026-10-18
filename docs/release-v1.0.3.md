# UDM 1.0.3

UDM 1.0.3 refines browser-captured media confirmation and error reporting. It pairs the desktop update with browser integration 0.63.17.

## Changes since 1.0.2

- **Edit destination before media preparation finishes:** Choosing Start while tracks are still downloading can now save a new folder or file name without cancelling the existing capture. UDM retains the original staging area, then publishes the verified result to the selected destination.
- **Accurate list display during confirmation:** The main list displays and sorts by the verified final media size while the confirmation dialog is open. The refresh is guarded so it cannot reopen dialogs or repeat capture presentation work.
- **Clearer track failures:** When one video/audio track fails and the other is stopped, UDM preserves and reports the initiating HTTP or transport error instead of replacing it with a cancellation error.
- **Coverage:** Added validation for in-flight destination edits, transactional failure rollback, modal size presentation, concurrent media capture, and both video-first and audio-first track failures.

The release is distributed as one Windows setup executable containing the desktop application, browser integrations, driver runtime, and bundled media tools.
