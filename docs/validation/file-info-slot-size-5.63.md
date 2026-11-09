# File Info size while prefetch waits in UDM 5.63

When background prefetch is enabled and all transfer slots are busy, File Info keeps any size it can already display. It skips an automatic metadata request in that case. A file with an unknown size can still check the server while it waits. The prefetch response can replace the browser hint with the server's size.

An isolated native dialog fixture held the only transfer slot and opened File Info with a browser hint of 4,000 bytes. Once the slot was released, the origin saw a single GET and the dialog changed to `File size: 4097 bytes`. The fixture checked that all 4,097 response bytes had arrived and the destination was still absent before pressing Start. The published file matched the full response. The unknown-size control made a HEAD followed by one GET and also matched the final response.

All 270 native File Info checks passed, including filename preview, explicit confirmation, retained prefetch and request counts. The fixture used a local HTTP origin. Public CDN timing and a paired IDM run remain open. [Production build receipt](file-info-slot-size-5.63-build.json)
