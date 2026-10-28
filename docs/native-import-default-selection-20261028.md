# Native import defaults after metadata

## Reference

Retained IDMan.exe analysis (SHA-256 03CC62E9ADB77A380F9DC12F67CCAAEE5106F12844AA73CE32C914DDD16D607C), function 004c14b0, identifies .htm and .html output filenames and clears the row selection field before updating the import list. This is static reference evidence, not a new live observation.

## Implementation

UDM now updates untouched default selections when a successful metadata result arrives. HTML filename extensions and HTML/XHTML MIME types clear the checkbox; other successfully identified files keep the default selection. Failed or unavailable metadata does not decide selection. Explicit user checkbox changes take precedence, including Check all and Uncheck all even when a row already has that state.

The native list reports checkbox changes by stable row identity. Programmatic checkbox updates during metadata application and filtering do not count as user choices. Hidden rows retain their choice and can receive metadata defaults; returning them to view restores their recorded state. Refreshing metadata after URL or authorization edits retains explicit choices.

The manual-choice precedence is UDM's asynchronous interaction rule; the retained reference establishes the HTML default, not every race or keyboard interaction. The rows remain selectable if the user intentionally wants HTML. No proprietary implementation code is copied.

Runtime, keyboard and regression tests are deferred at the user's request. Compilation and installer results are recorded separately; this is not a claim of full IDM parity.
