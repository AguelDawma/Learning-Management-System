#include "AuditService.hpp"
#include "../domain/Errors.hpp"

#include <map>

std::vector<AuditView> AuditService::decorate(const std::vector<AuditEntry>& entries) {
    std::map<int, std::string> names;
    std::map<int, std::string> titles;
    auto nameOf = [&](int id) -> const std::string& {
        auto it = names.find(id);
        if (it == names.end()) {
            auto u = users_.findById(id);
            it = names.emplace(id, u ? u->fullName : "(unknown)").first;
        }
        return it->second;
    };
    auto titleOf = [&](int id) -> const std::string& {
        auto it = titles.find(id);
        if (it == titles.end()) {
            auto a = assignments_.find(id);
            it = titles.emplace(id, a ? a->title() : "(unknown)").first;
        }
        return it->second;
    };

    std::vector<AuditView> out;
    out.reserve(entries.size());
    for (const auto& e : entries) {
        AuditView v{e, nameOf(e.userId), "", ""};
        if (auto s = submissions_.find(e.submissionId)) {
            v.studentName = nameOf(s->studentId());
            v.assignmentTitle = titleOf(s->assignmentId());
        }
        out.push_back(std::move(v));
    }
    return out;
}

std::vector<AuditView> AuditService::entries(const User& viewer) {
    viewer.require(Permission::ViewAuditLog);
    if (const auto* hod = dynamic_cast<const HeadOfDepartment*>(&viewer)) {
        if (hod->departmentId() == 0) return {};
        return decorate(audit_.findByDepartment(hod->departmentId()));
    }
    return decorate(audit_.findAll());
}
