# DASH resource credential scopes

Native recording capture now takes an explicit map of headers keyed by canonical HTTP(S) origin. Initialization files, media segments, alternate BaseURLs and redirect destinations each select their own header scope. A missing origin receives no supplied headers. Manifest credentials are no longer implicitly assigned to the first media URL, and an alternate CDN can use its own captured credentials.

Resource requests disable implicit browser-session, site-login and Windows intranet credentials while retaining transport/proxy preferences. Range and conditional headers are removed because these receipts describe complete resources. HTTPS downgrade rejection remains in place. The map is limited to 64 origins and 256 KiB of header names and values; each header set passes the existing validator. Headers are not written into recording indexes or receipts.

This changes the native capture API, not browser admission. The future live job must construct origin scopes from authorized browser request data and keep manifest refresh credentials separate. Live DASH still needs its polling/window selection, resume/stop handling, Period transitions and final publication connected. No complete IDM parity is claimed.

All five production targets and the installer compiled successfully; see [the build record](validation/native-dash-resource-credentials-build-20261028.json). Runtime and regression testing are deferred at the user's request. The candidate is unsigned, uninstalled and unpublished. Browser 0.63.61 is unchanged.
