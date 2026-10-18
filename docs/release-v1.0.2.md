# UDM 1.0.2

UDM 1.0.2 improves the confirmation and publication path for browser-captured media. It pairs the desktop update with browser integration 0.63.16.

## Changes since 1.0.1

- **Verified media size before Start:** Video and audio selections now assemble their final output into controlled staging before the confirmation dialog appears. The size shown there is the size of that final container, not a bitrate estimate.
- **Instant reuse of prepared output:** Choosing Start publishes the verified staged output directly, without a second download or mux. Destination folder and file name changes keep the same verified bytes.
- **Safer publication and recovery:** A publication journal protects a completed media file if state saving is interrupted. On recovery, UDM validates and reconciles the finished output. Cross-volume publication copies to a temporary destination, verifies it, and only then exposes the final file.
- **Cancellation and capture resilience:** Cancelling publication leaves no partial final file, and browser capture registration handles concurrent handoffs more reliably.

The release remains a single Windows setup executable containing the desktop application, browser integrations, driver runtime, and bundled media tools.
