# Completed-file controls during relocation — 1.12.1

Open, Open With, Move/Rename and Redownload now disable while a completed download is active. Delete Completed selects inactive completed records, allowing other completed files to be removed while a move runs. The download progress primary action checks relocation again when invoked. Closing an old download progress window no longer cancels its separate file move; use the move dialog for cancellation.

Progress destination/information text now uses the same locked metadata snapshot as the other displayed fields, avoiding an unlocked read while the relocation worker commits its new path.

IDM documents that main controls are enabled according to possible actions: https://www.internetdownloadmanager.com/support/using_idm/using_idm.html . This correction applies that principle to UDM's background move workflow; exact IDM relocation behavior was not executed.

All five native production targets built successfully at 1.12.1; build-start source hashes remained unchanged. Runtime tests, visual inspection, installer creation, installation and publication remain deferred. Installed UDM remains 1.6.0; browser source remains 0.63.84. Full parity remains unproven.
