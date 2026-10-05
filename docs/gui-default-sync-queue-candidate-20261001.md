# Default synchronization queue — staged candidate, 1 October 2026

New UDM profiles now include a synchronization default as well as Main queue. Existing profiles receive a one-time initialization, matching the two-primary-queue concept in [IDM’s queue documentation](https://www.internetdownloadmanager.com/support/idm-scheduler/idm_queues.html). The installed reference executable also contains the label Synchronization queue.

The new synchronization queue is stopped, unscheduled and not set to start with UDM. No files are moved into it automatically. An existing synchronization queue with the matching name is reused without changing its settings. A case-insensitive collision with a different custom queue generates a separate unique name, such as Synchronization queue (2). Other custom synchronization queues remain separate.

DefaultQueuesInitialized records the migration, so explicit later deletion or replacement of queue settings does not recreate the default on restart. DefaultQueueRole identifies primary download/synchronization queues for icons independently of the display name. The migration is in memory until the next normal catalog save; it does not immediately overwrite the original catalog. Existing before-native backup behavior is retained, not replaced with a new backup policy.

## Verification

- Retained baseline reproduced missing default/migration behaviors.
- 17 focused migration checks passed: fresh/reloaded profiles, legacy settings, name collision, matching queue reuse, deliberate deletion, already-initialized profiles, unchanged download metadata/assignments and saved file bytes.
- 349 accumulated native GUI/transfer checks passed. The former no-synchronization-queue case now explicitly removes synchronization queues for that test, while also testing the default enrollment route.
- 22 actual-app checks passed on an isolated legacy profile, including custom queue preservation, stopped default, completed-file enrollment through Add to queue, a distinct primary icon, and restart without duplication.
- The first app harness inspected the durable catalog before the first save. The fresh passing run checks the same migration fields after a real UI save; application binaries were unchanged between these runs. Initial model harness also needed a guard against deleting a nonexistent baseline default. These failures are retained and not counted as product passes.
- Only Core.cpp and the UI units depended on the change. Core.obj and the candidate native host were rebuilt; app and native regression executable were linked with that object. Other engine sources/shared headers did not change.
- Protected installed executables and the personal catalog retain their SHA-256 hashes.

Evidence: `D:\UDM-Workspace\candidates\native-084-backup\control\default-queue-acceptance.json`, `default-queue-model-results.json`, `default-queue-native-results.json`, `default-queue-app-2-results.json`, and the corresponding test/build sources. Candidate app: `build\UDM.RecoveryCandidate.exe`.

## Remaining scope

Staged, not installed. Full IDM parity is not established. UDM’s existing Main queue name and queue deletion/renaming policy are unchanged; exact reference naming/protection behavior still needs qualification. Full menu ordering, schedule edge cases, accessibility and the broader browser/transport/driver gaps remain open. No personal browser, driver, certificate or download changed.
