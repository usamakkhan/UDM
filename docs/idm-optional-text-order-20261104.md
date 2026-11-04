# IDM optional-text ordering, UDM 1.24.0

Static inspection of the matching installed and repository IDM 6.43.10.2 binaries confirms that Save Path and Parent Web Page ascending callbacks share a UTF-16 comparator. It places empty values after populated values, then calls the CRT case-insensitive bounded comparison using the larger text length. Descending wrappers negate its result. Description and Referer independently have the same empty-last ascending rule.

UDM now applies those empty-value rules. Save Path and Parent Web Page use `_wcsnicmp` on UTF-16 keys built once per candidate on a sort-cache miss. Their full raw strings invalidate cached order conservatively after edits. Description and Referer retain their existing non-empty comparisons; their complete Unicode/collation behavior has not been established from the reference.

The reference's wide list getter uses a 256-unit buffer including its terminator. Filename, Save Path and Parent Web Page comparison keys therefore stop at the first NUL or 255 UTF-16 units. This bounds sorting only: persisted paths, filenames, URLs and displayed text remain complete. Equal keys retain catalog order through stable sorting, including strings differing only beyond that boundary. Existing settings-change cache invalidation is retained.

Microsoft documents that [`_wcsnicmp`](https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/strnicmp-wcsnicmp-mbsnicmp-strnicmp-l-wcsnicmp-l-mbsnicmp-l?view=msvc-170) stops at NUL or the specified count and uses CRT locale for case conversion. The retained reference identifies a VS2008 library implementation; the exact active reference locale and cross-CRT locale equivalence remain unverified. Filename comparison continues to use Windows StrCmpLogicalW.

See [reference trace](reference/idm-optional-text-order-20261104.json) and [build receipt](validation/idm-optional-text-order-build-20261104.json). Runtime sorting, locale, visual and performance tests are deferred by instruction. Installer creation, installation and publication are also deferred. A successful native build is not full IDM parity.

All five production binaries built successfully at 1.24.0. Product versions, artifact hashes and unchanged feature-source hashes were verified in the receipt. This was compilation and artifact inspection only; no runtime tests were run.
