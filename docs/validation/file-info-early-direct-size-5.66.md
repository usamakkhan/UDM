# Earlier direct-media size in Download File Info

5.66 uses the declared response length and the recognized start of an eligible direct media download to calculate the saved byte count while prefetch is still receiving data. The content check uses the same bounded header window as the check at the end of prefetch. The response body remains staged until Start. Failed responses and fresh full retries clear the early size. A response without a declared length, or a replacement address still awaiting validation, follows the end-of-transfer check.

The local fixture serves the same synthetic audio response in two ways. One response declares its length and the other uses chunked framing. Both deliver the first 256 KiB and hold back the tail, so the dialog observation can distinguish a size calculated during transfer from one determined after EOF.

All 293 recorded File Info checks passed. File Info displayed 524,289 bytes before the tail in the declared-length case and after EOF in the chunked case. Both responses contained 60 extra bytes of HTML. The destination was absent until Start, and both published files matched the 524,289-byte payload. Each case made one GET. These checks used an owned HTTP origin and synthetic media bytes. Public CDN timing and a paired IDM run remain open. [Production build receipt](file-info-early-direct-size-5.66-build.json)
