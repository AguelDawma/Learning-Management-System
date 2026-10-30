#include "TestSupport.hpp"

class AdminServiceTest : public LmsFixture {};

TEST_F(AdminServiceTest, CreatesUserWithHashedTemporaryPassword) {
    const int id = ctx->admin.createUser(*admin, "new.student", "New Student", Role::STUDENT, "Welcome123");
    const auto r = ctx->users.findById(id);
    EXPECT_EQ(r->role, Role::STUDENT);
    EXPECT_TRUE(r->mustChangePassword);
    EXPECT_NE(r->passwordHash, "Welcome123");
    EXPECT_TRUE(ctx->auth.login("new.student", "Welcome123").mustChangePassword);
}

TEST_F(AdminServiceTest, RejectsInvalidOrDuplicateUsers) {
    EXPECT_THROW(ctx->admin.createUser(*admin, "student1", "Dup", Role::STUDENT, "Welcome123"), ValidationError);
    EXPECT_THROW(ctx->admin.createUser(*admin, "Bad Name", "X", Role::STUDENT, "Welcome123"), ValidationError);
    EXPECT_THROW(ctx->admin.createUser(*admin, "okname", "  ", Role::STUDENT, "Welcome123"), ValidationError);
    EXPECT_THROW(ctx->admin.createUser(*admin, "okname", "X", Role::STUDENT, "weak"), ValidationError);
}

TEST_F(AdminServiceTest, OnlyAdministratorsManageAccounts) {
    EXPECT_THROW(ctx->admin.createUser(*lecturer, "x.y", "X", Role::STUDENT, "Welcome123"), AccessDenied);
    EXPECT_THROW(ctx->admin.listUsers(*hod), AccessDenied);
    EXPECT_THROW(ctx->admin.enrolStudent(*student, courseId, student->id()), AccessDenied);
    EXPECT_THROW(ctx->admin.createCourse(*lecturer, "CS999", "X", deptId), AccessDenied);
}

TEST_F(AdminServiceTest, CannotDeactivateSelf) {
    EXPECT_THROW(ctx->admin.setActive(*admin, admin->id(), false), ValidationError);
    EXPECT_NO_THROW(ctx->admin.setActive(*admin, student->id(), false));
    EXPECT_FALSE(ctx->users.findById(student->id())->active);
}

TEST_F(AdminServiceTest, DepartmentsCoursesAndHeads) {
    const int d = ctx->admin.createDepartment(*admin, "Mathematics");
    EXPECT_THROW(ctx->admin.createDepartment(*admin, "Mathematics"), ValidationError);
    const int newHod = ctx->admin.createUser(*admin, "hod.maths", "Maths HoD", Role::HOD, "Welcome123");
    ctx->admin.setHeadOfDepartment(*admin, d, newHod);
    EXPECT_EQ(ctx->users.findById(newHod)->departmentId, d);
    // hod1 already heads Computer Science; a lecturer is not an HoD.
    EXPECT_THROW(ctx->admin.setHeadOfDepartment(*admin, d, hod->id()), ValidationError);
    EXPECT_THROW(ctx->admin.setHeadOfDepartment(*admin, d, lecturer->id()), ValidationError);

    const int c = ctx->admin.createCourse(*admin, " ma101 ", "Algebra", d);
    EXPECT_EQ(ctx->courses.findCourse(c)->code, "MA101");
    EXPECT_THROW(ctx->admin.createCourse(*admin, "MA101", "Again", d), ValidationError);
    EXPECT_THROW(ctx->admin.createCourse(*admin, "MA102", "No dept", 9999), NotFound);
}

TEST_F(AdminServiceTest, AssignsLecturersAndEnrolsStudentsByRole) {
    const int c = ctx->admin.createCourse(*admin, "CS201", "Data Structures", deptId);
    ctx->admin.assignLecturer(*admin, c, lecturer->id());
    EXPECT_TRUE(ctx->courses.isLecturerOf(c, lecturer->id()));
    EXPECT_THROW(ctx->admin.assignLecturer(*admin, c, student->id()), ValidationError);

    ctx->admin.enrolStudent(*admin, c, student->id());
    EXPECT_TRUE(ctx->courses.isEnrolled(c, student->id()));
    EXPECT_THROW(ctx->admin.enrolStudent(*admin, c, lecturer->id()), ValidationError);
    EXPECT_EQ(ctx->admin.studentsIn(*admin, c).size(), 1u);

    ctx->admin.unenrolStudent(*admin, c, student->id());
    EXPECT_FALSE(ctx->courses.isEnrolled(c, student->id()));
    ctx->admin.removeLecturer(*admin, c, lecturer->id());
    EXPECT_TRUE(ctx->admin.lecturersOf(*admin, c).empty());
}

TEST_F(AdminServiceTest, DemoSeedFillsAnEmptyDatabaseOnly) {
    TempDir files;
    AppContext fresh(":memory:", files.path(), std::make_unique<FixedClock>(), std::make_unique<FakeHasher>());
    seedDemoData(fresh);
    EXPECT_EQ(fresh.courses.listCourses().size(), 2u);
    EXPECT_NO_THROW(fresh.auth.login("student1", "Password1"));
    seedDemoData(fresh);
    EXPECT_EQ(fresh.courses.listCourses().size(), 2u);
}
