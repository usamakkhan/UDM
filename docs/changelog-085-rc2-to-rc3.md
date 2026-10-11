# Changes from 0.85.0 RC2 to RC3

RC3 preserves strong ETag and valid Last-Modified values from captured original HTTP responses through the signed broker and desktop catalog. A captured partial body can now resume with a guarded range request: UDM first verifies remote identity, then requests only the missing bytes with If-Range. It keeps the original partial if validation or range support fails and does not silently restart from zero.

Controlled live checks cover successful continuation, changed ETag, ignored ranges, range loss after probing, restart, and exact final bytes. Native and browser version declarations remain 0.85.0 and 0.63.0.

This is still limited to supported HTTP original-response capture. Replaying browser authentication or session headers, HTTPS original-response takeover, and cleanup of retained original prefixes after successful continuation remain open work. The release source is product commit `1fb2877`; later local work is outside this release.
