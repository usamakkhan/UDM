# Stop all concurrent-transfer qualification

The staged Stop all implementation passed 118 native checks, including 17 new batch checks. The fixture first verified two downloads actively receiving loopback HTTP bytes and a third queued behind the two-transfer limit.

Stop all paused all three jobs and closed their progress windows. A disabled progress window remained until it was re-enabled and received a timer callback; this simulates the ownership state of a modal child, without opening an actual nested dialog. Fifteen scheduler ticks over approximately 300 ms did not restart the stopped jobs. A previously completed record stayed unchanged. Explicitly resuming the three jobs completed every file with the expected SHA-256.

Evidence: D:/UDM-Workspace/candidates/native-084-backup/control/stop-all-multi-acceptance.json. No additional production correction was needed. The standalone candidate binary and all protected personal application/catalog hashes are unchanged.

Limits: this is a two-active/one-queued HTTP batch with individual starts, not all scheduled queue/repeat/cycle transitions. Actual modal child workflows, live-media finalization, completion races, long-duration schedule transitions and direct live IDM comparison remain unqualified. Full parity is not established. The candidate remains staged.
