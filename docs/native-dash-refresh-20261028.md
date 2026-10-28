# Native DASH manifest refresh

The native MPD module now has an HTTP fetch/revalidation entry point returning a parsed snapshot. It uses ETag preferentially, falling back to Last-Modified, and sends validators only to the exact URL that supplied them. An unsolicited 304 is rejected. A valid 304 preserves the parsed document and its original body receipt time, so revalidation cannot silently re-anchor an old direct clock timestamp as newly published time. Identical 200 bodies also preserve their original body receipt time.

Each refresh has a shared fifteen-second cancellation deadline, at most six HTTP requests and a 2 MiB body limit. Redirects are handled explicitly. HTTPS downgrade is rejected; explicit request headers and browser session/site-login settings are not forwarded across origins. HTTP/HTTPS and normal proxy configuration remain supported. Caller-supplied range and conditional headers are replaced with the snapshot's own conditions. Cache validator lengths/control characters and response Age are checked.

New documents are parsed before a replacement snapshot is returned. Failed requests or invalid updates leave the caller's previous value unchanged. Root presentation ID changes, backward/missing publication time after an existing publication time, changed live availability origins, and static-to-dynamic reversions are rejected. Live-to-static conversion is allowed. These are initial update guards; full selected-representation/Period identity, codec compatibility and recovery validation still need connection to the recorder.

Snapshot validity is measured from request initiation using minimumUpdatePeriod, with response Age deducted. Zero gives no additional validity, while an omitted value leaves the validity deadline unset. Static snapshots have no periodic validity deadline. These distinctions follow the [DASH-IF snapshot validity and refresh guidance](https://dashif.org/Guidelines-TimingModel/#mpd-validity). The entry point performs one refresh; it does not run a polling loop or impose an invented update interval.

## Remaining integration

The callable fetch path is connected to the native XML parser. A live recorder still needs to own its refresh schedule, synchronize clocks, choose and validate representations, plan availability windows, transfer and journal segments, and assemble the recording. Dynamic browser MPDs remain rejected until that path exists. No HTTP endpoint was exercised during this change.

Runtime and regression testing is deferred at the user's request. Build/package evidence is in `validation/native-dash-refresh-build-20261028.json`. Browser 0.63.61 is unchanged. The candidate is unsigned, uninstalled and unpublished; compilation does not establish end-to-end correctness or IDM parity.
