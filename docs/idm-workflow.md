# IDM workflow observation and UDM 0.3

Observed on September 18, 2026 using the installed IDM, its supplied 6.43 build 10 installer, and the same public [YouTube video](https://www.youtube.com/watch?v=Q3TI27IN7X0). This report distinguishes observed behavior from implementation assumptions. Temporary playback URLs and credentials are omitted.

## Video download workflow

1. IDM's player menu offered MP4 at 1080p, 720p, 480p, 360p, 240p and 144p, several English TTML subtitle entries and a download-all choice.
2. **Download File Info** showed URL, category with an add-category control, destination and browse button, a remember-category-folder checkbox, description, size, Download Later, Start Download and Cancel. The 1080p entry reported 114.71 MB.
3. **Download status** showed the address, status, size, received bytes/percentage, rate, time remaining and resume capability. A green overall bar, Pause/Cancel, collapsible range map and a connection table appeared below it. The three tabs were Download status, Speed Limiter and Options on completion.
4. **Speed Limiter** showed the current rate, an enable checkbox, maximum KB/sec, and a remember-for-stop/resume option. The limiter was disabled in the observed download.
5. **Options on completion** offered a completion dialog and modem/application/computer actions. Showing the completion dialog disabled the other completion actions in the observed state.
6. The last transfer stage visibly used an audio/mp4 URL, format 140, size 9.813 MB, one receiving connection, no resume capability and a disabled Pause button. Its displayed speed was about 30–44 KB/sec. IDM then showed audio/video mixing and its completion window.
7. **Download complete** reported 130,576,510 downloaded source bytes and offered Open, Open with, Open folder, Close and Don't show again. The resulting MP4 was 130,530,947 bytes; FFprobe verified 1920×1080 H.264 video, AAC audio and a 635.785578-second duration.

The main settings had eight maximum connections and **download immediately while displaying Download File Info enabled**. Therefore, the 322.46 seconds between our Start Download click and completion observation cannot be interpreted as a full-transfer benchmark. The large video stage may have overlapped earlier observation or prefetched; its exact order and start time were not established. Empty connection rows are not counted as active connections.

The earlier IDM run showed eight active connections and sampled rates of 4.455–5.35 MB/sec. The later slow audio stage is a separate observation, not a contradiction or evidence of an overall speed ranking. [Original samples](idm-live-observation.md), [dialog observations](idm-dialog-observation.json).

## Implemented in UDM 0.3

| Area | Current implementation |
|---|---|
| Main window | Compact toolbar with original icons, dark colors, categories/queues, filename, queue, size, status, ETA, rate, last attempt and description columns |
| Browser handoff | Persists the request and opens File Info before transferring; repeated pending handoffs reuse the existing request |
| File Info | Destination/browse, category, remembered folder, description, queue, Download Later, Start Download and Cancel |
| Progress | Separate window, three tabs, received bytes/percentage, speed, ETA, resume status, overall bar, per-stream range map, real worker rows and collapsible details |
| Limiter | Temporary or remembered per-download speed limit; media audio/video share the same aggregate rate gate |
| Completion | Saved byte count, public source address, path, Open, Open with, Open folder, Close and a persistent suppression preference |
| Settings | Show/hide browser File Info, progress windows and completion windows |
| Licensing UI | No UDM activation key, trial timer or purchase gate |

Double-clicking a download opens its progress or completion window. Closing a running progress window pauses the job and retains resumable data. Closing undecided File Info preserves a paused record; Cancel removes that pending record. Download Later also preserves it as paused.

The live dialog test downloaded formats 137+140 with six video and two audio workers. It transferred 130,576,510 source bytes in 29.52 seconds: **4.22 MiB/s average**, 5.18 MiB/s highest periodic sample. Resolution took 11.92 seconds, merging 1.21 seconds, and total active processing 42.76 seconds. These are measurements of that run, not a guaranteed rate or a controlled win over IDM.

The completed UDM file has 130,762,165 bytes. Independent FFprobe inspection verified H.264 1920×1080 plus AAC and the same duration. Its independently computed SHA-256 matches the earlier UDM outputs:

```text
9aa92850711c9669b9db1d9b837c55bbb876df94900aadef5d2ec3616ef80ac1
```

Container sizes and hashes can differ between IDM and UDM because they mux the streams differently. Matching dimensions and duration alone do not establish byte-identical media tracks. [UDM run data](udm-dialog-benchmark.json).

## Remaining differences

This is a working development release, not complete or pixel-identical IDM parity. UDM does not prefetch during File Info, add custom categories there, offer subtitles/download-all, expose all IDM settings tabs, execute completion power/modem actions, dynamically split active pieces, implement full system-wide capture, or ship a kernel driver. The built-in category list and main tree also differ. The complete broader feature matrix is in [analysis.md](analysis.md).

No IDM binaries, drivers, icons or translations were copied into UDM. Third-party media helpers retain their own notices and licenses; absence of UDM activation does not remove those notices.

## Installer and driver

See [installer-analysis.md](installer-analysis.md) for the supplied setup file, observed pages, service configuration and the limits of the installation observation.
