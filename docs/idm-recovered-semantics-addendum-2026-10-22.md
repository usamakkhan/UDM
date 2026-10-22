# IDM recovered semantics: addendum (2026-10-22)

Date: 2026-10-22. Reference: IDM 6.43 build 10 (`IDM FIles/`, same hashes as the
2026-10-16 plan). Static only: no IDM binary was executed. This closes two items the
2026-10-16 capture-rule and file-type plan recorded as unknown, from the Ghidra
export of `IDMan.exe` and the extension bundle `IDMGCExt.crx` (read as data).

Licensing boundary unchanged: behaviour is described, no IDM code or data is shipped.

## 1. The `D` capture-rule flag (`b.J`)

The plan's §2.1 step 3 said rules with `D` are ignored when the request lacks "the
secure/proxy context bit", only partly established. The extension evaluator (`Gc`)
tests `t && r.includes("D")` with `t = !b.J`, and the request builder sets

    a.J = g & 8 || (Firefox ? a.proxyInfo && a.proxyInfo.type == "https" : this.va[g])

where `g = T(url)` is the scheme code (`wa[scheme]`; the `proxyForHttps` slot is 9,
`proxyForHttp` 1, `proxyForFtp` 2, so bit 8 marks `https:`), and `va[slot]` records
whether the proxy configured for that scheme uses an `https://` scheme (`Rb`:
`e = scheme == "https"`; a single/fallback proxy fills every slot). So:

> `D` applies only when the request is secure in transit: an `https:` address, or a
> plain address routed through an HTTPS (TLS) proxy.

UDM now computes `secure` the same way in `browser/*/capture-rules.js`: an `https:`
address or `request.proxy === 'https'`, where the extension passes the captured route
type (Chromium: the observed proxy route; Firefox: `proxyInfo.type`).

## 2. Download flag `0x100` (prefix stripping)

The plan's §1.2 said what sets flag `0x100` at `+0x164` was unknown. The export has no
isolated setter (the flags word is assigned whole), but every test of the bit sits on
media paths:

- `FUN_005da140` / `FUN_005d58a0`: return code 2 with the bit set persists
  `exLenToSkip` (the junk prefix is skipped).
- `0x004d…` (around export line 276013): with `application/vnd.lumberjack.manifest`
  and the bit set, the content type is cleared and the download is switched to the
  media handling path (`+0x348 = 1`, `+0x34c = 1`).
- Around line 282260: for non-octet/plain/audio content the bit together with the
  `+0x17a` checked byte sets `+0xb0 = 2`.
- Around line 286317: with the bit set, "Checking flv file struct" /
  "Duration==lastTimeStamp" completion checks run on the FLV.
- Around line 276447: the bit is paired with `0x2000` in the filename-pattern path.

The bit therefore marks a download created by the video/audio capture path (the
"Download video" panel), and IDM strips a text/HTML prefix only for those downloads.

UDM now mirrors this: a download created from the media panel (`MediaOutput`,
`MediaFormatId`, or a `FormatDescription` starting "Browser capture") drops a detected
prefix without asking (`StripContentPrefix` set with `StrippedPrefixReason = media`,
`StrippedPrefixBytes` recorded); any other download keeps the resume prompt.

## 3. Checked and already equivalent

- `idmcheckedtype/<ext>` is IDM's internal content-type marker after a body check;
  UDM records `ContentCheck`, `SniffedType` and `SniffedExtension` instead.
- The printable-run prefix (`FUN_100057d0`, ≥31 bytes, ≤100) and the HTML prefix
  (`FUN_10005730`, ≤2000 bytes): UDM's `sniffContent` retries strong signatures at the
  end of any text run up to 2,000 bytes, a superset.
- The error-page verdict applies in IDM only to browser-originated downloads or flag
  `+0x164 & 8`; UDM applies it to every download behind `StopOnWebPage`.

Not mirrored: ID3 metadata extraction for `audio/*` results (title, artist, album),
which IDM stores in the extended result struct; UDM's media naming does not use it.
