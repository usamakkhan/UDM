# Size after a download address refresh in UDM 5.64

File Info now hides the previous response length while the replacement address is pending validation. The old size stays available to the transfer engine for saved-range validation. A metadata lookup against the replacement displays a server-reported size. A validated replacement transfer displays the file size.

The isolated native File Info fixture started both cases with a saved size of 10 MiB and refreshed the address to a local 4,097-byte response. Both dialogs initially showed an unknown size and the pending-validation message. With prefetch disabled, one HEAD produced the server-reported byte count and left the old size in the catalog. With prefetch enabled, one GET verified all 4,097 bytes before Start. The destination was absent until confirmation and the published file matched the response. The origin received no request for either old address.

All 290 File Info checks passed, including the earlier transfer-slot, single-use response, and retained-prefetch cases. The fixture uses a controlled HTTP origin. Public CDN behavior and the paired IDM comparison remain open.
