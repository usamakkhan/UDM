# Coherent history reads, 8.71 / browser 0.72.35

Normal main-window history refresh previously read navigation separately, changed tree controls, then acquired a blocking Manager lock to capture the matching rows. A catalog writer could stall that later read after navigation had already changed. Hidden-window status observation also waited on the model, and even advisory selection could enter blocking stale-row validation while history was deferred.

## Read and apply boundaries

`HistoryReadSnapshot.hpp` captures navigation, effective filter, settings, storage warning, selection/focus identities, quota flags, compact row records, selected search fields/cache decisions and status observations under one try-lock. It computes the existing nearest surviving navigation ancestor before filtering, including category, project and queue removal. Only the filtered cohort gets row/search snapshots. Private transfer plans and journals stay outside list records; unchanged rows and positive/negative search results remain reusable.

No control messages, sorting, protected-header decryption or filesystem operations run in the snapshot helper. The caller applies its navigation and rows after releasing the model lock. Hidden windows capture settings/status observations without rebuilding history or invalidating its visible search cohort.

If the model is busy, the helper returns no snapshot and leaves both row and search caches untouched. Full refresh retains the existing tree, cells, selection and filter, restores its reentrancy flag through a scope guard, and leaves history deferred for a later regular tick. It does not repeatedly post an immediate refresh message while busy. Initial pending-browser-offer observation also uses a try-lock, and busy offers remain available for later delivery.

Advisory command refresh reads UI selection without entering deferred-history validation before its try-lock. Ordinary command selection retains current-model membership validation, including rejection of a removed job still visible in hidden deferred controls.

## Qualification

All five native production resources report **8.71**; both source manifests are **0.72.35**. Bridge is the only rebuilt core unit; Transfer remains identical to 8.70. [Build identities](validation/history-read-8.71/build.json).

Thirteen backend suites on C: and D: pass **1,780 assertions**. The new history suite exercises coherent values, immutable old snapshots, compact-record reuse, inactive-search projection, positive and negative search reuse, existing navigation cohorts, removed category/project/queue fallbacks, hidden observation, lock contention, unchanged busy-cache state, and the distinction between an empty offer set and a busy read. Existing search-cache, tooltip, modal-size, writeback, checkpoint, publication and recovery suites also pass. [Backend assertions](validation/history-read-8.71/backend-checks.json).

The real MFC main-window fixture passes **37 assertions** on its own separate Windows test desktop. New checks hold a catalog writer while it changes a project and filename, call actual full refresh, verify that the old controls remain coherent and that advisory selection returns, then verify catch-up to the surviving Grabber ancestor. Actual category and queue removal, programmatic empty-cohort selection, hidden/visible catch-up, and authoritative rejection of a deleted deferred row also pass. Earlier tooltip/toolbar and modal-size checks remain included. The fixture exits normally and closes its owned desktop without switching desktops or injecting user input. [UI assertions and identities](validation/history-read-8.71/ui-checks.json).

All **29 owned fixture roots** and **28 temporary test executable/object files** were removed after recording evidence. Qualified production/core files, installed 8.68 files, rollback and both personal catalogs retain their hashes. [Cleanup](validation/history-read-8.71/cleanup.json), [preservation](validation/history-read-8.71/final-preservation.json).

The first new history fixture stopped before any assertions because its synthetic segment entries did not satisfy the existing saved-download validator. Correcting them to valid segment objects and recompiling only that test fixture allowed qualification to continue; the initial failed result is retained separately and is not counted as passing evidence. [Initial fixture result](validation/history-read-8.71/initial-fixture-run-failure.json).

An existing paced-cancellation test also failed because it treated a 20 ms future timeout as proof that the finish thread had entered stopping. Under compiler load the held IO could be released first, legitimately allowing another queued operation. The fixture now waits for the actual stopping state before release, using its existing test-access hook; production writeback code is unchanged. The original failed assertions are retained separately and are excluded from passing counts. [Cancellation fixture result](validation/history-read-8.71/initial-prefinish-fixture-failure.json).

## Delivery and next work

**8.71 is a qualified development candidate; installed UDM remains 8.68 / manifest files 0.72.32.** No installer, installation, browser reload or new download/IDM timing comparison is claimed. This is owned component qualification rather than operation of the user's browser or IDM.

This closes the normal history snapshot's split/blocking read boundary. It does not make the entire timer or capture workflow nonblocking: queue dispatch/tick work, initial browser-review handling, later confirmation/review reads, authentication persistence, transfer-event handling and authoritative commands still have independent synchronous dependencies. A writer can contend at those other boundaries after a snapshot has completed. Next separate timer work and remaining display-only discovery from durable mutations, without losing accepted offers, authentication prompts or command validation. Then obtain fresh live browser/File Info and IDM timing evidence when desktop control is authorized.
