# Requested authentication views, 8.76 / browser 0.72.40

Queue-retry login preparation still copied model data and resolved saved credentials on the window thread after 8.75. The main list and progress window now supply complete prepared views to the login dialog, which no longer has an unprepared fallback that reads Manager or decrypts settings during initialization.

## Coherent requested views

`RequestedAuthenticationTask.hpp` snapshots current membership, challenge fields, pending expectation, queue ownership and encrypted saved-login sources under Manager. It resolves the initial credentials outside that lock on a worker, then rechecks the snapshot before publishing it. Requested preparation is a read: it does not clear AuthenticationPromptPending, write the catalog or schedule a transfer.

Before presentation, a try-lock validates the complete view against current membership, activity, challenge, pending flag, queue ownership, suppression policy and saved-login sources. Busy validation retains the view for a later pulse. Changed or removed requests cannot be presented from a stale view. Explicit nonqueue preparation is supported by the same boundary, although the current production callers are queue-retry flows.

## Main list and progress ownership

Main-list Resume classifies a queue login request using its existing authoritative selection read, then coalesces duplicate requests for that job and starts one owned preparation task. It retains the front request until preparation and presentation settle, prioritizes requested login over automatic login, and handles later requests in order. A preparation or presentation failure drops only that failed UI request and leaves the saved authentication intent available for another explicit Resume. Stale results are discarded without resuming a changed download.

The progress window resolves a login follow-up inside its existing owned command worker. After acknowledging and joining that command, it retains the prepared view if another decision or login presentation prevents display. Its controls cannot submit conflicting commands during that wait. Dismissing the waiting progress window can discard its read-only view without clearing the original queue challenge.

Both callers use the same prepared Login required dialog and the joined Sign in command from 8.75. The command now validates queue ownership as well as the displayed challenge and pending expectation, so a queue change after presentation cannot silently receive stale credentials or an obsolete retry. A presentation counter prevents main-window shutdown from destroying a login dialog owned by progress; concurrent login presentations defer until the current one ends.

Cancel and read-only teardown leave AuthenticationPromptPending untouched. Main shutdown joins requested preparation and discards only its read result before Manager shutdown; there is no detached credential-resolution worker or extra restoration save. Sign in still waits for its accepted credential-save/retry command to settle before dismissal.

**The main list's initial Resume classification and its other duplicate, address-refresh and content-confirmation branches remain synchronous authoritative commands.** This increment removes the subsequent login-preparation waits; it does not claim that all Resume processing or every dialog in UDM is asynchronous.

## Qualification

All five native products report **8.76**; both source manifests are **0.72.40**. Bridge is the only recompiled shared core, and Transfer remains byte-identical to 8.75. Independently captured UI inputs and all shared core hashes match the qualified product build. [Build identities](validation/requested-authentication-8.76/build.json).

Twenty-one backend suites pass **2,246 assertions** across forty-two C:/D: runs. New requested-view tests cover a held model, duplicate requests, source/policy/pending/queue changes, changed-during-resolution results, asynchronous errors, joined teardown, removed downloads and real Basic/Digest retries. Each authenticated payload is checked against the complete exact local-origin file. Existing automatic authentication and Sign in command tests remain included. [Backend evidence](validation/requested-authentication-8.76/backend-checks.json).

The actual main/progress/login window fixture passes **125 assertions** on a separate owned Windows desktop. It exercises the real main-list Resume route while credential preparation is held, coalesces duplicate requests, rejects a changed challenge, and opens/cancels the actual prepared queue login while an independent writer holds Manager. It also starts the actual progress command under a held model, retains its login view behind a separate decision, cancels without consuming the challenge, and signs in through the real progress dialog to publish the complete exact Digest-authenticated file. Existing automatic prepared rendering, held Sign in persistence and exact Basic download cases remain included. [Main and login UI evidence](validation/requested-authentication-8.76/ui-checks.json).

A separate existing progress-menu/window fixture passes **88 assertions**, covering actual menu and persistence behavior after the progress ownership changes. [Progress evidence](validation/requested-authentication-8.76/progress-menu-ui-checks.json). Both owned desktops close normally; no switch to them or input injection into the user's apps occurs. These local fixtures are not public browser downloads or fresh IDM timing comparisons.

After evidence capture, all **44 owned fixture roots** and **46 temporary test executable/object files** were removed. Qualified products and shared cores, rollback data, installed 8.68 files and both personal catalogs are preserved. [Cleanup](validation/requested-authentication-8.76/cleanup.json), [preservation](validation/requested-authentication-8.76/final-preservation.json).

## Delivery and next work

**8.76 is a qualified development candidate. Installed UDM remains 8.68 / extension manifest files 0.72.32.** No installer, deployment, extension activation, push or release is included. The paired manifest increment records this improvement; it does not imply browser-script changes.

Continue the remaining authoritative Resume selection/confirmation work, capture/review presentation, network polling and completion-action paths. The broad parity goal also still requires fresh authorized installed-app/browser comparisons with IDM, including joined background work; this increment does not establish universal timing or visual parity.
