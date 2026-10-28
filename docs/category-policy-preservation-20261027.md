# Preserving explicit import categories

The native import destination modes mark an explicitly selected category so content
recognition cannot silently reclassify the file. Review found two metadata-copy
paths that dropped this marker: synchronization replacement and UDM catalog
export/import.

Synchronization replacements now inherit CategoryExplicit with Category. UDM
catalog export includes the boolean when present; import validates its type before
publishing any staged records. The marker is restored only when the referenced
category exists in the current profile. Otherwise the record uses the current
profile's filename/server category rules, including disabled categories and custom
rules, and remains eligible for automatic content-based classification. Older
catalogs without the marker remain supported.

This extends the earlier persistence, redownload and duplicate-restart handling.
It does not add an unsupported field to IDM's external export format, and does not
claim an IDM export round-trip test. No credentials are added to catalog exports.

Runtime and regression tests are deferred at the user's request. The desktop
candidate's build result is in
[the build record](validation/category-policy-preservation-build-20261027.json).
The candidate is not installed.
