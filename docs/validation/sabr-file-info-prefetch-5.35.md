# Exact YouTube file size after a session refresh

UDM 5.35 shows verified retained progress in the compact Download File Info dialog when a YouTube session requires renewal. The progress window's primary button opens the existing media recovery dialog. While File Info is still awaiting confirmation, the recovery dialog offers **Save session**. Saving the replacement returns to File Info and restarts background prefetch with the verified checkpoint.

The isolated acceptance fixture uses the real File Info controls and the actual Edge extension, native host and catalog. It starts from a verified public 1080p checkpoint at 1,038.396 seconds. It opens recovery from File Info, selects the matching offer in the browser, clicks **Save session**, and waits for a verified assembled file. Before clicking **Start Download**, it checks the visible exact-size line and confirms the destination is absent. After Start, it compares the published size and SHA-256 with the preconfirmation values and decodes both tracks through their expected ends.

The test scope starts close to the end of a held transfer. It does not measure how soon a cold public selection can produce an exact size, or how an installed browser and app behave together. Both staged 0.68.99 browser bundles passed their 19 capture and 31 audio checks.

## Public 1080p acceptance

The isolated Edge profile accepted a matching replacement on the existing 1080p job. File Info resumed prefetch and reused all **256,470,307** checkpoint bytes. All six native dialog checks passed. The compact line displayed `Exact final size: 265982413 bytes`, and the destination was absent before the fixture clicked **Start Download**.

| Check | Result |
| --- | --- |
| Verified staging and published size | 265,982,413 bytes each |
| Verified staging and published SHA-256 | `6446732df2fa58d59e66a1bcf79f14bea1c0c4370373de83ac502ad9ba8f7bfc` for both |
| H.264 video | 1920×1080, decoded to 1,072.800 seconds |
| AAC audio | Decoded to 1,072.994104 seconds |

The temporary native host registration and recovery marker were removed. The file was identical to the earlier verified 5.34 output, which is the copy retained for inspection. All five 5.35 native executables, 83 recorded inputs and 95 staged browser files matched their hashes. [5.35 build receipt](sabr-file-info-prefetch-5.35-build.json) · [Earlier publication](sabr-session-rejection-5.34.md)

The separate native capture presentation component fixture passed 140 checks, including File Info, duplicate choices and capture review. The public acceptance exercised File Info recovery directly. The progress-window shortcut still needs a live interaction check.
