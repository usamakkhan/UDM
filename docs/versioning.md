# UDM versioning policy

User direction, 2026-10-29: every functional improvement must advance the application version. Do not accumulate successive improvement batches under the same version.

- Bump the application version for each separately delivered functional improvement batch, before building it.
- Use a new minor version for a new feature, a new patch version for a fix or refinement, and a major version for a breaking change. Do not hold version numbers down artificially.
- Keep all three definitions in native/Version.hpp synchronized. All five production binaries must use that version; any later release installer must match.
- Add a matching CHANGELOG.md entry explaining the changed behavior.
- When extension code changes, also advance both Chromium and Firefox manifest versions together. Native-only changes do not require an unrelated extension bump.
- Preserve historical build records and artifact names. Never relabel an older executable as the new version.
- Record source, build, test, installation and publication status separately; a source bump does not establish a release.

User direction, 2026-10-30: continue native development and complete native builds, but do not create setup packages for each improvement. Keep tests deferred and maintain changelog/build records for a later major release. Do not infer a release number, publish, install, or generate a new installer from a successful development build alone. Existing setup artifacts remain historical.

Version 1.2.3 records synchronization recovery publication-state and review-required retry fixes. Version 1.2.4 adds browser-recapture retry exclusions and includes those fixes. A 1.2.4 installer has been built; testing, installation and publication remain separate steps. Historical 1.2.2 installers remain unchanged.
