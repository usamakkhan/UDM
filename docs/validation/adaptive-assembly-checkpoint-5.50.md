# Assembly checkpoint in UDM 5.50

The [5.49 timing study](adaptive-receipt-lock-5.49.md) separated response delivery from exact-size display. Two writes occur after the segment workers join and before assembly begins. The worker rewrites the complete recovery index, then saves the catalog with the Combining streams phase.

## Recovery behavior

Every completed segment already has a durable receipt with a byte count and SHA-256. Reopening the journal reads those receipts in the index's current generation and verifies each saved part before reusing it. UDM 5.50 starts assembly from those receipts without rewriting the aggregate index after the final segment. The catalog save at the assembly transition still runs. The separate checkpoint for address refresh remains before the first request for a missing segment, so rebound receipts become durable before the pending validation marker is cleared.

The control links the updated native fixture against frozen 5.49 objects. The fixture locks the aggregate index's scratch file before starting prefetch. Both segments still acquire valid receipts, but the 5.49 worker stops when the aggregate rewrite cannot open the scratch file. File Info cannot reach the verified-size gate. The 5.49 control passed 45 checks and failed two assertions that require a pre-Start size and digest. The recovery journal itself retained both receipts.

The 5.50 fixture uses the same fault. It checks for an exact size and digest before confirmation with the scratch file still locked, cancels the pending confirmation, reopens the catalog, and verifies that the same file can be assembled without requesting either segment again. The indexed-media fixture checks protected resource identity in the live per-part receipt before Start, when it is needed for recovery.

