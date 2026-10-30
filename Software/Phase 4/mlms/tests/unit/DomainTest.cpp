#include "domain/Assignment.hpp"
#include "domain/Errors.hpp"
#include "domain/Mark.hpp"
#include "domain/UserFactory.hpp"
#include "domain/Validation.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <limits>

TEST(Assignment, ValidatesTitleAndMaxMarkBoundaries) {
    const DateTime d(1000);
    EXPECT_THROW(Assignment(0, 1, "   ", "", d, 100, true, 1), ValidationError);
    EXPECT_THROW(Assignment(0, 1, "T", "", d, 0, true, 1), ValidationError);
    EXPECT_NO_THROW(Assignment(0, 1, "T", "", d, 1, true, 1));
    EXPECT_NO_THROW(Assignment(0, 1, "T", "", d, 1000, true, 1));
    EXPECT_THROW(Assignment(0, 1, "T", "", d, 1001, true, 1), ValidationError);
}

TEST(Assignment, EffectiveDeadlineUsesExtension) {
    const Assignment a(1, 1, "T", "", DateTime(1000), 100, true, 1);
    EXPECT_EQ(a.effectiveDeadline(nullptr), DateTime(1000));
    const Extension e{1, 5, DateTime(5000), "Illness", 2, DateTime(900)};
    EXPECT_EQ(a.effectiveDeadline(&e), DateTime(5000));
}

TEST(Mark, RawMarkBoundaries) {
    EXPECT_THROW(Mark::validateRaw(-0.5, 100), ValidationError);
    EXPECT_NO_THROW(Mark::validateRaw(0, 100));
    EXPECT_NO_THROW(Mark::validateRaw(100, 100));
    EXPECT_THROW(Mark::validateRaw(100.5, 100), ValidationError);
    EXPECT_THROW(Mark::validateRaw(std::nan(""), 100), ValidationError);
    EXPECT_THROW(Mark::validateRaw(std::numeric_limits<double>::infinity(), 100), ValidationError);
}

TEST(Mark, FeedbackNeedsTenCharactersAfterTrimming) {
    EXPECT_THROW(Mark::validateFeedback("123456789"), ValidationError);
    EXPECT_THROW(Mark::validateFeedback("   12345678   "), ValidationError);
    EXPECT_NO_THROW(Mark::validateFeedback("1234567890"));
}

TEST(Roles, EachRoleHasOnlyItsPermissions) {
    UserRecord r;
    r.id = 1;
    r.username = "u";
    auto student = UserFactory::create(Role::STUDENT, r);
    auto lecturer = UserFactory::create(Role::LECTURER, r);
    auto hod = UserFactory::create(Role::HOD, r);
    auto admin = UserFactory::create(Role::ADMINISTRATOR, r);

    EXPECT_TRUE(student->can(Permission::SubmitAssignment));
    EXPECT_FALSE(student->can(Permission::MarkSubmission));
    EXPECT_TRUE(lecturer->can(Permission::MarkSubmission));
    EXPECT_TRUE(lecturer->can(Permission::GrantExtension));
    EXPECT_FALSE(lecturer->can(Permission::ReleaseMarks)); // Conflict 1: only the HoD releases
    EXPECT_TRUE(hod->can(Permission::ReleaseMarks));
    EXPECT_FALSE(hod->can(Permission::ManageUsers));
    EXPECT_TRUE(admin->can(Permission::ManageUsers));
    EXPECT_FALSE(admin->can(Permission::MarkSubmission));

    EXPECT_THROW(student->require(Permission::ManageUsers), AccessDenied);
    EXPECT_NO_THROW(admin->require(Permission::ManageUsers));
}

TEST(Roles, InactiveUserHasNoAccess) {
    UserRecord r;
    r.active = false;
    auto admin = UserFactory::create(Role::ADMINISTRATOR, r);
    EXPECT_THROW(admin->require(Permission::ManageUsers), AccessDenied);
}

TEST(UserFactory, BuildsSubclassFromRole) {
    UserRecord r;
    r.departmentId = 7;
    EXPECT_EQ(UserFactory::create(Role::STUDENT, r)->role(), Role::STUDENT);
    auto hod = UserFactory::create(Role::HOD, r);
    ASSERT_NE(dynamic_cast<HeadOfDepartment*>(hod.get()), nullptr);
    EXPECT_EQ(dynamic_cast<HeadOfDepartment*>(hod.get())->departmentId(), 7);
    EXPECT_EQ(roleFromString(toString(Role::ADMINISTRATOR)), Role::ADMINISTRATOR);
    EXPECT_THROW(roleFromString("PRINCIPAL"), ValidationError);
}

TEST(Validation, UsernameRules) {
    EXPECT_NO_THROW(validateUsername("t.mokoena_2"));
    EXPECT_THROW(validateUsername("ab"), ValidationError);
    EXPECT_THROW(validateUsername(std::string(33, 'a')), ValidationError);
    EXPECT_THROW(validateUsername("Upper"), ValidationError);
    EXPECT_THROW(validateUsername("has space"), ValidationError);
    EXPECT_THROW(validateUsername("x';DROP"), ValidationError);
}

TEST(Validation, PasswordRules) {
    EXPECT_NO_THROW(validatePasswordStrength("Password1"));
    EXPECT_THROW(validatePasswordStrength("Pass1"), ValidationError);
    EXPECT_THROW(validatePasswordStrength("passwordonly"), ValidationError);
    EXPECT_THROW(validatePasswordStrength("12345678"), ValidationError);
}

TEST(Validation, TrimAndBlank) {
    EXPECT_EQ(trim("  a b \n"), "a b");
    EXPECT_EQ(trim(" \t "), "");
    EXPECT_THROW(requireNotBlank("  ", "Reason"), ValidationError);
}
