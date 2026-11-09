# YouTube session recovery in UDM 5.30

## Scope

The 5.26 and 5.28 public 1080p jobs reached the native app, then stopped at a UMP browser-attestation rejection. Inspection of the two owned fixture catalogs found a nonempty attestation field in each original request and eight native streaming requests with no retained media bytes in each job. This establishes that the origin rejected those sessions despite the field being present. Its acceptance conditions remain unverified. [Earlier browser runs](idm-gap-work-5.26-5.28.md#other-gap-checks)

UDM 5.30 recognizes the rejection as a request for session recovery. It keeps the original download and verified segments, suppresses queued replay and background prefetch, and explains how to refresh when the user requests Resume. In File Info, Start opens the existing refresh flow while the hold is set. A newer capture must match the original selected streams. UDM lifts the hold only after saving that replacement to the same download. Cancel, selection mismatch and storage failure keep the hold in place.

This change covers UMP type 58 with reason 3. Other protocol errors and transport retries keep their existing policies. The public origin still determines whether a replacement can supply media.

## Browser fixtures

Both browser source bundles passed 19 streaming capture checks and 31 YouTube streaming audio checks each. The audio fixture needed the standard `AbortController` global added to its Node sandbox before it could exercise the extension's handoff. These checks establish capture and selection behavior in the fixture.

## Native qualification

The focused streaming suite passed 219 checks, including rejection before any bytes and after a verified initialization segment. It exercised restart, manual and scheduled retry prevention, prefetch admission, unchanged and mismatched captures, cancel, failed catalog save, successful replacement, and exact selected track bytes after reusing verified data. Complete output coverage in the suite uses owned media fixtures. The production build and the final qualification fixture are recorded separately in the receipt because the fixture transcript was updated after the production binaries were built. The five native executables, 80 recorded production/browser inputs, and 95 staged browser files matched their hashes. [Build and fixture receipt](sabr-attestation-recovery-5.30-build.json)

## Public 1080p run

An isolated Edge profile activated extension 0.68.94 and delivered exactly one 1080p job to the staged UDM 5.30 instance. The origin required browser attestation again. UDM persisted `SabrSessionRefreshRequired`, retained zero bytes, and kept the size unknown. The test produced no completed video. The fixture removed its temporary native host registration. It did not exercise a subsequent browser refresh or the File Info controls.

## Adjacent parser gap

The first native recovery transcript returned the start of an already cached initialization segment in one response and its remaining bytes in the next. The reader drops the skipped header at the first response boundary and rejects the subsequent data as missing its header. A second transcript exercises session recovery when the origin sends only the missing media. Track the split cached-segment case separately because a usable replacement session may still return duplicate data.
