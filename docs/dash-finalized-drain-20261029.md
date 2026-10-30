# Downloading the remainder of finalized DASH streams

Once a refreshed DASH manifest is static and has a finite common end, UDM now persists that boundary immediately. The prior loop delayed adopting it until its wall-clock horizon reached the end, which could keep finalized content advancing at live pace.

Subsequent static iterations plan up to 30 seconds of media per track from its committed prefix, retaining whole-segment overlap, existing count/URL budgets and cancellation checks. Successful iterations continue directly instead of sleeping for the one-second live polling interval. The status identifies this phase as downloading remaining DASH media. Transfer rate limits and byte accounting remain unchanged.

The first finalized refresh may use the window planned while the stream was live. It can continue once without requiring new media, allowing the next iteration to use the final boundary. Later static iterations must add committed segments or advance the common end. If neither happens before completion, an explicit missing-coverage error enters the existing bounded retry policy. The failure counter is no longer reset before this condition is evaluated, preventing an empty finalized window from retrying indefinitely.

The existing committed-media completion check, compatible Period transition handling, Stop, checkpoint and final assembly paths remain in place. Dynamic streams keep their live polling delay. This does not claim low-latency chunk support or change the recorded byte rate.

Extension 0.63.68 is unchanged. All five native production targets and the installer compiled successfully; compiler warnings remain. The [build record](validation/dash-finalized-drain-build-20261029.json) records source and installer hashes. The candidate is unsigned. Runtime speed, final-refresh, missing-tail, interruption and playback tests remain deferred at user request. No installation or publication is performed.
