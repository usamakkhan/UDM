# Category site restriction routing — staged candidate

A reproducible routing defect allowed the predefined Video category's default extensions to classify downloads from outside its configured sites. Clearing the restricted file types also restored builtin matching. Three of six initial checks failed, including real engine admission.

Categories with only site-specific rules now skip unrestricted extension matching, including stale global type overrides in older settings. Empty restricted rules are retained so clearing types cannot reactivate builtin defaults. Turning the site restriction off restores global matching. Category properties and Options display the effective restricted types; editing types in Options updates the existing restricted rules instead of adding a global override. Generic and site-specific rule combinations are retained rather than treated as site-only.

All 23 engine translation units were rebuilt. Twelve focused checks pass, including the original three failures, actual engine admissions, persisted rules, legacy overrides, allowed subdomains and hostname boundaries. The broader rebuilt native suite passes 269 checks.

Twenty-seven actual-app checks pass in a fresh isolated profile. These include category dialog geometry/control regressions plus Options Save To displaying restricted types, Cancel preserving them, saved type changes retaining their host restriction, no global override being added, preserved download records/files and an unchanged personal Startup shortcut. Installed app/host/monitor and personal catalog hashes remain unchanged.

[Acceptance](D:/UDM-Workspace/candidates/native-084-backup/control/category-restrictions-acceptance.json) · [Before fix](D:/UDM-Workspace/candidates/native-084-backup/control/category-routing-before-results.json) · [Focused results](D:/UDM-Workspace/candidates/native-084-backup/control/category-routing-after-results.json) · [Native results](D:/UDM-Workspace/candidates/native-084-backup/control/category-restrictions-native-results.json) · [Actual-app results](D:/UDM-Workspace/candidates/native-084-backup/control/category-restrictions-app-1-results.json).

This candidate is not installed. Arbitrary site wildcard fidelity, reserved Other behavior, all category combinations and full IDM parity remain unverified or unfinished.
