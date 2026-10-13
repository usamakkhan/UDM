# Quota-enabled desktop refresh, 2026-10-13

With a download limit enabled, each visible desktop row previously called
`Manager::quotaStatus(job)`. That call scans download records and captured
response reservations, so refreshing a large history repeated the scan for
every row. The desktop now requests one snapshot of jobs actually waiting for
quota and uses set membership while rendering rows. The snapshot preserves
ordinary parent/child waiters and flow-controlled captured responses.

An isolated fixture with 5,000 completed records and a quota configured
measured **5,008,322 microseconds** for the former per-job loop and **3,906
microseconds** for the snapshot path in one run; both reported zero waiting
rows. This is a synthetic refresh-path comparison, not an end-to-end app
latency or IDM benchmark.

Validation on the same source:

| Check | Result |
| --- | --- |
| Quota-focused native checks | 85 passed, 0 failed |
| Captured original-response checks | 204 passed, 0 failed |
| Real-window menu and refresh checks | 374 passed, 0 failed |
| Full native suite | 2,608 passed, 0 failed |
| Production desktop build | Compiled and linked |

The isolated candidate binaries were built on drive C. SHA-256:
`UDM.exe` = `A7A78275086AA9EAD6F683388BE3F24CFED5A0D982106A73F3768D33C72659E9`;
`Udm.NativeTests.exe` =
`B2DCAED7B9A72E91FA88CBE9DBC88502210FA12035C82EE75A16373AF6307275`.
No installed application was changed.
