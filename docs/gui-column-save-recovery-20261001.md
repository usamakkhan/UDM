# Column customization: save failure and recovery

A real catalog-write failure reproduced four wrong outcomes: the visible width changed despite the error, Cancel retained that width, and the canceled edit was written during normal shutdown and appeared again after restart.

Column customization now constructs the complete width/order preferences and saves them through one rollback-capable settings transaction before applying the visible layout. A failed write keeps the original layout and leaves the draft open for retry. Cancel discards it; a successful retry commits it and survives restart.

17 real-GUI fault/recovery checks passed, followed by 16 column workflow and 27 desktop regression checks. Fault injection used a directory at the temporary catalog path in a fresh private profile, without touching personal data. Tests include failed-write reporting, unchanged catalog bytes, Cancel, retry of the retained draft, normal shutdown/restart, and unchanged download records. This does not establish physical disk-loss behavior.

The paired installer contains the tested executable. Only UDM.exe changed among 132 payload inputs. Installed executables and both personal catalogs remain byte-identical. The candidate was not installed or published. Full IDM parity remains unestablished.

[Acceptance and exact artifact hashes](D:/UDM-Workspace/candidates/release-084-055/control/column-transaction-acceptance.json)
