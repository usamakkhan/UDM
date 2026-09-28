# Native UDM 0.42.0 / browser 0.34.0: hierarchical external DASH indexes

An external RepresentationIndex file can contain a tree of child indexes, with some entries pointing directly to media. UDM 0.33.0 handled only a flat external index. Browser 0.34.0 expands a single rooted external tree, including mixed entries and child ranges that contain their descendants, into the selected media ranges for native parallel downloads.

## Behavior and bounds

Index bytes and media bytes have separate positions. Child indexes follow the containing SIDX in the index file, while first_offset identifies media relative to its file origin. Descendant media lengths advance the media position before a subsequent direct-media reference. Parent/child stream IDs, exact rational presentation times and durations, and contiguous media bounds must match. Each SIDX found in a fetched range must belong to the chosen tree; unrelated top-level indexes are rejected.

Previously fetched bytes serve fully contained child ranges without a second request. Partial overlaps are rejected instead of combining snapshots. New child reads use the index file's own validator and exact scoped credentials; the media probe and desktop requests retain the independent media validator. Selection navigation, host permissions and the existing 12-second deadline remain enforced. Shared limits remain 64 network reads, 2 MiB index/probe bytes, 512 KiB per fetched index range, eight child levels, 2,400 references and 1,200 final plan segments. Native 0.42.0 capability is required.

## Validation

**489 browser checks across 21 suites and 161 live-browser checks passed.** [Results and request records](evidence-browser-0.34.0/summary.json) are saved with the source. Native code was unchanged; old native-test counts are not included. The extension preparation test initially hit the sandbox's child-process EPERM restriction, then passed outside the sandbox with no code change.

Independent fixture writers construct three-level and mixed trees in both 32-bit and 64-bit SIDX forms. Parser and handoff checks cover whole-file/ranged expansion, cached descendants, exact alternate timescales, bounds, depth and shared budgets, unrelated indexes, malformed children, permission withdrawal, navigation and credentials. A separate media-size guard also prevents callers of the flat external parser from omitting that bound.

Actual Chrome and Edge tests click the video panel, format choice and audio selector. Firefox uses a temporary observer calling its real page-scoped discovery and handoff; this is not a visual-button or persistent-install qualification. Eight generated outputs per browser cover whole/ranged hierarchical and mixed trees, MP4 and audio-only M4A. All fully decode, preserve the selected Spanish metadata and 880-Hz audio, and match expected duration and byte ranges. Four concurrent native media requests were observed. Index and media files use separate loopback origins and different ETags; child requests use the index ETag and native requests use the media ETag. Unselected English audio and audio-only video are never requested.

Five live child failures cover ignored Range, changed ETag, HTTP 412, redirect and wrong stream identity, with no native job. A media version changing after its probe fails in native UDM before receiving media bytes or publishing an output. Chrome separately rechecks previous flat external and hierarchical same-file workflows.

## Limits and parity

Supported scope remains clear recorded, single-period MP4 DASH. Independent top-level indexes, partial overlapping external reads, multiple SIDX boxes in a same-file child range, live recording, protected content and arbitrary site/session variants remain unsupported. Last-Modified and size-only identities retain the weaker guarantees documented in [native 0.42.0](parity-0.42.0.md). Synthetic correctness tests establish neither universal site compatibility nor performance equality with IDM.

Offset and mixed-reference interpretation follows [ISO/IEC 14496-12:2012, section 8.16.3, pages 105–107](https://ossrs.net/lts/zh-cn/assets/files/ISO_IEC_14496-12-base-format-2012-b70dd5f101daecd072700609842c9649.pdf). This is independent UDM implementation.

The [103-workflow comparison](idm-parity-browser-0.34.0.md) remains 88 implemented, 12 partial, 2 unverified and 1 user-deferred. F075 is improved but still partial. Full parity also needs the recorded driver handoff/compatibility, proxy/site, signing/updater, clean-machine, accessibility and matched-route performance acceptance work.

## Package and activation

[UDM-0.42.0-Browser-0.34.0-Setup-x64.exe](../installer-out/UDM-0.42.0-Browser-0.34.0-Setup-x64.exe) includes unchanged native 0.42.0 binaries and browser 0.34.0. SHA-256: 45BD154D8870882189D9439C7E91DF5843BC1BE1AA7746DEFAD4328731D2922E.

The existing Chrome and Edge extensions were reloaded to 0.34.0; both visibly reported "UDM is connected and ready." The running app retained all 23 download records with zero active transfers, and its catalog remained byte-identical. Firefox was tested only as a temporary isolated extension. No native binary, driver or Test Mode setting changed. The prior installer and source backups were retained. This is current-PC activation, not clean-machine installation or publisher-signature qualification. See [deployment evidence](evidence-browser-0.34.0/deployment.json).
