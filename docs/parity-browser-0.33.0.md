# Native UDM 0.42.0 / browser 0.33.0: separate DASH indexes

Recorded DASH manifests can put their segment index in a separate file. UDM previously rejected that layout. The extension now reads a single flat RepresentationIndex SIDX, either the whole index file or its declared byte range, and sends the selected media ranges to the existing native parallel downloader.

## Behavior

- External offsets start at the media-file origin, independently of the index file's byte position. Same-file indexes retain their original offset rules and bounded hierarchical expansion.
- Opening a format menu performs no index or media-probe requests. Only the selected video/audio tracks are expanded. Audio-only omits video requests.
- A one-byte media range probe obtains the actual media size and available validator. Index-file identity never substitutes for media identity. The native plan includes media resources, not external index files.
- Permissions and opt-in captured Authorization are checked separately for each exact resource. Redirects are refused. Bounds are 512 KiB per index, 64 reads, 2 MiB total index/probe budget, 1,200 final segments and a 12-second selection deadline.
- Native 0.42.0 capability is checked before index reads. Conditional requests and protected resume metadata retain the [0.42.0 guarantees and limits](parity-0.42.0.md), including the weaker identity of date-only/size-only servers.

## Fresh validation

**446 browser checks across 20 suites and 105 live-browser checks passed.** [Machine-readable evidence](evidence-browser-0.33.0/summary.json) retains individual results and request records. Native binaries were unchanged; prior native/protocol counts are not included in these fresh totals.

Chrome and Edge use the actual extension panel, format action and audio selector. Firefox uses a temporary observer invoking the extension's real page-scoped discovery/handoff; this does not qualify its visual buttons or a persistent user installation. The external-index fixture uses independent loopback origins for indexes and media, generated clear 360p MP4 and separate English/Spanish audio. Whole/ranged indexes produce decoded MP4 and M4A with Spanish metadata, the expected 880-Hz audio, correct duration, exact native ranges and four concurrent media requests. No unselected rendition is fetched. External index and media validators deliberately differ.

Fault cases reject a range-ignoring index, range-ignoring media probe, oversized index and index redirect before native transfer. A media file changed after its probe is rejected by the desktop before any media bytes or output publication. Browser unit tests additionally cover permission, credential, navigation, size, offset, truncation and capability cases. Same-file hierarchical and flat compatibility were rechecked in Chrome.

The test fixtures were corrected during qualification: an escaped route first returned 404, and an oversized response originally declared more bytes than it sent, exercising truncation instead. The final oversized case sends the actual oversized body. An earlier Chrome run stalled before preview playback; the subsequent diagnostic-enabled run passed. All attempts remain in the local staging folder.

## Scope and reference

This is a bounded single-period, clear recorded MP4 DASH implementation. External hierarchical indexes, multiple SIDX boxes within one selected range, live recording, protected media and arbitrary real-site/session coverage remain open. These generated-media tests establish transfer correctness for the stated layouts, not universal compatibility or equal speed to IDM.

External offset interpretation follows ISO/IEC 14496-12 section 8.16.3 ([2012 text, pages 105–107](https://ossrs.net/lts/zh-cn/assets/files/ISO_IEC_14496-12-base-format-2012-b70dd5f101daecd072700609842c9649.pdf)). The supported single-index profile and alternate index locator follow [DASH-IF interoperability guidance v3.1, page 25](https://dashif.org/docs/DASH-IF-IOP-v3.1.pdf). UDM implements these formats independently.

The [103-workflow comparison](idm-parity-browser-0.33.0.md) remains 88 implemented, 12 partial, 2 unverified and 1 user-deferred. F075 is improved but still partial. Driver-level automatic handoff and broader compatibility, remaining proxy/site workflows, publisher signing/updater and clean-machine testing, accessibility and matched-route IDM speed acceptance remain open. Full IDM parity is not established.

## Package and activation

[UDM-0.42.0-Browser-0.33.0-Setup-x64.exe](../installer-out/UDM-0.42.0-Browser-0.33.0-Setup-x64.exe) includes unchanged native 0.42.0 binaries and browser 0.33.0. SHA-256: FFE177703DDD89AA7B01784EFFC6AF0528D4B2EDF6A07F8F2FAA612279F629F8.

The existing Chrome and Edge extensions were reloaded to 0.33.0; both visibly reported "UDM is connected and ready." The running app retained all 23 download records with zero active transfers, and its catalog remained byte-identical. Firefox was tested only as a temporary isolated extension. No native binary, driver or Test Mode setting changed. The prior installer and source backups were retained. This is current-PC activation, not clean-machine installation or publisher-signature qualification. See [deployment evidence](evidence-browser-0.33.0/deployment.json).
