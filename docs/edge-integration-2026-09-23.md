# Edge integration verification — 2026-09-23

UDM Browser Integration 0.15.0 is loaded, enabled and pinned in the user's regular Microsoft Edge Profile 1. Stable extension ID: kahfappnpjdcboccpnhinkcobcdgbdpl.

## Registration issue and repair

The visible Edge popup initially reported "Specified native messaging host not found." The native executable passed its direct stdio/desktop round trip, and a fresh isolated Edge 153.0.4234.48 profile passed native messaging. Those isolated successes did not prove that the regular profile worked.

A read-only diagnostic running in interactive Windows session 1 confirmed that its HKCU Edge native-messaging key was absent, even though the command environment could read a registration under the same Windows account name and SID. Running browser/register-host.ps1 in session 1 created the registration visible to Edge. The normal Edge popup then reported "UDM is connected and ready."

Registration remains per-user and points to C:\Users\Abuzar\AppData\Local\UDM\native-messaging\chromium.json, which launches D:\UDM\release\Udm.NativeHost.exe. No native-messaging policy was changed. Temporary diagnostic launch tasks were removed.

## Visible end-to-end test

1. Entered http://127.0.0.1:43821/capture-fixture.udmtest in the regular Edge UDM popup.
2. Clicked Download with UDM; popup reported Added to UDM.
3. Observed UDM's Download File Info dialog with the correct URL and completed background prefetch.
4. Clicked Start Download and observed Download complete.
5. Verified all 8,388,608 saved bytes against the deterministic fixture source and confirmed the job is Complete.

SHA-256: e59faaaa34e52df787e35c960893e70dbdbcbf3bb40c867ffc163644a6903dd2

Structured evidence: ../benchmarks/edge-connection-20260923/verified-handoff.json

The test adds one completed fixture entry to the 10 existing downloads. It validates explicit popup handoff and file integrity using localhost. It is not an Internet speed comparison or a new video-platform compatibility test. Automatic capture and optional site/cookie permissions were not enabled for this test.

Observed UI follow-up: after finalizing a fully prefetched file, the progress dialog reported Complete and 8.0 MB overall while individual connection rows displayed 0 B / 0%. The saved file is correct; these final row counters need a separate UI fix.
