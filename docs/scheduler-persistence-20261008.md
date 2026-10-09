# Scheduler persistence qualification — 8 October 2026

IDM's [scheduler documentation](https://support.internetdownloadmanager.com/support/idm-scheduler/idm_scheduler.html) specifies immediate saving of queue order/content and saving other edits on Apply, switching queues, or closing Scheduler. Current UDM implements these save paths. This turn verifies them rather than changing product behavior.

QueueExportUiTests still expected a Cancel button that discarded scheduler edits and assumed a fixed export queue index. The baseline failed at the fixed-index export selection. The updated test selects the queue by name, validates the current Close workflow, and asserts every modal callback actually ran. An intermediate visibility check ran before the owned dialog was shown; the final fixture explicitly shows its window before testing visibility.

All 69 final checks passed. Coverage includes EF2 import selection/cancellation and request metadata, export scope and format controls, scheduler Apply without starting downloads, Stop retaining the startup preference, outgoing drafts saved on queue switching, independent queue preferences, Close and WM_CLOSE persistence, IDCANCEL close-route consistency, catalog reload for both queues, and actual main-window import/export menu routes. Escape itself was not physically pressed; its dialog command route was exercised. The scheduler image was inspected.

No product-code change or new app candidate was necessary. The test entry point was rebuilt against unchanged backend objects; this is not a clean backend build. Raw logs, scripts, intermediate failures and final results are under `C:\Users\Abuzar\AppData\Local\Temp\udm-scheduler-persistence-20261008`. [Validation receipt](validation/scheduler-persistence-20261008.json).

This validates the scoped documented workflow, not complete scheduler or IDM parity. No personal installation or browser session changed.
