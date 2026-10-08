# SABR unselected compressed track candidate

The streaming parser previously rejected a compressed segment header before checking whether it belonged to the selected video or audio tracks. A compressed unrelated side track could therefore stop a valid selected download. The native fixture reproduced this against the unchanged parser: the suite stopped at the new mixed-track case after 2,290 passes with `Compressed streaming segments are not supported.`

The parser now records an unselected segment as skipped before applying the compression restriction. Selected compressed segments remain rejected. The fixture includes both paths and verifies the selected video and audio bytes after the unrelated compressed segment. This does not add decompression support or weaken rejection of encrypted media.

The rebuilt isolated native suite passed 2,570 checks with zero failures, including both new compression cases. This is a source/binary candidate, not an installed release. Actual public-site SABR variants and IDM behavior were not measured in this check.
