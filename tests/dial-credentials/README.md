# Dial-up / VPN credential acceptance

Build the normal native app and tests with native/build.ps1, then run Udm.NativeTests.exe --dial-credential-checks. This targeted Windows acceptance creates a uniquely named VPN entry in a new private phone book under the executable's test-output directory. It writes only synthetic credentials, verifies saved/session/clear/keep behavior, and deletes that entry afterward. It never calls RasDial or changes the user's phone-book entries. Windows RAS/VPN device support is required.

The ordinary native suite runs pure identity/password validation without creating a phone-book entry. The native dialog harness is built with native/build.ps1 -DialCredentialsUiTestsOnly. Run Udm.DialCredentialsUiTests.exe with a new isolated output directory. It opens only owned test dialogs using injected synthetic connections and credential storage, saves rendered PNGs and results.json, and never accesses a real connection. It verifies reference field geometry, masking, per-connection drafts, Apply, Cancel, empty lists and retry after a storage failure.

Real dial-up/VPN success, EAP variants and account-policy errors still require configured connection acceptance. Passing these fixtures does not establish complete IDM parity.
