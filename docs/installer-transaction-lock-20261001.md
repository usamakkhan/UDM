# Serialize cooperating setup and uninstall transactions

The prior installer allowed browser registration, removal and data migration to run while another process owned the intended transaction mutex. Six baseline assertions failed: existing browser state was changed and a migration request file was created.

Current setup and uninstall acquire a shared Windows mutex at initialization, before copying or removing application files. Browser registration, unregistration and catalog migration also acquire it around their whole transactions. Recursive acquisition permits these nested operations without releasing the outer lifetime lock. A contending operation exits with an actionable retry message before its transaction writes. Normal teardown releases the lock; a later operation can acquire an abandoned mutex and revalidate existing state.

The final pass contains 400 checks: eight contention checks, thirteen real fixture lifecycle/nesting/abandonment checks, another 21 checks using unique Global mutexes, 224 ownership checks and 134 migration integration checks. The lifecycle fixture installed and uninstalled only one disposable sentinel payload and redirected browser registrations. It verified blocked setup creates no installation, blocked uninstall retains it, retry succeeds, nested registration/removal works, and release is observable while an owner remains alive. A terminated private helper supplied a real abandoned mutex.

The original uninstaller process can return while its temporary worker is finishing. Two early fixtures queried the lock too soon; the final fixture waits up to five seconds on the mutex. Those failed attempts are retained. One automatic permission review timed out before execution; its permitted retry passed.

The production installer was rebuilt; all 132 application/browser/runtime payload inputs remain unchanged, as do installed binaries, both personal catalogs and actual Edge registration. Previous installers are retained. No personal installation or publication occurred.

This serializes cooperating current UDM installers only. External tools and older installers can still cause check/use races. The tests used private Local and Global mutexes and HKCU namespaces. Global namespace access passed for the current user; elevated HKLM, alternate administrators and cross-session/multi-user behavior remain unqualified. Abandoned-lock recovery does not establish full interrupted-installation or power-loss recovery. Full IDM parity remains unfinished.

The lifecycle follows [Inno Setup's documented setup and uninstall events](https://jrsoftware.org/ishelp/topic_scriptevents.htm).

[Exact acceptance and hashes](D:/UDM-Workspace/candidates/release-084-055/control/transaction-lock-acceptance.json)
