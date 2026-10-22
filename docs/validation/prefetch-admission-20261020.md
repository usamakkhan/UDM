# Prefetch catalog failure recovery

A failed save in beginPrefetch left ConfirmationPending=true and removed previous
prefetch metadata in memory, although the durable record was unchanged and no
worker started. The method now restores the original job data if that admission
save fails, then reports the storage error. It still saves before starting work.

A real Windows file-sharing lock on state.json reproduced the failure. Before
the correction, the fifth check failed on in-memory restoration; the preceding
checks confirmed no transfer and unchanged durable data. After the fix, all 187
streaming/media checks passed. Reports: prefetch-admission-before-20261020.json
and prefetch-admission-after-20261020.json. The application was relinked using
the current App object and new FileWorkflows object, but is not installed yet.

The tested executable is now installed and SHA-256 verified. Receipt:
`prefetch-admission-installed-20261020.json`. UDM restarted in the background,
and the installed native-host/desktop pipe round-trip check passed. Extension
0.63.24 is unchanged and does not require another reload for this fix.
