# Native import Edit selection rule

## Reference evidence

The SHA-256 of the inspected IDMan.exe remains 03CC62E9ADB77A380F9DC12F67CCAAEE5106F12844AA73CE32C914DDD16D607C. The MFC message-map entry for import list 1392 (0x570), WM_NOTIFY 0x4e / LVN_ITEMCHANGED 0xff9b, points to 004bbf10. That handler obtains the selected count through 0043a640 and enables its control only when the count equals one. The reference right-click map entry points to 004bb950; retained independent DUMPBIN disassembly shows it only sets the result to zero and returns. No context menu is inferred from that entry.

## UDM behavior

Edit is now enabled only for exactly one highlighted, not-yet-added row. Highlight notifications update the button immediately, while the existing periodic refresh keeps it consistent after metadata/filter updates. The Edit action repeats the count guard before opening properties. Multiple checked rows remain valid for download admission: highlighting and checked download selection are separate states.

Previously, highlighting several rows still enabled Edit and silently edited the first. This change removes that ambiguity without reducing multi-file download selection.

Static source/reference checks and production compilation are permitted; runtime interaction tests remain deferred at the user's request. No menu or double-click behavior was added without reference evidence.
