# Synchronization local integrity — 1.2.6

The current synchronization worker compared local size and remote size/validators before reporting Up to date. A same-length local edit could therefore pass despite a saved SHA-256 describing different bytes.

When the metadata matches and the saved SHA-256 is valid, the worker now verifies the local file against that digest. A mismatch enters the existing replacement workflow, which preserves the old version. File-open or read failures follow the existing synchronization failure path. Hashing runs outside the manager lock, checks cancellation between chunks, and holds a read-only sharing handle so another writer cannot change the file during verification. The handle closes before replacement admission. Existing records without a valid hash retain their metadata-based behavior; this change does not establish content integrity for those legacy records.

This also handles converted Site Grabber files: local integrity uses the final-file Sha256 while remote metadata still uses GrabberSourceBytes when appropriate. Extension 0.63.82 is unchanged. Source version is 1.2.6. All five production binaries compiled as 1.2.6 and installer packaging passed; runtime tests remain deferred by user instruction. See [build evidence](validation/sync-local-integrity-build-20261030.json). Installed version remains 1.2.5 until a later installation.

The menu comparison was also rechecked: QueueMenus.hpp already populates named Start/Stop queue submenus dynamically. The older GUI audit's description of only Start/Choose queue entries describes construction placeholders, not the current popup behavior; no duplicate menu implementation is needed.
