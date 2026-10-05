# Toolbar asset recovery

The optimized UDM app exited with 0xC0000409 during startup when the external assets folder was missing or add.png was corrupt. A top-level window briefly existed before the failure, so window creation alone was insufficient evidence of successful startup.

The executable now contains recovery copies of all twelve original UDM toolbar PNGs. External icons remain the first choice; decoding failures fall back to the matching embedded icon. Custom skin behavior and normal/hot/disabled rendering are preserved. This fixes toolbar startup failure; it does not restore missing category artwork.

Verification: normal, missing-folder and corrupt-icon startup cases all remained running after two seconds. The app then passed all 27 desktop menu, queue and restart checks with no external assets. All twelve embedded resources were independently compared byte for byte with their source PNGs. The first fallback attempt failed because resource names differed from the lookup; that evidence is retained, and the final implementation uses numeric resource IDs.

The paired 0.84.0/0.55.0 installer includes the tested executable. Of 132 payload inputs, only UDM.exe changed. Previous artifacts are preserved. The installed app and both personal catalogs remain byte-identical; this candidate was not installed or published.

Full IDM parity remains unestablished. Elevated installer lifecycle, multi-user behavior, manifest concurrency and intermittent live-HLS timing remain open.

[Acceptance and artifact hashes](D:/UDM-Workspace/candidates/release-084-055/control/embedded-icons-acceptance.json)
