# Automatic prompt discovery, 8.73 / browser 0.72.37

Main-window refresh previously obtained separate blocking locks to drain legacy offers, find authentication, discover review receipts and revalidate confirmations. Legacy offers lived in a local vector after the drain. If authentication saving or another presentation then failed, an unpresented explicit offer could be lost. Seen markers were also inserted before dialog creation without rollback. Browser-batch discovery moved a cohort out of Manager and discarded it after a presentation exception.

## Discovery and reservation boundaries

`AutomaticPromptSnapshot.hpp` performs one try-lock read for current receipt ownership, confirmation candidates, ready review tokens and the eligible authentication candidate. A held model or File Info acknowledgement guard yields no snapshot and changes no pending offers. Discovery neither marks dialogs seen nor clears authentication. Only display identities, job references and review tokens cross this boundary; no decrypted credentials or media plans are copied.

The snapshot merges existing window-owned confirmation reservations with explicit legacy offers and new automatic candidates. It deduplicates a job while retaining explicit-request semantics, excludes removed jobs and delegates receipt-owned duplicates to their existing capture workflow. All snapshot validation/allocation finishes before legacy ownership transfers; the result is nothrow-movable, and MainWindow immediately moves its confirmations into window-owned storage that survives later refreshes.

Before each confirmation the caller tries to revalidate current membership, receipt ownership and status. A busy result leaves the reservation in place. Obsolete candidates are discarded. Successful presentation removes its reservation. Presentation exceptions retain that reservation and undo only the newly added seen marker; an older successful marker remains intact for an explicit legacy retry. This addresses failures between discovery and presentation without consuming requests that arrive later into an untracked local list.

Review discovery remains nondestructive. A successful discovery pass handles the existing recovery reset; a busy pass leaves the reset flag and seen set alone. Failed review creation rolls back its seen marker, allowing the still-pending receipt to be offered later. A normally dismissed review stays seen for the session, preserving existing behavior.

Browser batches use a separate try-lock FIFO handoff into a window-owned optional cohort. The existing cohort survives presentation exceptions and is retried before taking a later batch. Normal dismissal, including Cancel, releases that cohort. Automatic batch errors use status reporting rather than opening a repeated error message box. Request metadata stays with its original cohort.

These reservations cover the live application's ownership handoff. They introduce no new persistent queue format or cross-process crash guarantee; existing saved jobs and browser receipts remain the persistent recovery mechanisms.

## Authentication claim and explicit limits

Discovery selects only eligible, non-queue-origin authentication and rechecks current suppression and eligibility when claiming it. The claim uses a try-lock, then retains the existing durable save command. Save failure restores the exact in-memory pending flag before reporting the error. A successful save still precedes the existing login presentation. Tests prove that a failed authentication save preserves both the challenge and any already-reserved confirmation.

**Authentication claim persistence and the existing login dialog's preparation/presentation remain synchronous.** This increment does not move that save off the UI thread or finish its dialog-construction acknowledgement redesign. Actual confirmation/review/batch presentation also retains its existing authoritative model accesses. The new boundary removes automatic discovery waits and fixes reservation loss; it does not make every dialog operation nonblocking.

## Build and evidence

All five production resources report **8.73**; both source manifests are **0.72.37**. Bridge is the only changed core unit and Transfer is unchanged from 8.72. [Build identities](validation/automatic-prompts-8.73/build.json).

Seventeen backend suites on C: and D: pass **1,996 assertions**. The 31 new assertions per drive cover legacy deduplication/reservation retry, capture-only behavior, current receipt ownership, changed/removed candidates, atomic failure before legacy transfer, authentication save rollback and actual saved state, queue-origin/suppression policy, busy/decision-guard deferral, FIFO browser-batch metadata and larger reservation cohorts. Existing maintenance, task-signal, CLI, history/search, modal-size, writeback, checkpoint, publication and recovery suites also pass. [Backend assertions](validation/automatic-prompts-8.73/backend-checks.json).

The real MFC main-window fixture passes **79 assertions** on a separate owned Windows test desktop. Its new automatic-discovery checks use component-only owned presenter callbacks to exercise the production discovery/reservation methods without opening uncontrolled modal dialogs. They test a failed presenter and retry, another thread holding the catalog, retained recovery intent, no repeated handled confirmation, preserved older seen markers for explicit retries, stale-row removal, review failure/retry, FIFO batch retry and an actual pinned-catalog authentication-save failure followed by storage recovery. Existing 54 actual control/timer/worker checks remain included. This is not a rendering or live-browser test of the authentication, batch or review dialogs. The owned frame and test desktop close normally, with no desktop switching or injected user input. [UI checks and identities](validation/automatic-prompts-8.73/ui-checks.json).

Prequalification review removed a needless history-sized membership index on idle reads, skipped unrelated ID copies/active lookups, and added the larger-cohort cases. App and the new snapshot test were rebuilt against those final inputs. Other compiler inputs remained unchanged. The UI fixture independently records every declared input hash, which is checked against the final product source record as well as all core object hashes. Earlier preliminary outputs are not the qualified source/binary pair. [Adjustment record](validation/automatic-prompts-8.73/prequalification-adjustment.json).

All **35 owned fixture roots** and **36 temporary test executable/object files** were removed after preserving evidence. Qualified products/cores, source, rollback, installed 8.68 bytes and both personal catalog hashes remain. [Cleanup](validation/automatic-prompts-8.73/cleanup.json), [preservation](validation/automatic-prompts-8.73/final-preservation.json).

## Delivery and next work

**8.73 is a qualified development candidate. Installed UDM remains 8.68 / extension manifest files 0.72.32.** No installer, installation, extension activation or fresh whole-download/IDM/browser timing claim is included.

Next move automatic authentication claim persistence to an owned joined task and make login presentation acknowledgement explicit, preserving cancel, save failure, shutdown and changed-challenge semantics. Audit remaining authoritative presentation reads, network polling and completion-action ownership separately. A fresh authorized live browser/IDM comparison remains necessary before broader latency or parity claims.
