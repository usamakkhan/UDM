# UDM browser 0.53.1: bounded video preparation and no late submission

Native UDM 0.78.0 and browser 0.53.1 are installed on disk. Personal browser activation remains unverified. Full IDM parity is not established.

## Defect and behavior

Cross-site video downloads previously had only a content-panel reply timeout. The background could continue waiting for a browser API after that timeout, then create a native download when the reply eventually arrived. A controlled baseline held the final player read beyond 65 seconds and demonstrated that exact late submission. This is a separately reproduced defect, not an established explanation of the earlier intermittent HLS audio-only timeout.

Video preparation now has a 20-second deadline. The actual native submission boundary checks that the operation is still active, including after asynchronous direct-media credentials/settings work. An expired preparation reports that no download was sent and permits a fresh attempt; its delayed browser reply cannot later create a job. HLS playlist and DASH index work receives the cancellation signal. If native submission has already started, a separate 40-second acknowledgement limit reports uncertain acceptance and tells the user to check UDM before retrying. No automatic resend is introduced. Elapsed-time checks also reject a late preparation when timer callbacks are delayed.

The two stages fit within the panel's existing 65-second message timeout under normal message delivery. Browser-worker startup/delivery remains separately bounded by the panel; a missing native acknowledgement still does not prove whether the native job was accepted.

## Evidence

- 434 targeted checks passed across 14 scripts. The 28 new checks cover tab/settings/offer/player/permission/cookie/fetch stalls, late results after retry, delayed timer callbacks, direct-media preparation, successful audio selection and uncertain native replies in both browser bundles. Regression coverage includes HLS audio, Dailymotion, hierarchical/external DASH indexes, live HLS, navigation, settings and automatic POST capture.
- 29 real isolated Edge checks passed through actual panel controls and native UDM. A real final player reply was deliberately withheld after the selected audio playlist was read. The panel displayed the preparation error in 20,149 ms. Releasing the reply created no native job; a fresh audio-only selection completed. Five files fully decoded with the chosen audio/language: nested 720p MP4, audio-only M4A, English TS, Spanish MP4 with subtitles, and a cyclic catalog's valid 720p branch. No unselected/overridden audio was fetched.
- Six more actual Edge checks passed for direct MP4. The real settings response inside the actual background handoff was delayed beyond preparation. The panel reported the timeout, and releasing the response did not call native Add. Successful attempts before and after it each produced one exact-byte decoded file.
- Screenshots of both timeout states were inspected: the message, Refresh and selection controls remain visible. [HLS/audio timeout](evidence-browser-0.53.1/site-deadline-edge-0531-20260930/preparation-timeout.png) · [Direct-video timeout](evidence-browser-0.53.1/site-deadline-direct-0531-20260930/direct-timeout.png).
- Test-only observers delay genuine API results; they do not fabricate player or native results. Runtime/native traces, request records, output hashes and the failed baseline are retained. The HLS run used the prior manifest version with the final code; packaging verified exact site/background source equivalence and that the sole manifest difference was the release version. The direct run used the final version.
- All 20 changed payload/support files are backed up and verified; all 49 native/runtime package files remain byte-identical. Two temporary host registrations and all isolated test processes were removed. No certificate, driver, hosts-file or personal browser-control changes were required.

The personal catalog's saved basket coordinates changed twice during the work. Comparisons against each preceding verified backup established that BasketX/BasketY were the only changes; downloads, queues, projects and all other fields were unchanged. Their cause was not established. The newer settings were preserved, and deployment retained the resulting entire file byte-for-byte. All 25 personal download records remain intact. [Reconciliation](evidence-browser-0.53.1/catalog-baseline.json).

Installer: `D:\UDM\installer-out\UDM-0.78.0-Browser-0.53.1-Setup-x64.exe`. SHA-256: `29a8bbe9756933b843caf3b5e7c1eaff6efcb959ac88bea89c3c777bcc47029a`. [Deployment receipt](evidence-browser-0.53.1/deployment-verification.json). Candidate/source archive: `D:\UDM-Workspace\candidates\native-0.78-browser-0.53.1-site-deadline`.

## Still open

The original `paired-edge-hls` audio-only timeout did not recur in this run. Its root cause remains unproved; this deadline fix must not be presented as a diagnosis of that failure. Discovery/catalog deadlines and more lifecycle/ad/player association cases remain separate qualification work. No new actual Firefox or public-site acceptance was performed in this release; both shipped bundles share the tested logic and have focused unit coverage.

Media receipts after lost native acceptance, current site coverage, nested audio masters, driver capture, transport fidelity, COM cold activation, full GUI/accessibility, physical recovery, matched-route speed and signed distribution remain open in the [gap register](idm-research-current-status-2026-09-29.md).
