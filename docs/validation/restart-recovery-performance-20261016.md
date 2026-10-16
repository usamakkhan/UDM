# Restart-recovery startup performance, 2026-10-16

`StartupPerformanceTests` now loads a synthetic catalog of 5,000 completed
downloads with 500 committed restart cleanup receipts. The receipts reference
retired, empty parts folders. Construction must preserve the catalog and clear
every receipt. This exercises the startup path used after interrupted or
deferred duplicate-overwrite cleanup.

| Build | Recovery startup | Ordinary startup samples |
| --- | ---: | --- |
| Before index | 14,025 ms | 86, 89, 178 ms |
| With index | 6,824 ms | 82, 88, 89 ms |

The change builds job-ID and occupied-parts indexes once per nonempty restart
journal directory instead of scanning all saved jobs for every receipt. The
individual receipt checks, file identity checks, and cleanup behavior remain
unchanged. The focused native `--overwrite-checks` suite passed 84 checks,
including restart crash-phase recovery and locked-part retry cases.

These are single local runs on synthetic data. Filesystem cache, antivirus,
and scheduling can affect the timings; they do not predict the installed app's
startup time or compare UDM with IDM.
