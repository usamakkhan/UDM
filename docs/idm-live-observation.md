# Live IDM / YouTube observation

Observed in the user's existing Chrome and IDM session on September 18, 2026.

## Directly observed

- A floating **Download this video** panel above the YouTube player.
- A menu offering MP4 at 1080p, 720p, 480p, 360p, 240p and 144p, plus English TTML subtitle choices (including ASR).
- An active media transfer whose dialog reported **114.713 MB** file size.
- At approximately **53.29%**, the displayed rate was approximately **5.35 MB/sec**, with **13 sec** remaining and resume capability **Yes**.
- A later accessibility snapshot reported **114.222 MB (99.57%)** and **4.455 MB/sec**.
- **Eight** numbered connections had nonzero downloaded amounts and showed **Receiving data...**. Rows 9–32 were empty placeholders and must not be counted as active connections.
- A connection-position/progress strip visualized different portions of the file.
- At 100%, the dialog changed to **Mixing audio and video streams into one file...**.
- The source was a signed `googlevideo.com/videoplayback` URL with MP4 media metadata. Its full query string is intentionally not recorded here because it contains temporary identifiers and network details.

An earlier observed completion dialog reported **124.52 MB (130,576,510 bytes)**. The subsequent main list and video-transfer dialog displayed about 114.71 MB. These values refer to different observed UI stages; they should not be treated as equivalent without additional measurement.

## Correction to the earlier observation

IDM displayed a trial-expired message during the attempted redownload workflow. It was incorrect to conclude from that message alone that no transfer was running. A subsequent direct observation confirmed active transfer, eight connections, and media merging. The warning and the transfer state must be tracked separately.

## Implications for UDM

The required user experience includes automatic media detection, a player-associated download panel, rendition/subtitle selection, concurrent media transfer, per-connection progress, resume support, and audio/video assembly into a usable file.

The initial UDM engine covered only part of that chain. Version 0.2 added a floating player panel, public page-ID resolution, selected MP4 quality, concurrent video/audio downloading and FFmpeg muxing. A real Chrome-initiated 1080p download completed with audio. Subtitles and broader streaming-format coverage remain unfinished. See [UDM's measured results](benchmark.md).

The observed rates are samples, not a whole-download average or peak-speed benchmark. They do not establish an acceleration ratio, prove what part of the speed comes from a driver, or predict UDM's performance. A fair comparison needs the same rendition, endpoint, machine and network conditions with timed fresh transfers and output verification.
