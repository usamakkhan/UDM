# File-move speed and timing — 1.12.0

The background Move/Rename dialog now displays measured processing speed, total elapsed time and estimated time left for the current phase. It uses the existing five-second time-weighted SpeedMeter and resets its samples when the phase, byte total or byte counter changes. Already completed bytes are excluded from the first sample, avoiding an artificial initial speed spike. Repeated samples without progress decay the speed; ETA becomes unknown when no positive measured rate is available.

Remaining time refers to the current copy or verification phase, since later verification and history-saving work may still follow. Unknown-size phases show unavailable speed. No fabricated throughput is displayed. The destination and byte-count rows remain visible.

All five native production targets built successfully at 1.12.0; build-start source hashes remained unchanged. Runtime tests, visual checks, installer creation, installation and publication are deferred. Installed UDM remains 1.6.0; extension source remains 0.63.84. These UI changes do not prove IDM parity or measured performance.
