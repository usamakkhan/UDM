# Browser 0.31.0: hierarchical recorded DASH

UDM now downloads clear recorded DASH presentations whose SegmentBase index references child SIDX indexes in the same MP4 file. Previously these offers failed when the index was opened. The extension validates and expands the selected hierarchy, then sends exact leaf ranges to the existing native 0.41.0 parallel engine. No native binaries or driver settings changed.

## Validation and limits

Child indexes must match the parent stream identity, presentation start and duration; differing timescales are compared exactly. Overlapping or out-of-order media, malformed indexes, wrong responses and page changes prevent native submission. Child requests use the root response's strong ETag, or Last-Modified when available, and reject changed validators or total size. Existing authorization opt-in and redirect restrictions apply to every read.

A selection permits at most 64 index requests, 2 MiB index bytes, eight child levels and 1,200 final segments. Each index is at most 512 KiB; each media part retains the native 256 MiB bound. The 12-second deadline covers index expansion. The final cap includes non-indexed audio/subtitle tracks too.

## Fresh tests

- **400 checks passed across 19 browser suites**, including 31 hierarchical-index checks.
- **35 live hierarchical checks passed:** Chrome 12, Edge 12, Firefox 11. Chrome and Edge clicked the real player panel and audio selector. Firefox used a temporary observer invoking the same page-scoped discovery and handoff; this is not a Firefox visual-button test.
- **10 existing flat-index Chrome checks passed** to verify the earlier workflow.
- An independently generated three-level index tree produced 360p MP4 and audio-only M4A. Decoded output was 8.021333 seconds with the selected Spanish 880 Hz audio, and no unselected English requests. Video transferred 10 parts and audio-only 5; both reached four simultaneous media requests.
- A server ignoring a child range and one changing ETag produced no native job or media request. Request logs prove exact index/media byte ranges and child If-Match headers.

All live fixtures used separate profiles and native catalogs. An initial packaging-preparation check was blocked by subprocess permissions; it passed in the final complete suite. Test evidence is in [evidence-browser-0.31.0](evidence-browser-0.31.0/summary.json). Native source was unchanged; its prior 824-check result is historical and was not rerun for this extension-only change.

## Remaining scope

Support is for clear recorded single-period MP4, with one SIDX box in each requested index range. External index resources, ranges containing multiple SIDX boxes, broader manifest/site/session compatibility, live recording and protected content remain open. The index validator does not yet bind the later native media requests. These tests establish correct output and concurrency, not IDM speed equivalence.

The [finite 103-workflow inventory](idm-parity-browser-0.31.0.md) remains 88 implemented, 12 partial, 2 unverified and 1 deferred. F075 improved but remains partial. Full IDM parity is not established.

## Format references

The independent parser follows the index fields and anchor/parent semantics in [ISO/IEC 14496-12, section 8.16.3 (archived specification)](https://ossrs.net/lts/zh-cn/assets/files/ISO_IEC_14496-12-base-format-2012-b70dd5f101daecd072700609842c9649.pdf). [DASH-IF indexed addressing](https://dashif.org/Guidelines-TimingModel/#indexed-addressing) describes SegmentBase/indexRange usage; its restricted profile is narrower than the hierarchy supported here. No IDM code is included.

## Installation

Existing Chrome and Edge installations were reloaded to 0.31.0 and both desktop-connection checks reported ready. Firefox was qualified with temporary isolated installs. Native UDM remains 0.41.0 and is running. The original 23-record history is byte-identical (SHA-256 `AAE5B9BF965F9850CC771232D988052FF5083FD809EA711B1520FB10A0ECC851`).

The new package is [UDM 0.41.0 with browser 0.31.0](../installer-out/UDM-0.41.0-Browser-0.31.0-Setup-x64.exe), SHA-256 `F17EA4556588F7AE4BAA90A43FE6AF31E93F4405C1736182C60C37F01A23A8CF`. It was built, copied and hash-verified; it was not run as a clean-machine installation test. Earlier packages were retained. No public release was created.
