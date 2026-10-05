# Category dialog controls — staged candidate

The category editor now uses the installed IDM category resource's 322 by 177 dialog-unit dimensions, 8-point Tahoma font, field positions, separators and right-hand OK/Cancel/Browse positions. The title is UDM categories. The new site restriction checkbox enables/disables the sites field; unchecked saves no host restriction. Remember last save path now appears in category properties and saves atomically with the category fields. Rename carries its preference; validation or persistence failure restores the previous settings.

Read-only reference: IDMan.exe SHA-256 03cc62e9adb77a380f9dc12f67ccaaee5106f12844aa73ce32c914ddd16d607c, dialog resource 5/204/1033. IDM's [category documentation](https://www.internetdownloadmanager.com/support/main.html) also confirms predefined categories may be edited/deleted. UDM's built-in name/delete restrictions remain open. The site help text describes UDM's current domain/subdomain support; arbitrary wildcard equivalence is not claimed.

## Verification

The final rebuilt native suite passed 254 checks. Added checks cover creation, rename, disabling destination memory and a real locked-catalog failure restoring both memory/type preferences and durable bytes.

Twenty-two checks passed against the rebuilt app in a fresh isolated profile. Measured client size was 564 by 354 physical pixels, matching 322 by 177 units at the measured 7 by 16 font base units. All four input rectangles and all three action button rectangles match the reference coordinates. Site toggling, remember-path persistence, reopen, Cancel, unchanged existing records/files and normal exit pass. These measurements do not establish pixel-perfect painting or all monitor/DPI transitions.

The first compilation exposed an incorrect CWnd default-button call and was corrected. Two initial GUI geometry attempts failed because the observer was DPI-unaware and measured a foreign font handle; the observer was corrected to use per-monitor awareness and its own matching font. The final candidate was unchanged for the passing rerun. Failed results remain retained.

[Acceptance](D:/UDM-Workspace/candidates/native-084-backup/control/category-controls-acceptance.json) · [Native results](D:/UDM-Workspace/candidates/native-084-backup/control/category-controls-native-results.json) · [Actual app results](D:/UDM-Workspace/candidates/native-084-backup/control/category-controls-app-3-results.json).

The personal catalog and installed app/host/monitor hashes remain unchanged. The candidate is not installed. Full IDM parity remains unfinished.