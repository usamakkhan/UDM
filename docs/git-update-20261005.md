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
