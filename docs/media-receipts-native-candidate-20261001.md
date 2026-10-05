# Native media admission receipts — staged 0.83 candidate

The native recovery path is implemented and qualified in the isolated candidate. The browser extension does not use it yet. This candidate is not installed, its source is not promoted over the accepted 0.82 tree, and no new installer is delivered. Full IDM parity remains unestablished.

## Behavior

Adaptive video/audio submissions can carry a durable operation token. UDM saves the accepted job and its receipt in one catalog replacement. Identical retries return the original job even after completion or restart. Changed selections or credentials under the same token, expired identities and already-removed history cannot silently create another job. Transport reply correlation IDs do not change operation identity.

A receipt query returns accepted (including whether its history row still exists), released, or uncertain. Querying a missing fresh token creates a durable barrier against a delayed original submission. Unknown old tokens remain uncertain. No automatic resend is introduced.

Receipts contain an opaque ID and a DPAPI-protected request digest, not plaintext cookies, headers or URLs. Existing adaptive-refresh routing remains available and preserves its saved review presentation. The running desktop separately advertises mediaReceipts: 1; clients must check it because a newer native host could be connected to an older desktop.

## Evidence

- 2,531 full native checks passed on final linked binaries.
- The 49 new media checks cover replay, completion, restart, removed history, credentials, malformed/expired tokens, precommit failure rollback, a postcommit lost reply, and adaptive refresh.
- Four actual child-process terminations verified that precommit crashes leave no orphan job and postcommit crashes retain the accepted receipt.
- 11 real native-host/desktop checks passed in a private catalog: unconsumed acknowledgement, query through a new host, replay with a new transport ID, desktop restart, delayed submission rejection and changed selection rejection.
- The accepted source tree, personal download catalog and installed runtime hashes remain unchanged.

Source: D:/UDM-Workspace/candidates/native-083-media-receipts/project

[Protocol and reproduction](D:/UDM-Workspace/candidates/native-083-media-receipts/project/tests/media-receipts/README.md) · [Source/binary manifest and acceptance](evidence-media-receipts-native-20261001/acceptance.json)

## Required before release

Wire durable tokens and running-desktop negotiation into the extension. Persist uncertain operations before submission, reconcile them after worker restart, and expose recovery without blindly retrying or discarding unknown outcomes. Preserve preparation deadlines. Qualify actual Edge/Firefox lost replies and restart, native adaptive-refresh presentation after crash, and compatibility with older hosts/desktops. Direct-video and YouTube media/SABR admission remain on their existing paths.

The original historical browser audio-only timeout remains unexplained. This is a newly implemented recovery mechanism, not a claimed diagnosis of that timeout or evidence of IDM's private implementation.
