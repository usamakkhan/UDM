# Selected-download move recovery — 1.11.3

Remove, Recycle and Redownload now reconcile a pending Move/Rename journal belonging to the selected download before removing its record or using its saved location. An uncertain move therefore retains the catalog record required for recovery; a verified destination can update the location before subsequent file actions.

Move recovery accepts an optional selected-download scope. An unrelated journal is left alone, including while another file is actively moving. Startup recovery retains its global behavior. Journal parsing, file identity/checksum verification, path-only metadata reconciliation and durable-save requirements are retained.

All five native production targets built successfully at 1.11.3; build-start source hashes remained unchanged. Runtime tests, installer creation, installation and publication are deferred. Installed UDM remains 1.6.0; browser source remains 0.63.84. Full IDM parity remains unproven.
