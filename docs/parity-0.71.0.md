# UDM 0.71.0 / browser 0.47.0 — ZIP preview before downloading

Open an ordinary ZIP download in **Download File Info** and click **Preview**. UDM reads the directory and shows file names, sizes, packed sizes and encryption flags before starting the download. HTTP/HTTPS and FTP use existing login, session and proxy settings. Preview can be cancelled; errors return to File Info.

The preview checks bounded ZIP/ZIP64 directory metadata, handles small servers without ranges, and rejects changed or inconsistent responses. It lists contents without extracting files or treating the listing as proof of payload integrity.

**1,887 final passing checks**: 1,831 native, 32 HTTP, six native dialog, 10 FTP and eight host protocol checks. [Summary](evidence-0.71.0/summary.json), [test details and initial failures](evidence-0.71.0/DEVELOPMENT.md), [dialog rendering](evidence-0.71.0/zip-preview.png).

This implements the supported remote ZIP workflow identified as R03/G23 in the [research review](idm-research-gap-review-2026-09-29.md). The current [workflow inventory](idm-parity-0.71.0.md) also corrects the toolbar and dial-up scope labels. Full IDM parity remains unestablished: recognition/capture rules, cancellation offers, toolbar skins, COM integration, driver handoff, transport comparison and distribution qualification still need work.

Browser integration stays **0.47.0**. This native release requires no extension reload.
