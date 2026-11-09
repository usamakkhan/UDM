# Direct browser prefetch from an existing catalog write

Browser admission already saves a fresh direct offer as awaiting confirmation. File Info can now start prefetch from that record when the complete job matches the latest successful catalog write. The in-memory confirmation gate still prevents publication until Start. An interrupted unconfirmed offer recovers paused from the saved record. A changed record, resumed transfer, media working directory, or unverified catalog write still uses the existing admission write.

All 301 native File Info checks passed against an owned HTTP origin. The new cases compared `state.json` under the catalog lock before and after prefetch admission. The unchanged offer left the file byte-for-byte identical and reopened as paused in an isolated catalog. The changed offer saved the new admission before the worker ran. Each displayed 4,097 bytes before Start and published exactly those bytes using a single GET.

The fixture results are under `candidates/durable-direct-prefetch-5.72-tests/results`. The eliminated write was verified directly. Browser click-to-dialog latency and paired IDM timing remain open. [Native build receipt](durable-direct-prefetch-5.72-build.json)
