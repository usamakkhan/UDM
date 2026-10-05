# Preserve replacement browser hosts during uninstall

Registration rollback previously checked manifest ownership, but uninstall deleted both manifest files unconditionally and removed registrations pointing to their paths. A replacement installation using the same manifest path could therefore lose its browser connection. The baseline reproduced 18 failed assertions.

Uninstall now receives the expected native-host executable and uses the same generated manifest contents as installation. It inspects both manifests before modifying any registrations. An unchanged owned manifest may be removed; a missing manifest permits stale-registration cleanup. A replaced or edited manifest retains its file and registrations. A directory or unreadable file at a manifest path stops ownership processing before registry changes. Other installations' registry paths and unrelated values remain untouched.

The corrected compiled shared-code fixture passes 224 checks, including valid replacement hosts, mixed ownership, missing manifests, replacement-directory preflight, rollback, Unicode paths and existing registration behavior. Combined catalog migration/registration tests pass another 134 checks using the current setup helper. Catalog and partial-file sentinels remained byte-identical. Fixture registry keys were independently verified removed.

The production installer was rebuilt. All 132 application/browser/runtime inputs, installed binaries and personal catalogs remain unchanged. Actual Edge still points to the installed D:\UDM host. The prior installer is retained; no installation, uninstall or publication was performed on the personal installation.

This fixes replacement ownership observed before uninstall. Preflight, registry deletion and manifest deletion remain separate operations: fully concurrent replacements and simultaneous installers are not atomically protected. Real elevated HKLM scopes, alternate administrator identity and multi-user lifecycle remain unqualified. Full IDM parity is still incomplete.

[Acceptance, exact hashes and cleanup evidence](D:/UDM-Workspace/candidates/release-084-055/control/uninstall-ownership-acceptance.json)
