# Server filename category routing

Download File Info's Use server name action previously changed only the filename. A generic download URL could therefore retain its initial category and default folder after accepting a server filename with a recognized extension.

The action now recalculates automatic category assignment using the accepted filename and final response host. HEAD preview returns the response host alongside its filename suggestion. Active prefetch uses its existing protected resolved address, only when response filename metadata is bound to the current request URL. The action does not change the original request URL or expose a redirected URL in the preview metadata.

A manual category change in the dialog, or a saved CategoryExplicit flag, prevents reassignment. A category that no longer matches the initial automatic classification is also preserved. When an automatic category changes, a destination still equal to that category's default folder follows the new category folder. A custom folder remains selected. Existing absolute-path, occupied-file and other-download reservation checks validate the final proposed path before any control changes. The new category's remember-folder setting is reflected in the dialog. This action changes the draft only; Start/Download Later still commits it through the existing workflow.

This implements filename/site routing for this explicit action. It does not establish automatic category refresh for every transfer redirect, adaptive-media origin or manually edited filename. [IDM's main-window documentation](https://help.internetdownloadmanager.com/support/using_idm/using_idm.html) describes file-type/site category routing and category folders; live behavior for this particular action remains to be compared.

Production compilation and packaging are in progress. Runtime, dialog and browser tests are deferred at the user's request. Extension 0.63.82 is unchanged. Nothing is installed or published.
