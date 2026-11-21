# Toolbar image caching — 9.54 / extension 0.73.18



Cache built-in toolbar sources and bounded size/style/DPI rasters; reuse the active native normal, hot and disabled image lists on unchanged repaints. Keep custom skins fresh, make Reload invalidate built-in assets explicitly, and preserve current image handles if regeneration fails. Build five native 9.54 products and pass 187 backend plus 814 optimized UI assertions. Source browser manifests advance to 0.73.18. Installed 8.68 and extension files 0.72.32 remain unchanged; installed IDM parity and the prior total-time gate remain open.



## Ownership and bounds



Each main window owns twelve immutable decoded sources, bounded to 3 MiB of pixel storage. A lazy source load validates dimensions and pixel length before retention. Shared immutable borrowers survive invalidation safely. Reload clears both caches and advances the source generation. Ordinary repaints do not poll built-in PNG overrides on disk; users must choose Reload after editing those assets.



Rendered sets contain twelve normal, hot and disabled frames. The key includes exact pixel width, height, raised/flat style and source generation. An LRU retains at most four sets and 8 MiB of frame pixels. Checked arithmetic rejects oversized entries before building them. Oversized UI requests retain the uncached per-frame path. These bounds describe retained pixel buffers, not all allocator or native image-list overhead.



If the active key and three native image-list handles remain valid, repaint rebinds those handles without source IO, rasterization or native list recreation. Font, caption, toolbar geometry and palette handling still execute. A size/style/DPI key change reuses cached pixels when available and constructs replacement native lists before replacing existing handles. Failed source/raster work preserves the current lists and is retryable. A generation change during preparation is rejected. Caches are owned by the UI thread, not concurrent shared services.



Custom TBI/BMP skins retain their existing fresh-read behavior on every apply; they are deliberately not cached. Switching back to the built-in appearance restores the correct built-in pixels even when dimensions match the custom strip.



## Qualification



All 187 backend assertions across nine suites pass, including 33 cache assertions. All 814 optimized UI assertions pass: 25 new-cache checks, 284 full toolbar checks, 36 appearance checks and 469 menu-state checks. Tests self-attest separate desktops; no desktop switch or physical input injection occurs.



The new fixture draws all twelve native icons in all three states and compares actual pixels through size, style and DPI roundtrips. It verifies no extra decoding/raster work on hot applies, native handle reuse and rebinding, font/palette updates, explicit Reload of a temporary PNG override, custom-skin freshness, and injected source-load failure with successful retry. The temporary assets live only beside an owned test executable and are removed afterward. Personal settings and installed files are not test inputs.



Frozen 9.53 fails the same first assertion requiring unchanged appearance to reuse its native image lists. Its single same-appearance observation is 27.8504 ms; the current observation is 26.4147 ms. This small, noisy difference is not a benchmark distribution or evidence of installed IDM speed parity. The stronger result is eliminating repeated decoding, rasterization and native handle creation on the covered path. DPI handling still performs other image, font and layout work.



## Legacy fixture maintenance and evidence scope



Running the existing full toolbar fixture exposed stale test assumptions. Its menu snapshot now uses the current explicit capture/render API, queue labels use the display-label contract, and gallery/local-folder expectations follow the existing October 29 toolbar-discovery contract. A generated small-icon BMP was missing and has been supplied. No failing product behavior was hidden by changing production layout.



Exact geometry expectations retain the frozen pre-cache 9.53 layout: bottom inset 2, closed-category left inset 7, and toolbar/list gap 8 at 120 DPI, scaled by the existing formula. The tracked October 18 IDM UIA capture independently establishes the eight-pixel toolbar/list gap. The other two values are frozen UDM geometry, not freshly measured IDM observations.



The initial 662-file build inventory differs from the final qualification inventory only in native/ToolbarUiTests.cpp. All production inputs are unchanged. The new-cache UI run uses the initial inventory; toolbar, appearance and menu runs use the final inventory. Both inventories and the exact exception are retained. Diagnostic Od runs, a compiler memory failure, stale API/layout assertions and a four-failure aggregate fixture run are retained as diagnostics and excluded from qualification counts. The ordinary optimized fail-fast toolbar suite is the final authority.



## Remaining work



Other image/font work, startup, shared settings command ownership, live browser integration, real-network performance, installed completion/background-work comparisons and full IDM parity remain open. No installer, installation, commit, push or release was performed for this increment. Browser version changes track the increment; capture behavior is unchanged.



[Sanitized validation status](validation/toolbar-cache-9.54/status.json) records source, core and output hashes, assertions, baseline proof, compiler logs, fixture maintenance and cleanup. Prior products, required core objects, shared media helpers, installed files and both personal catalogs are preserved.

