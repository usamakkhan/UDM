# Revalidate adaptive media hosts at handoff

Chromium and Firefox now record the origins they used while reading HLS playlists and DASH indexes, then recheck permission at native dispatch. The guard also applies to batch review. If a checked origin loses access while the selection is being prepared, the extension reports the change before sending a download request.

The external-index fixture now invokes the same pre-send validation callback as the native bridge. It verifies navigation after the last media probe and permission loss for either the index host or media host. All 44 external-index checks passed.

The Period fixture now checks a standalone explicit duration, matching end times, and rejection of conflicting end times. The inconsistent fixture had an explicit end at 10 seconds and an MPD end at 12 seconds. The DASH-IF timing model requires those ends to agree. All 16 Period-start checks passed in Chromium and Firefox. [DASH-IF timing model](https://dashif.org/Guidelines-TimingModel/)

All 13 focused browser suites passed, including all eight DASH suites, the HLS site catalog, receipt recovery, and capture and transport handoff.

The new permission case failed against the frozen 5.90 extension because its handoff did not reject the revocation. The isolated 5.91 Edge and desktop run passed 10 DASH checks. With access granted it fully decoded indexed MP4 and M4A files and verified the selected Spanish audio and exact media ranges. With permission denial injected after the final probe, the wire log showed both index reads and both one-byte probes, followed by zero media segment requests and no new desktop job. The separate isolated Edge HLS run passed 25 checks across MP4, TS and M4A.

The fixture logs are in the Windows local temporary folder under `udm-adaptive-permission-dispatch-5.91`. Both temporary native host registrations were removed.

[Production build receipt](adaptive-permission-dispatch-5.91-build.json)
