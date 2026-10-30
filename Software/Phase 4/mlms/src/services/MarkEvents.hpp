#pragma once

#include "../data/Repositories.hpp"
#include "../domain/DateTime.hpp"

#include <optional>
#include <string>
#include <vector>

struct MarkChangeEvent {
    int userId = 0;
    int submissionId = 0;
    std::string field;
    std::optional<std::string> oldValue;
    std::string newValue;
    std::string reason;
    DateTime at;
};

// Observer pattern: MarkingService, ModerationService and AssignmentService
// publish every mark or state change; observers react without the services
// knowing who they are.
class IMarkObserver {
public:
    virtual ~IMarkObserver() = default;
    virtual void onMarkChanged(const MarkChangeEvent& e) = 0;
};

class MarkSubject {
    std::vector<IMarkObserver*> observers_;

public:
    void attach(IMarkObserver* o) { observers_.push_back(o); }

protected:
    // Called inside the service's transaction, so the audit row commits or
    // rolls back together with the mark it describes (FR-18).
    void notify(const MarkChangeEvent& e) const {
        for (auto* o : observers_) o->onMarkChanged(e);
    }
};

// Writes each change to the append-only audit_log table.
class AuditLogger : public IMarkObserver {
    IAuditRepository& audit_;

public:
    explicit AuditLogger(IAuditRepository& audit) : audit_(audit) {}
    void onMarkChanged(const MarkChangeEvent& e) override {
        audit_.append(AuditEntry{0, e.userId, e.submissionId, e.field, e.oldValue, e.newValue, e.reason, e.at});
    }
};

// "45.5" rather than "45.500000" in the audit log and the UI.
std::string formatMark(double value);
