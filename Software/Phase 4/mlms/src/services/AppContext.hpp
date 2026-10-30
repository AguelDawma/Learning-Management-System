#pragma once

#include "../data/Database.hpp"
#include "../data/FileStore.hpp"
#include "../data/SqliteRepositories.hpp"
#include "../domain/LatePenaltyPolicy.hpp"
#include "../infrastructure/Infrastructure.hpp"
#include "AdminService.hpp"
#include "AssignmentService.hpp"
#include "AuditService.hpp"
#include "AuthService.hpp"
#include "MarkEvents.hpp"
#include "MarkingService.hpp"
#include "ModerationService.hpp"
#include "SubmissionService.hpp"

#include <filesystem>
#include <memory>
#include <string>

// Composition root: the only place where concrete SQLite, Argon2, clock and
// policy classes are chosen (Dependency Inversion). Services see interfaces only.
// Tests build one with ":memory:", a fixed clock and a fast hasher.
struct AppContext {
    Database db;
    std::unique_ptr<IClock> clock;
    std::unique_ptr<IPasswordHasher> hasher;
    LocalFileStore files;
    TenPercentPerDayPolicy policy;

    SqliteUserRepository users{db};
    SqliteCourseRepository courses{db};
    SqliteAssignmentRepository assignments{db};
    SqliteSubmissionRepository submissions{db};
    SqliteMarkRepository marks{db};
    SqliteAuditRepository audit{db};

    AuditLogger auditLogger{audit};

    AuthService auth;
    AdminService admin;
    AssignmentService assignmentService;
    SubmissionService submissionService;
    MarkingService marking;
    ModerationService moderation;
    AuditService auditService;

    AppContext(const std::string& dbPath, const std::filesystem::path& filesRoot, std::unique_ptr<IClock> clk,
               std::unique_ptr<IPasswordHasher> hsh)
        : db(dbPath), clock(std::move(clk)), hasher(std::move(hsh)), files(filesRoot),
          auth(users, *hasher, *clock),
          admin(users, courses, *hasher, *clock, db),
          assignmentService(assignments, courses, submissions, *clock, db),
          submissionService(submissions, assignments, courses, marks, files, policy, *clock, db),
          marking(marks, submissions, assignments, courses, users, policy, *clock, db),
          moderation(marks, submissions, assignments, courses, users, policy, *clock, db),
          auditService(audit, users, submissions, assignments) {
        db.initialiseSchema(); // no member constructor above touches the database
        assignmentService.attach(&auditLogger);
        marking.attach(&auditLogger);
        moderation.attach(&auditLogger);
    }
};
