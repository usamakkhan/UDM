# Source checkpoint — 2026-10-05

This checkpoint records the accumulated working-tree changes since the previous Git release commit, including native 0.84.0 and browser 0.62.8. It is not a new release or deployment. The [candidate history](current-staged-candidate.md) distinguishes installed, packaged and tested revisions.

Maintained source, regression fixtures, build scripts, installer modules, technical Markdown, research tools, and required static dependency libraries are versioned. The libraries retain their headers, licenses, rebuild instructions and recorded provenance.

Local `candidates/`, `build-source-*/`, raw `docs/evidence-*/` and `docs/research-*/` directories, and generated dependency packaging metadata are ignored. Existing tracked files remain tracked. No candidate, personal catalog, backup or raw evidence was deleted. References to these local artifacts in historical reports are not portable repository links.

Fresh validation for this checkpoint:

- All 49 JavaScript regression scripts passed using `node --test --test-concurrency=4` with the root `tests/*.test.cjs` files except `native-protocol.test.cjs` and `prepare.test.cjs`.
- `node tests/prepare.test.cjs` passed separately, checking repeated extension preparation in an isolated copy.
- PowerShell files parsed without errors; local quoted C++ includes and backend test manifest sources were present.
- Both browser manifests report 0.62.8.

The native protocol script remains pinned to an older 0.78.0 host and was not run against the installed app. The full native build, native regression suite, and installer lifecycle were not rerun for this Git checkpoint. Prior candidate results retain their original scope and are not fresh validation of this commit. In particular, the October 2 server-retry full run had a timing failure in live HLS stop/save, followed by a passing focused rerun; it was not a clean full-suite acceptance.

## Post-RC1 source and cleanup update

The next source checkpoint includes transactional individual Pause, the completion-dialog icon/Unicode fixes, regression fixtures, the completion test runner, and refreshed installed/staged/published status. The optional warning observer uses an atomic flag and cached window captions. A small versioned completion-dialog metadata file replaces the runner's dependency on an ignored local research directory.

Removed 28 regenerable `.obj` files from the completed RC1 build cache, reclaiming 390,710,713 bytes (about 373 MiB). Paths were checked to stay in `candidates/github-v0.84.0/build/out`, without reparse points. The local deletion manifest retains file names, sizes and SHA-256 values at `candidates/pause-transaction/cleanup-objects.json`. Release executables, debug symbols, installers, source snapshots, evidence, backups and personal data were retained. Added ignore rules for object files and other native compiler intermediates. No tracked application or dependency file was identified as disposable.

Pause validation passed 55 focused and 12 isolated application checks. The broader native run had 2,546 passes and one scanner subprocess failure; the subsequent scanner-focused rerun passed 293 checks. See [the detailed record](backend-pause-transaction-20261005.md) for the retained failure and scope. This checkpoint updates source; it does not replace the existing GitHub RC1 release or installer.

Fresh completion validation passed all 50 checks after rebuilding against the current backend. A second 50-check run using the runner's new default, versioned reference file also passed. Evidence: `candidates/pause-transaction/completion-current/results.json` and `completion-portable/results.json`. The opt-in missing-file modal diagnostic was excluded. PowerShell parsing, backend-manifest source checks and Git whitespace checks passed.
