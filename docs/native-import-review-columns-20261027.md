# Native import property review

The native import list now displays Folder override and Description alongside file name, address, size and type. Both columns support ascending and descending case-insensitive sorting, with original row identity used for ties.

An empty folder override means the row inherits the import destination/category policy; the heading intentionally does not claim it is a resolved final path. Editing a row updates both cells immediately. Rebuilding the list after HTML or repeated-link filtering restores the values from the original row records. Admission behavior is unchanged.

This closes a UDM review gap: the properties could already be edited but could not be compared in the list. It does not assert exact visual parity with IDM. The retained IDM import evidence remains the behavioral reference, not copied implementation code.

Runtime and interactive tests are deferred at the user’s request. Compilation and packaging results are recorded separately. The candidate is not installed or published.
