#include "Bootstrap.hpp"

#include <random>

namespace {

std::string randomPassword() {
    static const char kChars[] = "abcdefghjkmnpqrstuvwxyzABCDEFGHJKLMNPQRSTUVWXYZ";
    std::random_device rng;
    std::string pw;
    for (int i = 0; i < 8; ++i) pw += kChars[rng() % (sizeof(kChars) - 1)];
    pw += std::to_string(10 + rng() % 90); // guarantees the digit the policy requires
    return pw;
}

int addUser(AppContext& ctx, const std::string& username, const std::string& name, Role role,
            const std::string& password, bool mustChange) {
    if (auto existing = ctx.users.findByUsername(username)) return existing->id;
    UserRecord r;
    r.username = username;
    r.fullName = name;
    r.role = role;
    r.passwordHash = ctx.hasher->hash(password);
    r.mustChangePassword = mustChange;
    return ctx.users.add(r);
}

} // namespace

std::optional<std::string> ensureAdministrator(AppContext& ctx) {
    for (const auto& u : ctx.users.listAll()) {
        if (u.role == Role::ADMINISTRATOR && u.active) return std::nullopt;
    }
    const std::string pw = randomPassword();
    if (auto existing = ctx.users.findByUsername("admin")) {
        // The only administrator was deactivated: reactivate it with a new temporary password.
        ctx.users.setActive(existing->id, true);
        ctx.users.updatePassword(existing->id, ctx.hasher->hash(pw), true);
    } else {
        addUser(ctx, "admin", "System Administrator", Role::ADMINISTRATOR, pw, true);
    }
    return pw;
}

void seedDemoData(AppContext& ctx) {
    if (!ctx.courses.listCourses().empty()) return; // only into an empty database
    const std::string pw = "Password1";
    auto tx = ctx.db.begin();
    addUser(ctx, "admin", "System Administrator", Role::ADMINISTRATOR, pw, false);
    const int hod = addUser(ctx, "hod1", "Mrs T. Mokoena (HoD)", Role::HOD, pw, false);
    const int lec = addUser(ctx, "lecturer1", "Mr K. Sello", Role::LECTURER, pw, false);
    const int s1 = addUser(ctx, "student1", "Lerato Nthane", Role::STUDENT, pw, false);
    const int s2 = addUser(ctx, "student2", "Thabo Mohapi", Role::STUDENT, pw, false);
    const int s3 = addUser(ctx, "student3", "Palesa Letsie", Role::STUDENT, pw, false);

    const int dept = ctx.courses.addDepartment("Computer Science");
    ctx.courses.setHod(dept, hod);
    const int cs = ctx.courses.addCourse(Course{0, "CS101", "Introduction to Programming", dept});
    const int ma = ctx.courses.addCourse(Course{0, "CS102", "Discrete Mathematics", dept});
    ctx.courses.assignLecturer(cs, lec);
    ctx.courses.assignLecturer(ma, lec);
    const DateTime now = ctx.clock->now();
    for (int s : {s1, s2, s3}) {
        ctx.courses.enrol(cs, s, now);
        ctx.courses.enrol(ma, s, now);
    }
    ctx.assignments.add(Assignment(0, cs, "Loops and Functions", "Write the three programs in the hand-out.",
                                   now.plusDays(7), 100, true, lec));
    ctx.assignments.add(Assignment(0, ma, "Proof Techniques", "Answer questions 1 to 5.", now.plusDays(14), 50,
                                   true, lec));
    tx->commit();
}
