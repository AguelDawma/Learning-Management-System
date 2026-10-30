#pragma once

#include "domain/Errors.hpp"
#include "services/AppContext.hpp"
#include "services/Bootstrap.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <filesystem>
#include <fstream>
#include <memory>
#include <random>
#include <string>

// A clock the test controls, so deadline boundaries can be tested exactly.
class FixedClock : public IClock {
public:
    DateTime t = DateTime::fromCivil(2026, 11, 2, 9, 0, 0);
    DateTime now() const override { return t; }
};

// Instant stand-in for Argon2 in service tests; Argon2Hasher has its own tests.
class FakeHasher : public IPasswordHasher {
public:
    std::string hash(const std::string& pw) const override { return "fake$" + pw; }
    bool verify(const std::string& pw, const std::string& h) const override { return h == "fake$" + pw; }
};

class TempDir {
    std::filesystem::path path_;

public:
    // Lives in the build tree (not the system temp folder, which may be unset or
    // unwritable) and is unique across the parallel test processes of ctest -j.
    TempDir() {
        static std::atomic<int> counter{0};
        std::random_device rng;
        path_ = std::filesystem::u8path(MLMS_TEST_TMP_DIR) /
                ("t_" + std::to_string(rng()) + "_" + std::to_string(rng()) + "_" + std::to_string(counter++));
        std::filesystem::create_directories(path_);
    }
    ~TempDir() {
        std::error_code ec;
        std::filesystem::remove_all(path_, ec);
    }
    const std::filesystem::path& path() const { return path_; }

    std::filesystem::path write(const std::string& name, const std::string& content) const {
        auto p = path_ / name;
        std::ofstream(p, std::ios::binary) << content;
        return p;
    }
};

// An in-memory LMS with one department, an HoD, a lecturer, two enrolled
// students and a published assignment due 2026-11-07 23:59:00 UTC (max 100).
class LmsFixture : public ::testing::Test {
protected:
    TempDir uploads;
    TempDir work;
    FixedClock* clock = nullptr;
    std::unique_ptr<AppContext> ctx;

    std::unique_ptr<User> admin, hod, lecturer, student, student2, outsider, otherHod;
    int deptId = 0, courseId = 0, assignmentId = 0;
    const DateTime deadline = DateTime::fromCivil(2026, 11, 7, 23, 59, 0);

    std::unique_ptr<User> makeUser(const std::string& username, Role role) {
        UserRecord r;
        r.username = username;
        r.fullName = "Name of " + username;
        r.role = role;
        r.passwordHash = "fake$Password1";
        r.id = ctx->users.add(r);
        return loadUser(r.id);
    }

    std::unique_ptr<User> loadUser(int id) {
        auto r = ctx->users.findById(id);
        return UserFactory::create(r->role, *r);
    }

    void SetUp() override {
        auto c = std::make_unique<FixedClock>();
        clock = c.get();
        ctx = std::make_unique<AppContext>(":memory:", uploads.path(), std::move(c), std::make_unique<FakeHasher>());

        admin = makeUser("admin", Role::ADMINISTRATOR);
        lecturer = makeUser("lecturer1", Role::LECTURER);
        student = makeUser("student1", Role::STUDENT);
        student2 = makeUser("student2", Role::STUDENT);
        outsider = makeUser("student9", Role::STUDENT);
        const int hodId = makeUser("hod1", Role::HOD)->id();
        const int otherHodId = makeUser("hod2", Role::HOD)->id();

        deptId = ctx->courses.addDepartment("Computer Science");
        ctx->courses.setHod(deptId, hodId);
        const int otherDept = ctx->courses.addDepartment("Languages");
        ctx->courses.setHod(otherDept, otherHodId);
        hod = loadUser(hodId); // reload so the HoD carries its department
        otherHod = loadUser(otherHodId);

        courseId = ctx->courses.addCourse(Course{0, "CS101", "Programming", deptId});
        ctx->courses.assignLecturer(courseId, lecturer->id());
        ctx->courses.enrol(courseId, student->id(), clock->t);
        ctx->courses.enrol(courseId, student2->id(), clock->t);
        assignmentId = ctx->assignmentService.create(*lecturer, courseId, "Loops", "desc", deadline, 100, true);
    }

    std::filesystem::path file(const std::string& name = "work.pdf", const std::string& body = "%PDF my work") {
        return work.write(name, body);
    }

    // Submits at the given time and returns the new submission id.
    int submitAt(const User& who, DateTime at, const std::string& body = "%PDF my work") {
        clock->t = at;
        return ctx->submissionService.submit(who, assignmentId, file("work.pdf", body)).submissionId;
    }

    int auditCount(int submissionId) { return static_cast<int>(ctx->audit.findBySubmission(submissionId).size()); }
};
