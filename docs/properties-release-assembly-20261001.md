# Paired 0.84 / 0.55 candidate: Properties correction assembled

The paired installer now contains the completed-file Properties queue-membership correction. The desktop, native host, monitor and setup helper were assembled against the corrected engine, with live-HLS diagnostic output disabled. All four executable version resources report 0.84.0; browser payloads remain 0.55.0.

Fresh acceptance passed 27 actual desktop/queue/restart checks, seven host cold-start/reconnection checks, nine native framing checks, 15 monitor protocol/runtime checks without loading a driver, 47 setup-helper cases and ten real migration/restart checks. The 2,545-check full native result, 22 focused Properties checks, nine actual Advanced Properties checks and 132 normal live-HLS checks remain retained evidence; those were not rerun merely for packaging.

All 132 frozen package inputs were verified, and the installer compiled successfully. Previous payload executables and installer are preserved. Installed UDM, host, monitor and both personal catalogs are unchanged. This package has not been installed or published.

The elevated setup/uninstall, alternate-admin, multi-user and clean-machine gates remain open, as do simultaneous manifest replacement and intermittent live-HLS latency. Full IDM parity is not established.

[Acceptance, installer hash and exact inputs](D:/UDM-Workspace/candidates/release-084-055/control/properties-release-acceptance.json)
