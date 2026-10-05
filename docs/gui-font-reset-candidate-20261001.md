# Font menu and reset — staged candidate

Installed IDM's menu resource exposes View > Font > Choose Font and Reset to default Font. The staged UDM app now has those two routes.

Reset restores UDM's default face (Tahoma), logical height (11) and weight (400), immediately reapplies appearance, and saves the settings. It preserves unrelated preferences and download records.

Verification: 87 native checks passed, including seven font-specific checks for menu availability, settings isolation, record preservation, immediate native font application, list/tree application and persistence. Eight actual app checks passed, including reset from Arial 17/700 and persistence after closing and reopening through recovery. These totals overlap existing regression coverage and are not parity percentages.

Evidence: D:/UDM-Workspace/candidates/native-084-backup/control/font-reset-acceptance.json. The rebuild completed successfully. All four protected personal catalog/application hashes match their prior values. The change remains staged, not installed or packaged.

The resource establishes IDM menu structure, not its live font defaults. Exact visual matching, DPI scaling, keyboard/accessibility coverage and full IDM parity remain unverified. Diagnostic LimiterTrace sources/binaries in this candidate remain excluded from packaging.
