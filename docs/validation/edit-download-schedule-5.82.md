# Edit a scheduled download in UDM 5.82

Use **Schedule…** in Download File Info to create a one-time schedule. Before it starts, right-click the waiting download and choose **Edit download schedule…** to reopen Destination, Dates and queue, Options, and Review. Edit the start and optional stop in local time, the save folder and filename, the category and description, connection and file retries, and the wake request. The wizard keeps the original queue. Use Scheduler to edit shared queues.

The isolated native fixture passed 21 checks. It created and reopened a dated download, moved its destination, verified the exact UTC start and stop and their scheduler boundaries, and reloaded the changes from disk. It also checked stale drafts, shared queues, invalid dates, retry modes, category defaults, and rollback after a forced catalog write failure. Validation covered the native scheduling and catalog behavior. Interactive wizard navigation and hardware wake were outside this run.

[Production build receipt](edit-download-schedule-5.82-build.json)
