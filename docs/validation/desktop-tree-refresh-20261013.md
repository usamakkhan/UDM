# Desktop navigation-tree refresh, 2026-10-13

The 500 ms desktop refresh previously serialized the full queue and Grabber
project catalogs before checking whether the navigation tree had changed.
During site exploration, a project can hold thousands of links and update its
progress frequently, so this repeated work could also rebuild the tree even
when every visible tree label and icon was unchanged.

The comparison now reads only category names, queue names and indicator kinds,
and Grabber project IDs and names. Tree construction also iterates projects
without copying their link catalogs. The real-window fixture inserts a project
with 2,000 links and verifies that its tree comparison stays below 4 KiB,
progress updates preserve the existing tree, and project renames and queue
indicator changes still rebuild it.

The real-window `Udm.MenuStateTests.exe` passed **374 checks, 0 failed**. The
production `UDM.exe` built and linked successfully. Candidate SHA-256 hashes:

| Binary | SHA-256 |
| --- | --- |
| `UDM.exe` | `E347730C94EB834095E7BBA9E452A06CE7B8340D6E59D882CB795AC9F4C9F5F9` |
| `Udm.MenuStateTests.exe` | `609D5C249F4764ECDAC5D378A9E424FD30E5A343286CA57BC69AFCE6024D0E99` |

The candidate was built and tested in an isolated temporary directory on drive
C. No installed app was changed. This verifies less work in the refresh path;
it is not a measured end-to-end startup or IDM speed comparison.
