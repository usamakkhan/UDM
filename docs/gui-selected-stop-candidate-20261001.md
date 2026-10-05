# Selected-file Stop dialog lifecycle — staged correction

UDM's selected Stop now pauses selected active or queued downloads and closes their progress windows. It removes pending progress-open events for those jobs. Stop all and selected Stop share the same window/event cleanup helper. Completed records and unrelated downloads are excluded.

The official IDM main-window guide documents Stop for selected files and Cancel for stopping and closing a download dialog: https://www.internetdownloadmanager.com/support/main.html. This implementation follows those documented semantics; a direct live comparison with IDM was not performed.

Verification: 124 native checks passed, including six added checks with two simultaneous receiving downloads and another queued. Stopping one selected transfer closes its window while the other receiving transfer continues and the queued record/window remains unchanged. A completed record selected alongside it is preserved. The stopped transfer resumes before the batch-stop test, and the complete batch later matches expected file hashes. Thirteen standalone app menu/dialog/reopen regression checks also passed; those 13 are smoke coverage, not a separate selected-stop interaction test.

Evidence: D:/UDM-Workspace/candidates/native-084-backup/control/selected-stop-acceptance.json. The standalone app rebuilt successfully. All protected personal application/catalog hashes remain unchanged. Candidate remains staged.

Selected already-paused records retain their prior command enablement. Live recording/finalization, scanner completion races, real nested modal child workflows, scheduled queue transitions and exact visual/reference comparison remain unqualified. Full IDM parity is not established.
