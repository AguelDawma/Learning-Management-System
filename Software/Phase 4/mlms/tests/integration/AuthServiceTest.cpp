#include "TestSupport.hpp"

class AuthServiceTest : public LmsFixture {};

TEST_F(AuthServiceTest, ValidLoginReturnsPolymorphicUser) {
    auto r = ctx->auth.login("lecturer1", "Password1");
    ASSERT_TRUE(r.user);
    EXPECT_EQ(r.user->role(), Role::LECTURER);
    EXPECT_FALSE(r.mustChangePassword);
    auto h = ctx->auth.login("  hod1 ", "Password1");
    EXPECT_EQ(dynamic_cast<HeadOfDepartment&>(*h.user).departmentId(), deptId);
}

TEST_F(AuthServiceTest, WrongPasswordAndUnknownUserGiveSameMessage) {
    std::string a, b;
    try { ctx->auth.login("student1", "wrong"); } catch (const AuthenticationError& e) { a = e.what(); }
    try { ctx->auth.login("nobody", "wrong"); } catch (const AuthenticationError& e) { b = e.what(); }
    EXPECT_FALSE(a.empty());
    EXPECT_EQ(a, b);
}

TEST_F(AuthServiceTest, FifthFailureLocksAccountForFifteenMinutes) {
    for (int i = 0; i < 4; ++i) EXPECT_THROW(ctx->auth.login("student1", "wrong"), AuthenticationError);
    EXPECT_EQ(ctx->users.findByUsername("student1")->failedAttempts, 4);
    EXPECT_THROW(ctx->auth.login("student1", "wrong"), AuthenticationError);
    // Locked: even the right password is refused.
    EXPECT_THROW(ctx->auth.login("student1", "Password1"), AuthenticationError);
    clock->t = clock->t.plusSeconds(14 * 60 + 59);
    EXPECT_THROW(ctx->auth.login("student1", "Password1"), AuthenticationError);
    clock->t = clock->t.plusSeconds(2);
    EXPECT_NO_THROW(ctx->auth.login("student1", "Password1"));
    EXPECT_EQ(ctx->users.findByUsername("student1")->failedAttempts, 0);
}

TEST_F(AuthServiceTest, SuccessfulLoginResetsFailureCount) {
    EXPECT_THROW(ctx->auth.login("student1", "wrong"), AuthenticationError);
    ctx->auth.login("student1", "Password1");
    EXPECT_EQ(ctx->users.findByUsername("student1")->failedAttempts, 0);
}

TEST_F(AuthServiceTest, DeactivatedAccountCannotLogIn) {
    ctx->admin.setActive(*admin, student->id(), false);
    EXPECT_THROW(ctx->auth.login("student1", "Password1"), AuthenticationError);
}

TEST_F(AuthServiceTest, ResetForcesPasswordChange) {
    ctx->admin.resetPassword(*admin, student->id(), "Temporary9");
    auto r = ctx->auth.login("student1", "Temporary9");
    EXPECT_TRUE(r.mustChangePassword);
    EXPECT_THROW(ctx->auth.changePassword(*r.user, "Temporary9", "short"), ValidationError);
    EXPECT_THROW(ctx->auth.changePassword(*r.user, "Temporary9", "Temporary9"), ValidationError);
    EXPECT_THROW(ctx->auth.changePassword(*r.user, "wrong", "NewPassword7"), ValidationError);
    ctx->auth.changePassword(*r.user, "Temporary9", "NewPassword7");
    EXPECT_FALSE(ctx->auth.login("student1", "NewPassword7").mustChangePassword);
    EXPECT_THROW(ctx->auth.login("student1", "Temporary9"), AuthenticationError);
}

TEST(AuthWithArgon2, PasswordsAreStoredAsSaltedArgon2id) {
    TempDir files;
    AppContext ctx(":memory:", files.path(), std::make_unique<FixedClock>(),
                   std::make_unique<Argon2Hasher>(1, 1024, 1));
    auto pw = ensureAdministrator(ctx);
    ASSERT_TRUE(pw);
    const auto rec = ctx.users.findByUsername("admin");
    EXPECT_EQ(rec->passwordHash.rfind("$argon2id$", 0), 0u);
    EXPECT_EQ(rec->passwordHash.find(*pw), std::string::npos);
    EXPECT_TRUE(ctx.auth.login("admin", *pw).mustChangePassword);
    EXPECT_FALSE(ensureAdministrator(ctx)); // only on first run
}
