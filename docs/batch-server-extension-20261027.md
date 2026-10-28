# Server extensions in batch filename patterns

Batch filename patterns now incorporate extensions from successful metadata
responses with a usable Content-Disposition filename. For example, pattern
`chapter-*.bin` and a server name `document.pdf` preview `chapter-001.pdf` for
token 001. Each row uses its own server extension; a mixed batch does not force
all files into the first file's type. Explicit per-file names take precedence.

The first available server extension other than .htm or .html updates the shared
pattern field once per lookup generation, provided the filename stem contains
an asterisk. The same correction is applied if the user enters a pattern after
that response arrives. The suffix after the last dot is replaced; if there is
no dot, the extension is appended. A pattern with its asterisk only in the
extension keeps that pattern. All resulting names still pass the existing
length and safe-name checks before admission.

Changing a URL or authorization resets the metadata generation and first-server
extension state. Already-admitted rows retain their submitted names. A blank
pattern continues to use ordinary server naming.

## Reference and remaining scope

Retained IDM function 004c14b0 sets a first-non-HTML-extension flag, adjusts the
shared pattern's extension when its stem contains an asterisk, and then combines
each row's substituted pattern stem with that row's extension. This UDM change
implements that behavior for extensions supplied by its metadata preview.

UDM does not infer an extension from MIME alone here. Servers without a usable
Content-Disposition filename, refused HEAD requests and timeouts leave the
pattern unchanged. IDM's broader response-derived filename resolution remains
to be matched; this is not a claim of complete filename parity.

Runtime and regression testing are deferred at the user's request. The native
candidate is built and staged, not installed. See the
[build record](validation/batch-server-extension-build-20261027.json).
