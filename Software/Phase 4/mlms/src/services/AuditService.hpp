#pragma once

#include "../data/Repositories.hpp"
#include "Views.hpp"

#include <vector>

// Read-only view of the audit log: the HoD sees their department, the
// administrator sees everything.
class AuditService {
    IAuditRepository& audit_;
    IUserRepository& users_;
    ISubmissionRepository& submissions_;
    IAssignmentRepository& assignments_;

    std::vector<AuditView> decorate(const std::vector<AuditEntry>& entries);

public:
    AuditService(IAuditRepository& audit, IUserRepository& users, ISubmissionRepository& submissions,
                 IAssignmentRepository& assignments)
        : audit_(audit), users_(users), submissions_(submissions), assignments_(assignments) {}

    std::vector<AuditView> entries(const User& viewer);
};
