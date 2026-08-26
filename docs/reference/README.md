# Reference metadata

`pe-analysis.json` was produced on 2026-09-18 by `tests/inspect-reference.cjs` from ten files in the user's installed IDM directory. Input SHA-256 values identify the exact inspected binaries. The script reads PE tables and dialog resources without loading DLLs or executing reference code.

This folder contains structural metadata, symbol names and control text/geometry. It contains no executable sections, driver binaries, extracted icon images or copied extension implementation. The report is evidence for an independent implementation; imported function names do not by themselves establish runtime behavior.
