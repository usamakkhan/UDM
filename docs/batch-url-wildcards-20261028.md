# Repeated wildcards in batch URLs

## Reference evidence

The retained reference IDMan.exe (SHA-256 03CC62E9ADB77A380F9DC12F67CCAAEE5106F12844AA73CE32C914DDD16D607C) has numeric and letter generation paths in function 004885d0. Each copies the URL template, formats its sequence token, then calls 00402c00 with the string at 006978f0. Reading the mapped PE bytes confirms that string is an ASCII asterisk. The replacement helper counts and replaces every occurrence, advancing past each replacement. This is static analysis, not a fresh live observation.

## Independent UDM implementation

`batchPattern` now substitutes every literal asterisk with the same formatted token for that row. For example, `https://example.com/part*/clip*.mp4`, From 2, To 1, width 3 yields `part002/clip002.mp4` and `part001/clip001.mp4`. Letter batches use the same substitution. Encoded `%2A` remains literal. No-asterisk input retains the existing bracket expansion path.

The dialog previews already call this generator, so preview and submitted URLs agree. Original sequence tokens remain attached to rows for filename patterns after sorting. Existing numeric range, padding and batch-count limits are unchanged. No proprietary implementation code is included.

Runtime and regression tests remain deferred at the user's request. See the build record for compilation and staging status.
