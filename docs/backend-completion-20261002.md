# Backend consolidation — 2 October 2026

The backend consolidation is validated. Complete IDM parity remains unfinished.

The repository's historical and candidate records distinguish implemented features,
staged fixes, installed binaries, and unverified IDM comparisons. They do not establish
complete parity. This pass consolidates the tested native backend improvements into
a reproducible build and validates them before integration into the workspace.

## Integrated source

The native 0.84.0 source comes from the existing `native-115-ftp-login-recovery`
candidate. Integration added 40 files and updated 37, covering the production
dependencies and selected backend regression harnesses. It preserves the workspace's
additional live-HLS stop test. Unrelated standalone debug and UI test fixtures remain
in the candidate directory. Existing files were backed up under
`.media-cache/backend-consolidation/20261002-073254` before replacement.

The consolidated changes include HTTP connection/probe recovery, FTP authentication
handling, durable media admission, queue retry and completion policies, synchronization
recovery, backup, and restore. Shared application models and their UI callers move
together so that the complete application still compiles.

The first regression run found that an idle queue inserted a false
`DailyRunUntilComplete` property and unnecessarily rewrote its catalog. The fix clears
that flag only when it is active. A dedicated regression also verifies that finishing
an active daily cycle still persists the change.

## Initial consolidation verification

- Full native application, host, monitor, setup helper, and test executable build:
  succeeded with compiler warnings.
- Native regression suite after the idle fix: **2,545 passed, zero failed**.
- Twelve focused queue/synchronization/recovery/backup/restore cases: **241 passed**.
- Dedicated idle regression: **five passed**, including a run through the integrated
  workspace test command.

The unique combined total is **2,791 passing checks**; repeated idle validation is
not counted twice. See [native results](evidence-backend-20261002/native-suite.log),
[focused results](evidence-backend-20261002/focused-summary.json),
[idle results](evidence-backend-20261002/idle-summary.json), and
[integrated source hashes](evidence-backend-20261002/source-changes.json).

The subsequent [synchronization probe correction](backend-synchronization-probes-20261002.md)
adds 83 regression checks and validates a fresh complete run of **2,874 passing
checks**. This is the latest backend result; the initial consolidation evidence
above remains the historical record for that earlier source snapshot.

Use `native/test-backend.ps1` as documented in [the build guide](../native/README.md).
The native source version is 0.84.0; this pass does not publish a release or qualify
an installer. Existing packaging scripts require an explicit `-Version 0.84.0` after
building matching release binaries.

## Remaining scope

These local regression checks do not establish exact IDM behavior, full browser or
driver capture parity, public-site compatibility, speed equivalence, or physical
disk/device and sleep recovery. The existing [research reconciliation](idm-research-current-status-2026-09-29.md)
retains the broader acceptance gaps. Installer acceptance, browser integration, and
visual matching require their own validation; they are not implied by this backend
test result.

Build output uses the current user's temporary directory on C: to keep generated
files outside the checkout. Personal catalogs and installed applications are not
test fixtures.

## Repository cleanup

The 2 October cleanup removed 90 Git-ignored generated files, totaling
4,099,484,094 bytes (3.82 GiB): obsolete installers older than 0.77.0, native
compiler objects/resources, and development debug symbols. Every path was checked
against the workspace boundary and Git's tracked and ignored file lists before
deletion. No tracked source or evidence was deleted. The 0.77 rollback installer,
current release, newer candidates, toolchains, download data, and source backups
were retained. The deletion manifest is stored locally at
`.media-cache/cleanup/cleanup-20261002-072356.json`.
