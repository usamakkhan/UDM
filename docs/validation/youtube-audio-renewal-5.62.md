# YouTube direct audio URL renewal in UDM 5.62

The direct-audio fixture reached its signature-renewal case after the Node sandbox was given the standard AbortController global. With 5.61, the same selected AAC track was rejected when YouTube supplied a new signature before the handoff.

5.62 moves the existing confirmation read ahead of browser request preparation. The current URL supplies the request context and reported size. The same document, visit, selected track, media resource and revision must still match. Dispatch rechecks the exact prepared URL after browser and desktop work, so another rotation or a source change before dispatch leaves no stale audio job.

Both browser bundles passed 46 direct-audio, 31 streaming-audio and 73 transport checks each. The renewal case observed one request context lookup for the new URL and exactly one native add. Resource, signed revision and metadata revision changes were rejected, as were navigation and a second URL change during preparation.

An isolated Edge profile and the staged 5.62 desktop passed all 11 browser and transfer checks. The controlled YouTube page rotated the signature after selection. The production extension submitted the renewed URL and source size for both languages before the fixture substituted its loopback origin. Spanish matched 3,470,142 bytes and English matched 3,464,393 bytes. Each file matched the source SHA-256, contained only AAC audio, and decoded through its full 180 seconds. Each transfer used one GET for the selected track.

This run exercises the real panel, extension and native transfer with controlled playback metadata and an owned origin. Public CDN renewal and the visible File Info size still need live validation.
