#include <iostream>
#include "domain/User.hpp"
#include "domain/UserFactory.hpp"
#include "domain/Assignment.hpp"
#include "domain/Submission.hpp"
#include "domain/PenaltyStrategy.hpp"

int main() {
    std::cout << "Testing MLMS Domain Layer...\n";

    // Test User Factory & Permissions
    auto student = UserFactory::create(Role::STUDENT, 1, "alice", "Alice Smith", "hash123", true);
    std::cout << "User: " << student->fullName() << ", Role: STUDENT\n";
    std::cout << "Can submit assignment? " << (student->can("submit_assignment") ? "Yes" : "No") << "\n";

    // Test Assignment & Effective Deadline (BR-01)
    Assignment assignment(101, 1, "Lab 1", "C++ basics", "2026-06-01T23:59:59Z", 100, true, 2);
    Extension ext(101, 1, "2026-06-05T23:59:59Z", "Medical", 2, "2026-06-02");
    std::cout << "Effective Deadline with extension: " << assignment.effectiveDeadline(&ext) << "\n";

    // Test Submission & Penalty Strategy
    Submission sub(1, 101, 1, 1, "/uploads/sub.cpp", "a3b2c1...", "2026-06-03T12:00:00Z", SubmissionState::SUBMITTED);
    bool late = sub.isLate(assignment.effectiveDeadline(&ext));
    std::cout << "Is submission late? " << (late ? "Yes" : "No") << "\n";

    StandardPenaltyStrategy penalty(10.0);
    double finalMark = penalty.calculateMark(85.0, late);
    std::cout << "Final Mark after penalty: " << finalMark << "\n";

    std::cout << "Domain layer tests passed successfully!\n";
    return 0;
}