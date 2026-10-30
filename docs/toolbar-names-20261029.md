# Toolbar menu names

The toolbar menu now reads the declared `name` from each discovered `.tbi` file instead of showing its filename until that skin is selected. The inspected gallery packages include `PureFlat.tbi` declaring `Pure Flat` and `Kavian.tbi` declaring `Kavian K.`; both should now retain those names before selection.

Metadata parsing is shared with the full skin loader. Opening the menu reads at most 16 KiB per metadata file and does not decode bitmap strips. Existing discovery is capped at 64 entries. Invalid or unavailable metadata falls back to the filename (or the previously loaded name for the selected skin), so one damaged skin does not prevent opening the menu. Selecting a skin still performs full image validation. Literal ampersands are escaped through the existing menu-label helper.

This change does not add a persistent metadata cache, change ordering, or alter image decoding. Extension 0.63.70 is unchanged. All five production targets compiled and installer packaging passed. See [build evidence](validation/toolbar-names-build-20261029.json) for installer and source hashes; the production bundle manifest records all 147 staged payload files. Runtime and UI tests remain deferred at user request; no installation or publication is performed.
