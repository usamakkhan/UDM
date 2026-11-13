# Native metadata filename resolution

Native HTTP metadata previews now resolve a filename from the final response
URL when Content-Disposition provides none, then use a specific MIME type to
replace a generic or endpoint suffix. For example, `download.php` served as
application/pdf previews `download.pdf`; `clip.bin` served as video/mp4 previews
`clip.mp4`. Query parameters are not used as filenames.

A specific filename suffix is preserved. Script endpoint suffixes are eligible
for replacement only when derived from the URL, not from an explicit server
filename. Generic MIME values such as application/octet-stream never imply an
EXE, ISO or another specific format. Unknown MIME types retain the original
name. The existing CorrectFileExtensions preference disables MIME correction.

The 7.43 body-verification follow-up removes a separate eight-character limit
on explicit alphanumeric suffixes. The isolated 7.42 browser handoff published
`old.udmfixture` as `old.udmfixture.zip`; that suffix was ten characters and was
therefore treated as unusable despite being explicit. Long alphanumeric suffixes
now follow the same preservation rule as short ones. Numeric version endings
still gain the recognized extension, and known weak/script suffixes remain
eligible for correction. This includes the existing long `crdownload` entry in
the weak-suffix list. This rule does not establish preservation of every possible
punctuated or Unicode custom extension, nor an observed IDM filename comparison.

The map is generated from the repository's existing mime-db 1.54.0 browser data,
with 1,016 MIME entries. `tools/generate-native-mime.py` regenerates the sorted
native lookup. The upstream MIT notice is retained under native/third_party and
copied into native build output. No IDM proprietary data is used.

This shared metadata path feeds File Info suggestions, native import names and
batch pattern extension adjustment. Suggestions remain informational: they do
not rewrite an explicit per-file name or become a transfer/resume validator.
Runtime behavior and full IDM equivalence are unverified; testing is deferred at
the user's request. See the [build record](validation/native-mime-filenames-build-20261027.json).

The broader backlog review also found stale historical claims: current
`StreamProgress.hpp` already renders media length when byte size is unknown,
and the current toolbar command set includes the 3D style. Those areas should
be compared against the installed build rather than implemented again from the
old GUI audit. Signing and physical/live comparative qualification remain open.
