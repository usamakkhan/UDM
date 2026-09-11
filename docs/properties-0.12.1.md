# UDM 0.12.1: completed download properties

IDM's [File Properties documentation](https://www.internetdownloadmanager.com/support/properties.html) states that double-clicking a file name opens its properties and that the Open button launches the file.

UDM previously launched a completed file on double-click. Its File > Properties and context-menu Properties commands also redirected completed items to the progress window. Version 0.12.1 routes all three completed-file actions to a File Properties dialog.

The dialog displays filename, completion status, exact size, URL, saved path, category, queue, description, recorded SHA-256 and local completion time. Completed-file fields are read-only. Open and Open folder are separate explicit actions; the Open button is disabled if the saved file is missing. Opening properties does not change history or file contents.

A double-click on blank list space does nothing, and a double-click acts on the clicked row when several items were selected. Double-click behavior for unfinished downloads still opens their progress window. Existing editing of paused/failed download properties is unchanged.

The desktop version and portable package are 0.12.1. Browser extension code and the download algorithm are unchanged; the HTTP User-Agent version string is updated. This is a focused UI correction, not complete IDM properties-editing parity.

Validation: the native build succeeded. In the deployed app, double-click, File > Properties, and context-menu Properties each opened the completed ISO's File Properties dialog. Double-clicking blank list space opened nothing. All ten download records remained identical to their pre-update snapshots. The app was left running with the properties dialog visible. See [machine-readable checks](reference/properties-0.12.1.json).
