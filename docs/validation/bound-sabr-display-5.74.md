# YouTube streaming address in download dialogs

For a newly accepted or renewed YouTube streaming session, UDM records that the chosen track URLs match the endpoint in the protected session. Download File Info, Progress and Download Complete can read that endpoint without copying, decrypting or parsing the full session. Legacy records and records with incomplete or inconsistent tracks use the protected session. The transfer and detailed links paths continue to read the protected request.

All 274 native streaming checks passed. They covered video and AAC audio-only admission, missing or mismatched track URLs, and two accepted session renewals. All 306 File Info checks passed. The dialog displayed the bound endpoint even with an unreadable synthetic session payload, and displayed the legacy endpoint from a valid saved session. The existing direct-file size and request-count checks also passed.

Fixture results are under `candidates/bound-sabr-display-5.74-stream-verified-tests/results` and `candidates/bound-sabr-display-5.74-file-info-tests/results`. These checks verify the opening path and address. Public YouTube and paired IDM timing remain open. [Native build receipt](bound-sabr-display-5.74-build.json)
