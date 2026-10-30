# Durable completion checkpoint

Manager::save previously populated its checkpoint cache before atomicText completed the catalog replacement. completionSaved then parsed the cached JSON text and checked only the saved job ID, Complete status and scanner result. This could treat a prepared snapshot as saved, and it repeatedly parsed the entire catalog while holding the manager lock.

The checkpoint cache is now published only after the catalog write succeeds, and only if its snapshot sequence is still current. Failed or superseded writes do not publish that snapshot as the current saved checkpoint. Moving the detached snapshot and serialized text into the cache avoids another full copy at publication; old-cache destruction and comparisons still occur under the manager lock.

completionSaved reads the existing checkpoint JSON directly. It also uses the existing jobMatchesCheckpoint comparison, including child media records, so a prior completed snapshot is not enough when the current job state has changed. CLI completion actions already require current Complete status, no active transfer and successful scanner status; they now additionally depend on a matching successfully written checkpoint. If state changes after a save, a later matching save may be required before the completion action proceeds.

This complements [serialization outside the manager lock](catalog-serialization-20261029.md). The file format, catalog size limit and sequence-ordered write mutex are unchanged. Source review does not establish crash durability or runtime concurrency behavior; those checks remain deferred.

All five native production targets compiled and installer packaging passed. See [build evidence](validation/catalog-completion-checkpoint-build-20261029.json); the production bundle manifest records all 147 payload hashes. Extension 0.63.76 is unchanged. No runtime tests, installation or publication are performed.
