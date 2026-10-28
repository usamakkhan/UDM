# Browser link import

The IDM browser-menu guide documents editing imported download attributes before
adding the selection to the queue:
https://www.internetdownloadmanager.com/support/right_click_IE.html

UDM's Chromium and Firefox link-selection pages now provide a Save as field for
each row. An empty field preserves server-provided naming. Entered filenames pass
through the existing native handoff; they do not replace the source URL or its
captured browser context. The page and background handler reject path characters,
control characters, trailing dots/spaces and reserved Windows device names before
any selected link is sent. Names are limited to 240 characters.

Download later is selected by default, matching the documented queue import flow.
Users can clear it to retain immediate download behavior. Selection limits remain
100 links. After submission, controls stay locked even if the response is lost or
only some links were accepted; the result directs users to inspect UDM before
collecting a new selection. This complements the existing persisted background
batch lock rather than replacing it.

Extension version is 0.63.44 in both manifests. JavaScript syntax, manifest parsing,
matching browser files and whitespace were checked. Runtime, native handoff and
browser acceptance tests are deferred at the user's request. Not installed.

This closes filename editing and queue-default gaps only. The link-import page
still does not offer IDM's full per-link property editor or background type/size
inspection; no claim of complete import-dialog parity is made.

## Follow-up: folders and descriptions, 0.63.45

Each row now has More properties with Save folder and Description fields. Empty
folders retain category routing; explicit absolute folders pass through to native
admission before filename collision handling. The native bridge validates the
properties object, string bounds, absolute folder and usable path components.
Descriptions apply only to newly admitted records or explicit replacement attempts,
not to an existing duplicate selected by the user's duplicate policy. Download
address-refresh offers continue to use their separate desktop review workflow.

The bridge advertises `browserImportProperties: 1`. The extension checks that
capability when properties are present, so an older desktop cannot silently ignore
the chosen folder or description. An empty properties object uses the existing
protocol. The browser's captured request context and URL are unchanged.

The matching desktop build includes the earlier unattended-queue-dialog changes.
Build status and candidate hashes are recorded separately in
[the build record](validation/browser-import-properties-build-20261027.json).
No live browser, regression or handoff tests were run, as requested. This does not
close the remaining background type/size inspection and full property-editor gaps.

## Follow-up: size and type inspection, 0.63.46

The selection table now shows Size and Type. Four workers inspect rows without
blocking selection or submission. Existing unambiguous GET-response metadata is
reused. Otherwise an eight-second HEAD request runs in the captured source frame,
checking the source page address before and after the request. The document ID is
used when available. Requests include credentials only when the cookie-sharing
setting and permission are both enabled. Captured non-GET requests are not replayed
for inspection. No GET fallback downloads a response body.

The HEAD result reports a byte count only for a safe integer Content-Length without
non-identity Content-Encoding. These are server-reported response sizes, not an
exact remuxed-media size prediction. Unsupported HEAD, CORS/CSP restrictions,
navigation changes, errors and absent size headers leave Unknown. HTTP responses
that declare HTML can be hidden; hidden rows retain their explicit selection.

Metadata HEAD requests are ignored by request-context capture and media discovery,
so they cannot replace the original GET/POST context or trigger a background
download. Pending inspection does not delay import; the page stops scheduling
checks once submitted. Already-issued checks have their own eight-second deadline.

JavaScript syntax, manifest parsing, mirrored source files and whitespace checks
passed. Browser and regression tests remain deferred. The native executable is
unchanged from the 0.63.45 import-properties candidate; 0.63.46 browser files are
staged separately in `candidates/browser-link-metadata-06346/browser/`. Neither
this extension nor the matching import-properties desktop candidate is installed.

## Follow-up: sorting and selection, 0.63.47

Name, Address, Size and Type headers are keyboard-accessible sort buttons with
`aria-sort` state. Sizes sort numerically, unknown values stay last in either
direction, and equal values keep their original order. Sorting moves the existing
row controls, preserving edits, selection, focused control and table scroll.
The completed metadata scan reapplies the chosen sort; partial results are sorted
when the user chooses a header. Import follows the displayed sorted order while
retaining original batch indexes for browser context and edited filenames.

