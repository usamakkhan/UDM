# Coordinated native DASH live planning

The native MPD module now connects its fetch, continuity, clock and availability components through a live state/update API. Initial state selects one to three distinct representations in the same Period. Each update accepts one bounded media-time window per selected track.

An expired snapshot triggers conditional refresh and rematching of every selected track. Changed XML invalidates the retained clock sample. Missing or expired samples trigger declared-source synchronization. Cached manifest responses with nonzero Age disable the direct timestamp scheme while still allowing independent HTTP clock sources. Revalidation keeps the original direct-time anchor. Positive manifest validity that expires during synchronization fails explicitly; zero validity requires a fresh snapshot for the next update.

Availability is evaluated at both ends of the recorded clock uncertainty interval. The returned segment URLs must be ready in both plans: the lower clock estimate avoids premature requests, and the higher estimate excludes resources that might already have expired. This uses the clock module's uncertainty estimate, not a guarantee of actual network/server error bounds. All tracks share aggregate limits of 10000 segments and 4 MiB of URL text.

Updates operate on a candidate state and return it only after all track plans succeed. The caller's original state survives cancellation, refresh errors, incompatibility and planning errors. A transition to a static MPD is identified separately and plans the requested remaining windows with Period overlap filtering. That flag is not proof that all media has been downloaded or finalization can skip pending segments.

## Integration status

Refresh, rematching, clock synchronization and availability now call each other through the update API. The recorder still needs to drive it, select useful windows, transfer segments, maintain receipts, handle Period transitions, and assemble/publish complete output. It remains unconnected to browser live admission. No claim of a working live recorder is made by this change.

Runtime and regression tests remain deferred at the user's request. Production compilation and packaging are recorded in `validation/native-dash-live-plan-build-20261028.json`. Browser 0.63.61 is unchanged; the candidate is unsigned, uninstalled and unpublished. Complete IDM parity remains unproven.
