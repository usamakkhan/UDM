# Remaining publication responsiveness work

Source review after the 1.2.8 build found a separate whole-file hash under the catalog lock in Duplicates.cpp::publishFile. The replacement branch revalidates ReplacementHash before ReplaceFileW, preserving edits to the original file made after download admission. This validation cannot simply be removed or replaced by the earlier admission digest.

The direct transfer path calls publishFile after its staging digest without an outer manager lock. Other callers, including LiveHls.hpp, hold the manager lock around publication. Moving only publishFile's own lock therefore would not reliably remove the stall: the recursive caller lock must also be addressed.

Replacement failure cleanup may hash the original again, and recovery independently verifies journal files. These paths have different lifetimes and must not be conflated with the 1.2.8 preparation improvement. A later change needs a cancellable preparation phase, stable file identity across the transition into publication, revalidation of job/catalog state, and preservation of rollback and journal behavior. There is no runtime timing evidence from this review; tests remain deferred.

Relevant existing acceptance cases are in DuplicateChecks.hpp: changed original, occupied previous-version path, sharing violations, catalog-save rollback and successful preservation of the old version. They were inspected, not executed. Current 1.2.8 source removes the admission-stage synchronization hash from the catalog lock only; whole-application responsiveness is not established.

The later 1.2.9 source/build implements the publication verification change; see publication-responsive-20261030.md. Runtime acceptance remains deferred.
