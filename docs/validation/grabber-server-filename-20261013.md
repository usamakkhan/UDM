# Grabber server-filename acceptance, 2026-10-13

The Grabber previously applied the file include pattern to a URL path before
requesting metadata. A link such as `/download?id=1` or `/opaque.bin` was then
skipped even if its response supplied `Content-Disposition: filename=renamed.pdf`
and the project included `*.pdf`. The Grabber now probes opaque file endpoints
and applies the include pattern to the server filename. Explicit URL-name
exclusions still suppress the request. Known static page links outside the file
filter retain their no-request behavior.

The local HTTP fixture covers an extensionless download route, a `.bin` route
renamed to `.pdf`, a URL-name exclusion, page scope, and a completed transfer.
The isolated `--grabber-checks` run passed **402 checks, 0 failed**. The full
`Udm.NativeTests.exe` run passed **2,605 checks, 0 failed**. Both used the same
binary (SHA-256
`60750E7C55ECD1B005787376709C923A5E66F2CA2719B08C0357984F0F0AB26B`)
and isolated temporary test roots on drive C. No installed application or
browser extension was changed. Public-site and complete IDM parity acceptance
remain open.
