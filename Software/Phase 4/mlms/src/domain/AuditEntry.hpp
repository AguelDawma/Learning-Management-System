#pragma once

#include "DateTime.hpp"

#include <optional>
#include <string>

// One row of the append-only audit log: who changed what, when, and why (FR-18).
struct AuditEntry {
    int id = 0;
    int userId = 0;
    int submissionId = 0;
    std::string field;
    std::optional<std::string> oldValue;
    std::string newValue;
    std::string reason;
    DateTime timestamp;
};
