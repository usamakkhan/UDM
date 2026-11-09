# Completion dialog display inputs

UDM 5.71 reads the address inputs, saved path, size and scanner summary under the catalog lock. It resolves the address outside the lock and keeps only display values in the dialog. Automatic completion also reads the individual sound, progress and popup settings it uses. Completion actions retain their existing durable consumption path.

All 144 completion checks passed. The split-media fixture supplied 4,096 entries in each track journal and verified that the address input contains only the two track URLs and the source marker. The real dialog displayed the selected video URL and the saved path. Other added checks covered the legacy manifest, cached manifest without the full plan, and SABR precedence. All 207 progress-dialog checks passed with the shared address selection.

The local fixture results are under `candidates/completion-display-inputs-5.71-tests/results` and `candidates/completion-display-inputs-5.71-progress-tests/results`. The checks establish the data copied and displayed. Paired IDM timing and installed behavior remain open. [Native build receipt](completion-display-inputs-5.71-build.json)
