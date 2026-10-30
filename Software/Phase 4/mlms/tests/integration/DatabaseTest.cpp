#include "TestSupport.hpp"

class DatabaseTest : public LmsFixture {};

TEST(DatabaseSchema, InitialiseIsIdempotentAndVersioned) {
    Database db(":memory:");
    db.initialiseSchema();
    EXPECT_NO_THROW(db.initialiseSchema());
    auto st = db.prepare("PRAGMA user_version");
    ASSERT_TRUE(st.step());
    EXPECT_EQ(st.getInt(0), Database::kSchemaVersion);
}

TEST(DatabaseSchema, RefusesUnknownPrototypeSchema) {
    Database db(":memory:");
    db.exec("CREATE TABLE app_user (user_id INTEGER PRIMARY KEY)");
    EXPECT_THROW(db.initialiseSchema(), DatabaseError);
}

TEST(DatabaseSchema, BadSqlBecomesDatabaseError) {
    Database db(":memory:");
    EXPECT_THROW(db.exec("SELEC nonsense"), DatabaseError);
    EXPECT_THROW(db.prepare("SELECT * FROM missing_table"), DatabaseError);
}

TEST_F(DatabaseTest, ParameterisedQueriesTreatInputAsData) {
    // A classic injection string is just an unknown username.
    EXPECT_FALSE(ctx->users.findByUsername("' OR '1'='1").has_value());
    EXPECT_TRUE(ctx->users.findByUsername("student1").has_value());
}

TEST_F(DatabaseTest, AuditLogIsAppendOnly) {
    const int sid = submitAt(*student, deadline.plusDays(-1));
    ctx->marking.saveMark(*lecturer, sid, 70, "Clear and correct work", "");
    ASSERT_GT(auditCount(sid), 0);
    EXPECT_THROW(ctx->db.exec("UPDATE audit_log SET new_value = '100'"), DatabaseError);
    EXPECT_THROW(ctx->db.exec("DELETE FROM audit_log"), DatabaseError);
}

TEST_F(DatabaseTest, CheckConstraintsGuardTheData) {
    submitAt(*student, deadline.plusDays(-1));
    EXPECT_THROW(ctx->db.exec("UPDATE submission SET state = 'DRAFT'"), DatabaseError);
    EXPECT_THROW(ctx->db.exec("INSERT INTO app_user (username, full_name, role, password_hash) "
                              "VALUES ('x', 'x', 'PRINCIPAL', 'h')"),
                 DatabaseError);
    EXPECT_THROW(ctx->db.exec("INSERT INTO assignment (course_id, title, deadline, max_mark, created_by) "
                              "VALUES (1, 't', '2026-01-01T00:00:00Z', 0, 1)"),
                 DatabaseError);
}

TEST_F(DatabaseTest, UncommittedTransactionRollsBack) {
    {
        auto tx = ctx->db.begin();
        ctx->courses.addDepartment("Temporary");
    }
    for (const auto& d : ctx->courses.listDepartments()) EXPECT_NE(d.name, "Temporary");
}

TEST_F(DatabaseTest, NestedTransactionRollsBackOnlyInnerPart) {
    {
        auto outer = ctx->db.begin();
        ctx->courses.addDepartment("Outer");
        {
            auto inner = ctx->db.begin();
            ctx->courses.addDepartment("Inner");
        }
        outer->commit();
    }
    bool outer = false, inner = false;
    for (const auto& d : ctx->courses.listDepartments()) {
        outer = outer || d.name == "Outer";
        inner = inner || d.name == "Inner";
    }
    EXPECT_TRUE(outer);
    EXPECT_FALSE(inner);
}

TEST_F(DatabaseTest, StaleRowVersionIsRejected) {
    const int sid = submitAt(*student, deadline.plusDays(-1));
    ctx->marking.saveMark(*lecturer, sid, 70, "Clear and correct work", "");
    auto m = *ctx->marks.find(sid);
    EXPECT_EQ(ctx->marks.update(m), 2);
    EXPECT_THROW(ctx->marks.update(m), ConcurrencyError); // still holds row_version 1
}

TEST_F(DatabaseTest, ActiveSubmissionIsHighestVersion) {
    submitAt(*student, deadline.plusDays(-2), "%PDF v1");
    const int v2 = submitAt(*student, deadline.plusDays(-1), "%PDF v2");
    auto active = ctx->submissions.findActive(assignmentId, student->id());
    ASSERT_TRUE(active);
    EXPECT_EQ(active->id(), v2);
    EXPECT_EQ(active->version(), 2);
    EXPECT_EQ(ctx->submissions.findVersions(assignmentId, student->id()).size(), 2u);
    EXPECT_EQ(ctx->submissions.findActiveByAssignment(assignmentId).size(), 1u);
}

TEST(DatabaseFile, PersistsAcrossConnections) {
    TempDir dir;
    const std::string path = (dir.path() / "mlms.db").u8string();
    {
        Database db(path);
        db.initialiseSchema();
        SqliteCourseRepository(db).addDepartment("Science");
    }
    Database db(path);
    db.initialiseSchema();
    const auto depts = SqliteCourseRepository(db).listDepartments();
    ASSERT_EQ(depts.size(), 1u);
    EXPECT_EQ(depts[0].name, "Science");
}
