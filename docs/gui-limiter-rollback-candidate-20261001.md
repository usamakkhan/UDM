# Limiter save failure and Options rollback qualification

The staged shared-rate correction passed 18 native checks: 12 new settings checks plus six existing failed-close/download recovery checks.

A real Windows file handle prevents replacement of the private catalog. Both a positive-rate update and a disabled-rate update report save failure and restore the complete prior settings object; the catalog hash remains unchanged. Releasing the handle permits retry and records the new remembered rate.

A stale Options draft changing only Sound is merged with a newer disabled limiter choice through the production mergeOptionsDraft function and Manager::setSettings. The new limiter choice survives, and the intended Sound change applies. Simulating Options' follow-up failure rollback by saving its prior settings restores the exact disabled preferences on disk.

The existing failed-close scenario then completes an actual loopback download with matching file hashes. All 18 checks passed. Personal installed binaries and catalog hashes remain unchanged. Evidence: D:/UDM-Workspace/candidates/native-084-backup/control/limiter-rollback-acceptance.json.

This qualifies the native save and merge paths. It does not inject failures into the actual Windows startup registration or credential store, click through the Options dialog, establish crash/power-loss behavior, or prove full IDM parity. No additional production change was required. Candidate remains staged.
