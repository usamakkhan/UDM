# Completed-file Properties and synchronization membership

The separate candidate now lets Advanced download properties edit a completed file's queue membership. The same engine validation used by main-window queue actions is applied before saving the edited record. It rejects ordinary-queue enrollment, unsupported transfers, missing saved files and deleted queues; a failed catalog write restores the complete original record. Queue removal clears pending synchronization, while a description-only edit preserves it. Downloaded file bytes are unchanged.

The failing baseline had nine failed assertions. The final candidate passes **22 focused engine checks**, **9 actual dialog checks**, **27 desktop/restart checks**, and **2545 full native checks**. Cancel/OK staging and restart persistence were tested.

The intermediate full run failed two existing live-HLS stop/save timing bounds, with publication taking 2672 ms and 2188 ms. No live-HLS correction or deadline relaxation was made; the final full-run evidence is retained separately. Those intermittent timing failures remain unresolved.

The candidate executable is separate from both the installed app and the paired installer. Installed executables, personal catalogs and all 132 accepted package inputs remain byte-identical. Release assembly is pending. This completes the scoped Properties workflow correction, not complete visual or functional IDM parity.

[Acceptance and exact hashes](D:/UDM-Workspace/candidates/release-084-055/control/completed-properties-acceptance.json)
