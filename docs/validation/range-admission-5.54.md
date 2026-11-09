# Fresh HTTP range admission in UDM 5.54

On the two-slot loopback origin, the staged 5.53 app verified both 24 MiB outputs and made six and eight refused connection attempts. These refusals are counted at TCP admission, separately from accepted HTTP requests. Several range workers can claim and split work after the midpoint response has been verified, before an earlier extra worker has learned whether its connection will be accepted.

UDM 5.54 lets one additional worker claim a range while an existing response is active and the additional worker has yet to validate its response. After a refused open, idle workers wait for the existing three-second busy probe interval while an accepted response is available. A worker that just finished can continue with the existing connection reuse path. The total-outage detector clears the expansion wait when all responses have stopped. The additional admission gate applies to fresh transfers.

The native cap fixture now counts sockets as soon as the server accepts them. This makes the two-slot limit apply during the fixture's setup delay as well. It requests eight workers, checks the complete file against the expected digest, and counts refused sockets independently of useful HTTP requests.

The frozen 5.53 core passed the 68 other transfer checks. It completed the capped download with the expected digest and failed the new admission check with ten refused opens.

The staged 5.54 build passed 67 of 69 checks. The capped case finished the exact file with six refused opens, above the fixture's two-refusal threshold. The reset-every-2-MiB case verified the file but took longer than its six-second bound. Total-outage recovery opened a connection 47 ms after the blackout ended. [Build receipt](range-admission-5.54-build.json)

Two staged 24 MiB diagnostic downloads against the two-slot origin each made three accepted HTTP requests and three refused opens. A second group of refusals appeared just before the origin finished serving a response whose assigned range had already been shortened. The engine had treated the finished assignment as a connection it could immediately reuse. The 5.55 follow-up distinguishes a fully read response from one closed at a split boundary.
