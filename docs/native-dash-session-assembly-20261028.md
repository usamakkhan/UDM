# Joining recorded DASH Periods

Build status: all five production executables and the installer compiled successfully. The candidate is unsigned, staged, uninstalled and unpublished. Runtime joining and playback remain unverified under the testing deferral.

`assembleDashRecordingSession` accepts an ordered list of up to 128 job-owned Period folders and the selected MP4/M4A output contract. The current live worker calls it with its single legacy `.` folder. Automatic capture advancement and a persistent list of multiple Period folders remain unfinished; this change does not enable multi-Period browser admission.

Each Period is reconciled once, must be sealed, and must have a finite capture interval and consistent track/presentation identity. Only the first folder may be `.`; other names must match `period-NNNN`. Folder and Period identities cannot repeat. Across Periods, selected track-format metadata must match, absolute presentation starts must advance, and the prior known Period end must equal the next start. Captured intervals must meet without a gap or overlap. Only the first Period may start partway through its timeline; the final Period may end early after Stop. Total supported presentation timing remains bounded to seven days.

The existing per-Period assembly and protected output receipts are reused. Completed outputs are locked against writing/deletion, checked by size and SHA-256 using the open handle, and probed for the selected stream layout, codec configuration, time base and duration. Stream signatures must match before stream-copy joining. A one-Period session returns its verified existing output without a second FFmpeg merge.

Multiple compatible outputs are listed in a uniquely created, flushed local concat file using generated relative paths. Source output handles and the concat-list read lock remain held during FFmpeg execution. The existing file/pipe protocol restriction and safe concat mode apply. The joined file is probed again against the expected signature and total duration, then flushed and hashed before return. Publication remains owned by the job's existing publication transaction. No source Period output or media part is deleted by this function.

The result's start is presentation-relative, while individual Period assembly starts remain Period-relative. Joined output is not yet checkpointed as a whole session; retry can reuse individual Period receipts but would repeat the final join. Coarse duration checks and codec metadata comparisons are not decoded-frame or sample-accurate completeness proof.

Runtime media, interruption and browser tests remain deferred at the user's request. Browser 0.63.62 is unchanged. Build evidence is recorded in `validation/native-dash-session-assembly-build-20261028.json`. The candidate is staged, uninstalled and unpublished.
