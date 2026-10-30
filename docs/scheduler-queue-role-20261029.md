# Scheduler queue roles and reference recheck

[IDM's Scheduler documentation](https://www.internetdownloadmanager.com/support/idm-scheduler/idm_scheduler.html) restricts switching between download and synchronization types to custom queues. UDM previously locked those controls by matching the literal names Main queue and Synchronization queue. Its default-queue initializer already uses DefaultQueueRole, and may create a suffixed synchronization queue when a name is occupied. Name-based UI checks could therefore unlock a built-in queue or lock a later custom queue sharing its old name.

Scheduler now determines whether queue type is fixed from the stored Download/Synchronization role. The change handler uses the same rule. Manager::setQueues also rejects a type change for an existing role-bearing queue and preserves its role when an update omits or alters that field. Existing saved queue contents and types are not rewritten. Custom queues retain their type-switching behavior, including the existing requirement to remove completed members before switching to download mode. Queue deletion and reassignment behavior is unchanged.

## Reference-to-source review

The following paths were inspected, not executed. Presence in source does not establish complete reference parity.

| Workflow | Current UDM source |
|---|---|
| Per-queue settings, Apply and saving when switching/closing | SchedulerUi.hpp::saveAll, TVN_SELCHANGING, accept/cancel callbacks |
| Manual start/stop and independent stop times | Queue.cpp::queueRun and QueueStop.hpp |
| Repeated synchronization and replacement | Queue.cpp::prepareQueue, tick and startSynchronization |
| Failure reordering and optional retry limit | Queue.cpp retry traversal and QueueRetry.hpp |
| Multiple selected files and cross-queue moves | SchedulerUi.hpp, QueueMembership.hpp and QueueDragUi.hpp |
| Completed membership follows queue type | QueueMembership.hpp::retainCompletedMembership and membership validation |
| Per-queue concurrency | SchedulerUi.hpp Parallel control and Queue.cpp dispatch |
| Completion actions suppressed after manual stop | Queue.cpp CompletionCanceled and QueueCompletion.hpp |
| Shared download quota controls | SchedulerUi.hpp::applyLimits and existing quota settings |

These items should not be treated as missing implementations solely from older backlog descriptions. Differences still require acceptance review: retry exclusions for requests needing capture/authentication, server Retry-After handling, fixed concurrency/retry ranges, synchronization restrictions and local-file failures, plus the installed GUI and timing behavior.

All five native production targets compiled and installer packaging passed. See [build evidence](validation/scheduler-queue-role-build-20261029.json); the production bundle manifest records all 147 payload hashes. No runtime, GUI or regression tests are run under the current user instruction. Extension 0.63.82 is unchanged. Nothing is installed or published.
