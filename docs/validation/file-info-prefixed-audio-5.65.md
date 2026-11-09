# File Info size for a prefetched response with inserted text

UDM 5.65 applies the existing browser-media content handling to a newly admitted direct YouTube M4A. When a direct response is eligible for prefix removal, File Info labels the header length as the response size. A read-only check of the completed prefetch uses the same signature rules as publication to determine the exact saved length. Start still controls stripping and publication.

The native fixture admitted a direct YouTube audio selection and substituted an owned HTTP origin. That origin supplied a 4,157-byte response with 60 bytes of inserted HTML and a 4,097-byte payload with an M4A signature. File Info showed `Response size: 4157 bytes` while the response was arriving. It showed `Exact final size: 4097 bytes` after the check, with all original bytes still staged and the destination absent. After Start the saved file matched the 4,097-byte payload byte for byte. The origin received one GET.

All 286 recorded File Info checks passed, including the address-refresh, capacity, filename, and single-use response cases. The prefix fixture proves byte counting and publication with synthetic media data. Public CDN behavior, decoded playback for this prefix case, and a paired IDM run remain open. [Production build receipt](file-info-prefixed-audio-5.65-build.json)
