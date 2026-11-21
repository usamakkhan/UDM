# UDM 8.40: publication directory checks outside the model lock

UDM previously repeated pinned directory metadata checks while holding its shared Manager mutex immediately before beginning a file-publication commit. A slow filesystem operation at that point could hold up unrelated model access and UI refresh.

The check now runs before acquiring that mutex. Existing pinned directory handles retain their rename/deletion protection. After reacquiring the lock, publication still revalidates catalog membership, destination, replacement inputs and project-root policy before beginning commit. The second filesystem validation after writing the recovery intent remains. Final data flushing, recovery-intent persistence, rename and catalog acknowledgement are preserved.

Two new project-directory cases hold the post-validation boundary: an independent model save remains possible, and a destination changed during the hold is rejected before journal or output publication. The maintained preparation fixture passed 87 checks. All 803 focused checks pass across preparation, ownership, commit, recovery and completed-checkpoint suites. Five native products and five fixtures are built in a separate 8.40 output area; the canonical 8.39 objects and installed app are retained. [Qualification and cleanup](validation/publication-directory-lock-8.40.json).

Extension source manifests advance together to 0.72.4. No browser JavaScript change, installer, installation, commit, push or release is included. This addresses shared-model responsiveness; no reduction in the retained D: finishing times or universal IDM speed parity is claimed.

## Follow-up: recovery transaction ownership

The source audit identifies a remaining broader lock path. `Manager::recoverReplacements` retains a recovery lock while enumerating journals, hashing bytes and reconciling persisted publication state. Resume, Remove and Site Grabber project admission call it from outer model-lock scopes; replacement publication also invokes predecessor recovery from its own scope. A local unlock inside recovery cannot release those outer recursive leases. This requires a coordinated change across entry points, not removal of a single lock statement.

Next, establish reservation-backed ownership of the selected job tree and its destination/staging/previous paths, snapshot the exact journal/commit identity, perform filesystem work outside all model leases, then revalidate before applying recovered state and accepting its catalog checkpoint. Preserve journal-removal ordering and legacy recovery behavior. Held-IO tests must exercise destination mutation, removed records, competing owners, shutdown and failed persistence. [Audited source locations and hashes](validation/finishing-8.40/recovery-lock-audit.json). No recovery-performance fix is claimed for 8.40.

## Runtime recovery-lock baseline

The isolated `recovery_lock_probe.cpp` reproduces the lock hold through both direct recovery and public Resume. Each case starts from a known legacy catalog-save failure, reaches the actual catalog temporary-file flush, holds the completion boundary and probes the model mutex from a second thread. Both probes find the mutex unavailable; after release, both recoveries preserve the exact fourteen-byte file and complete correctly. This confirms the baseline responsiveness problem without asserting a storage root cause. The held boundary is a controlled callback, not an injected kernel delay.

The probe explicitly reports `recoveryResponsivenessFixed=false`; its two observations are not added to the 803 passing product checks. Generated fixture files, probe executable and compiler object were individually removed after boundary/link/hash checks. [Baseline result](validation/finishing-8.40/recovery-lock-probe.json).

## Indirect recovery entry points

The expanded audit also traces File Info Start, schedule creation, schedule revision and Configure through `recoverBeforeConfigure`, each under an outer model lock. The runtime baseline now includes File Info Start. All three observed entry paths retain the mutex at the held catalog-flush boundary while preserving exact recovered bytes. File Info correctly reports that the file was recovered at its previous destination, rather than silently changing the recovered destination or starting a new transfer. This behavior must survive the responsiveness refactor. Schedule/create/revise and Configure are source-audited here, not runtime-qualified by the three-case probe. [Expanded baseline](validation/finishing-8.40/recovery-lock-workflows.json).

## Journal-backed recovery baseline

The probe now covers six cases: direct recovery, Resume and File Info Start, each for legacy repair and an actual publication interrupted after rename with its recovery journal retained. All six preserve exact output bytes but retain the model mutex at the held catalog-flush boundary. Both File Info cases preserve the existing recovered-destination warning. The journal cases use the real publication transaction and its interruption callback, rather than a hand-written synthetic journal. This guards against qualifying only the simpler legacy path. [Six-case evidence](validation/finishing-8.40/recovery-lock-journals.json). Temporary probe binaries, object and fixture payloads were individually removed. No functional recovery change or version increment is claimed.
