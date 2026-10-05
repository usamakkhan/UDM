# UDM 0.70.0 / browser 0.47.0 — offline website folders

Offline ZIP projects can now preserve the website's original folders. Select **Offline website (ZIP)**, choose a specific destination folder, and enable **Use original subfolders**. Extract the downloaded ZIP and open **index.html**.

Nested pages, CSS imports and assets use relative local links. Unicode names, query variants, case collisions, external origins and redirects are handled without overwriting each other. The option persists across restart, and paused captures reuse verified cached resources. The flat ZIP option remains available. The About dialog's stale release label is also corrected.

**1,848 passing checks:** 1,812 native checks, nine hierarchical offline/Edge acceptance checks, 19 legacy offline/proxy checks and eight native messaging checks. The extracted hierarchical pages render and navigate in offline Edge with zero HTTP(S) requests. [Detailed evidence](evidence-0.70.0/summary.json), [test scope](evidence-0.70.0/DEVELOPMENT.md).

This closes the hierarchical ZIP gap in F091. JavaScript-rendered exploration, broader real-site coverage, native visual acceptance and full IDM parity remain unfinished. Browser integration remains **0.47.0**; this native update requires no new extension reload. [Workflow inventory](idm-parity-0.70.0.md).
