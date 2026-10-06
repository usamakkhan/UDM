# GUI and CLI delivery candidate — 2026-10-06

Built a new native 0.84.0 / browser 0.62.8 installer containing the CLI `/q` and `/h` additions and the wrapping, scrollable scanner diagnostic field. All four native delivery programs were rebuilt from current source. The 260 native source hashes and 145 installer input hashes remained stable.

Validation:
- Full native suite: 2,547 passed, zero failed, empty stderr. The initial attempt lacked FFmpeg in its fresh folder; supplying the existing helper allowed the unchanged test executable to complete successfully.
- Exact staged app/host in isolated browsers: 10 Edge and 9 Firefox generated indexed-DASH checks passed. Both temporary host registrations were removed.
- Exact staged app CLI: 10 lifecycle/guard checks passed. No RAS connection was disconnected.
- The earlier 98-check completion GUI fixture remains source-level evidence for the scanner display; it is separate from the freshly rebuilt packaged app.

Installer: `candidates/release-gui-cli-20261006/project/installer-out/UDM-0.84.0-Browser-0.62.8-Setup-x64.exe` (61,143,026 bytes).
SHA-256: `7A002F2B5701955A6FD14640BA220039992A3FB8596392B683E2A6343D8DAABE`.

The installer was compiled but not executed, signed, or published. The installed UDM app and browser host are still running. Computer Use returned `GetCursorPos failed: Access is denied (0x80070005)` while attempting to show UDM, so no forced shutdown or binary replacement was performed. Deployment needs a verified normal shutdown first. Clean-machine install/upgrade/rollback, public-site parity, actual RAS hang-up, and the remaining GUI/driver requirements are still open.

[Validation receipt](validation/gui-cli-package-20261006.json).