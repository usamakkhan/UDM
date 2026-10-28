# DASH duration metadata across address refresh

The duration handoff exposed a compatibility issue in `adaptiveSelection`: the normalized plan still contained every top-level field except transport/session fields. An older DASH capture without `durationMs` therefore compared unequal to an otherwise identical refreshed plan from extension 0.63.56.

The matcher now excludes the declared presentation duration from selection identity. Quality, audio/subtitle selection, track order, byte ranges and segment layout remain compared. Existing retained-resource validation still runs before downloaded bytes are reused; presentation length is not used to establish resource identity.

When the user accepts a validated DASH refresh, the record replaces `MediaLengthMs` with the new declared duration, or removes it when the refreshed manifest has none. This mutation is inside the existing catalog transaction: a commit failure restores the original record with its old metadata. A staged or cancelled refresh does not change the displayed length. HLS duration identity and calculations are unchanged.

The existing list renderer continues to prefer byte size over media length. No final-file-size calculation, network request order or browser source is changed.

## Delivery

`validation/dash-refresh-duration-build-20261028.json` records the production build and installer. Runtime tests, old-profile refresh acceptance and save-failure regression checks remain deferred at the user's request. The candidate uses extension 0.63.56 and is not installed or published.