Select file links now uses UDM's existing MIME/filename classifier, including its
conflicting-web-response handling, with the same supported extension list as the
previous selector. Extensionless PDF/media links can be selected when their MIME
type is known. Metadata refines this automatic selection until the user changes
a checkbox or uses Check all/Clear all. Manual selection is then preserved until
Select file links is explicitly chosen again.

The footer shows selected count, hidden selected count, total known bytes and
number of unknown sizes. Integer summation avoids precision loss across large
selections; known bytes are not presented as the complete total when sizes are
missing. These are source changes with syntax/manifest/whitespace checks, not a
new browser acceptance result. Runtime tests remain deferred. Extension files are
staged at `candidates/browser-link-selection-06347/browser/`, not installed.

## Follow-up: per-link authorization, 0.63.48

More properties now includes Use authorization, User name and Password. The
checkbox controls whether credentials are submitted. Empty passwords are allowed;
empty user names, colons in user names and control characters are rejected before
submission. The background repeats validation before constructing the HTTP Basic
header from UTF-8 bytes. Credentials are explicit user input and do not depend on
consent to share cookies captured from the browser.

Authorization uses the existing native `headers` admission path. The native manager
stores those headers with its existing protection function; new plaintext catalog
fields were not added. Credentials are never attached to the persisted link-batch
record, and the form clears them after submission succeeds or fails. The automatic
HEAD inspection does not use manually entered credentials. This is HTTP Basic
authorization through the existing desktop authentication path, not a new form-login
or OAuth implementation.

Both extension manifests are 0.63.48. Syntax, manifest, mirrored-file and whitespace
checks are performed; live authenticated transfers and browser acceptance remain
deferred at the user's request. No desktop rebuild is needed for this extension-only
change. The already-built import-properties desktop candidate remains the matching
desktop artifact; it and this extension have not been installed.

## Follow-up: page images and link descriptions, 0.63.49

A fresh read-only extraction of the retained IDM executable's dialog 228 confirms
controls 1234 (Use link texts as download descriptions) and 1385 (Hide images located
on this web page), along with its filename-pattern control. The resource establishes
the presence of the controls, not wildcard substitution semantics or runtime
behavior. The relevant metadata is retained in
[the reference record](reference/idm-import-controls-20261027.json).

UDM now collects image URLs as well as anchor/area links and records whether a URL
is an image displayed on the source page. Responsive images use currentSrc first.
Repeated URLs remain deduplicated, with image presence and available anchor text
retained. Image URLs do not substitute for an enclosing link's different target.

Hide images on this page combines with the existing text/HTML filters; selected
hidden rows remain included and counted explicitly. Use link texts as descriptions
fills a blank per-row description at submission, leaving an explicit description
unchanged. Image alternative text is not treated as anchor text. The existing
native import-properties capability check protects older installations.

Extension 0.63.49 passed syntax, manifest, mirrored-source and whitespace checks.
Browser and regression tests remain deferred. It is staged, not installed. Wildcard
rename remains open pending evidence of its semantics; no guessed behavior was added.

## Follow-up: selected-links review, 0.63.50

The selected-links overlay now opens the same review page used by Download all
links instead of sending each URL directly to the desktop. The chosen link list is
validated and deduplicated before creating a bounded, persisted batch. Source tab,
frame, document and page identity come from the extension message sender. The
page must match the sender, and private/disabled tabs are rejected.

An explicit-selection batch starts with every chosen link checked. Metadata no
longer changes that selection. This also applies to selected-link context-menu
imports. Names, folder/description overrides, authorization, sorting and metadata
inspection are therefore available before final confirmation. Opening review makes
no native download request. Link descriptions retain up to 1,000 characters; the
overlay labels remain shortened for display.

The legacy direct selected-links message remains supported for older loaded content
scripts until extension reload. Source syntax, manifest, mirror and whitespace
checks are recorded separately; runtime and browser tests remain deferred. Neither
extension 0.63.50 nor the staged desktop destination-mode build is installed.
