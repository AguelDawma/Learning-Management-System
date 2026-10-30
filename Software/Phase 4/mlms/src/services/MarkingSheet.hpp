#pragma once

#include "../data/Repositories.hpp"
#include "Views.hpp"

#include <vector>

// Builds the rows shared by the lecturer's marking list and the HoD's
// moderation sheet: active submissions with days late, mark and penalty.
// Penalty and final mark are calculated, never stored (3NF).
std::vector<MarkingRow> buildMarkingRows(const Assignment& assignment, ISubmissionRepository& submissions,
                                         IAssignmentRepository& assignments, IMarkRepository& marks,
                                         IUserRepository& users, const ILatePenaltyPolicy& policy);

// Throws NotFound / InvalidTransition unless the submission exists and is the active version.
Submission requireActiveSubmission(ISubmissionRepository& submissions, int submissionId);
