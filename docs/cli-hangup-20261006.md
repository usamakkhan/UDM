# Command-line hang-up after downloading

UDM now accepts `/h` (alias `--hangup-after-download`) with `/d URL`. This option requests the existing Windows RAS dial-up/VPN disconnect action after that download completes successfully. It does not disable Ethernet or Wi-Fi adapters. Unlike `/q`, `/h` is included when forwarding a local command to an already-running UDM instance.

```powershell
UDM.exe /d "https://example.com/file.zip" /n /h /q
```

The action is delivered once after the worker finishes, the scanner permits completion, and the catalog contains the saved completion. Queued, paused, failed, and unconfirmed downloads do not execute it. A removed record cancels its action. Repeated requests for the same record do not duplicate the action. A record that was already complete when requested does not count as a new successful download. Intent lasts for the current application session and is not restored from catalog data. Ordinary browser `add` requests cannot arm it, and the native messaging host continues to reject `cli-add` requests.

The main window processes pending hang-up actions before first-instance `/q` exit. The operation calls the same `performSystemAction("Disconnect dial-up / VPN")` used by UDM's existing completion controls. Actual RAS disconnection is not part of the automated tests because it would affect the user's network connection. API return timing, real dial-up/VPN providers, duplicate-resolution changes, and exact observed IDM edge-case behavior remain unverified.

Reference: [IDM command-line documentation](https://support.internetdownloadmanager.com/support/command_line.html). The source documents `/h` as hang-up after successful download and limits `/q` to the first instance.

Build passed. Validation passed: 36 backend checks, 10 live lifecycle/guard checks, and 12 queue/native-host checks. See [validation receipt](validation/cli-hangup-20261006.json). An initial focused test build encountered an asset lock while the live app ran; its sequential retry succeeded. Test coverage is in `native/LaunchQueueTests.cpp` and `tests/cli-quit.native-live.cjs`; set `UDM_TEST_HANGUP_GUARDS=1` to include `/h` in the paused, failed, and no-URL real-app cases. Successful action delivery is tested in-process without invoking Windows disconnection. The candidate does not replace the installed app.