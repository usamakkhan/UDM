# Per-file import request details

## Reference evidence

The retained IDMan.exe hash remains 03CC62E9ADB77A380F9DC12F67CCAAEE5106F12844AA73CE32C914DDD16D607C. The Edit command (control 1352) maps to 004c0140. Independent retained disassembly shows it constructs 005163b0 and opens that dialog for the highlighted row. The constructor creates File Properties resource 135. Its retained controls include Referer 1569, Login 1033 and Password 1041. These static findings identify a missing UDM workflow; they do not prove every reference credential edge case.

## UDM implementation

Import properties now includes an explicit per-file login/Referer override. Unchecked, the row inherits shared batch request details. Checked, its login and Referer take precedence while unrelated shared headers remain intact. Blank login/password explicitly disables inherited or remembered authorization using UDM's existing empty Authorization marker; a blank Referer removes the shared Referer. Disabling the override restores inheritance.

Rows store only the protected override header dictionary. Passwords appear only in the masked edit control and transient validation/serialization values. Existing Basic-login validation and header validation apply. Cancel does not save edits. Saving unchanged override values retains the existing protected data and avoids an unnecessary metadata refresh.

Metadata workers receive protected effective headers by original row index. The optional vector must match the address count; empty entries inherit the shared headers. Changing request overrides cancels the old metadata generation and refreshes details. Admission uses the same effective-header merger before the manager stores the job with its existing protected-header mechanism. Sorting, hiding rows and filename patterns do not change this association.

No runtime, browser or regression tests were run, per the user's request. Compilation/package results are recorded separately. The candidate remains uninstalled and unpublished.
