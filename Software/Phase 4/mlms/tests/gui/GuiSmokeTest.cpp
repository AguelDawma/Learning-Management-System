#include "TestSupport.hpp"
#include "ui/LoginWindow.hpp"
#include "ui/MainWindow.hpp"

#include <QApplication>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>

// System-level smoke tests: real windows over real services, driven without a display.
class GuiSmokeTest : public ::testing::Test {
protected:
    TempDir files;
    TempDir work;
    FixedClock* clock = nullptr;
    std::unique_ptr<AppContext> ctx;

    void SetUp() override {
        auto c = std::make_unique<FixedClock>();
        clock = c.get();
        ctx = std::make_unique<AppContext>(":memory:", files.path(), std::move(c), std::make_unique<FakeHasher>());
        seedDemoData(*ctx);
    }

    std::unique_ptr<User> signIn(const QString& username) {
        LoginWindow login(ctx->auth);
        std::unique_ptr<User> user;
        QObject::connect(&login, &LoginWindow::loggedIn, [&](User* u, bool) { user.reset(u); });
        auto edits = login.findChildren<QLineEdit*>();
        for (auto* e : edits) {
            if (e->accessibleName() == "Username") e->setText(username);
            if (e->accessibleName() == "Password") e->setText("Password1");
        }
        login.findChild<QPushButton*>()->click();
        return user;
    }

    static QTableWidget* table(QWidget& w, const QString& name) {
        for (auto* t : w.findChildren<QTableWidget*>()) {
            if (t->accessibleName() == name) return t;
        }
        return nullptr;
    }
};

TEST_F(GuiSmokeTest, WrongPasswordStaysOnLoginScreen) {
    LoginWindow login(ctx->auth);
    bool signedIn = false;
    QObject::connect(&login, &LoginWindow::loggedIn, [&](User* u, bool) {
        delete u;
        signedIn = true;
    });
    for (auto* e : login.findChildren<QLineEdit*>()) e->setText("student1");
    login.findChild<QPushButton*>()->click();
    EXPECT_FALSE(signedIn);
}

TEST_F(GuiSmokeTest, EveryRoleOpensItsOwnView) {
    auto student = signIn("student1");
    ASSERT_TRUE(student);
    {
        MainWindow w(*ctx, UserFactory::create(student->role(), *ctx->users.findById(student->id())));
        ASSERT_NE(table(w, "My assignments"), nullptr);
        EXPECT_EQ(table(w, "My assignments")->rowCount(), 2);
        EXPECT_EQ(table(w, "My released results")->rowCount(), 0);
        EXPECT_EQ(table(w, "User accounts"), nullptr); // no admin screens for students
        QLabel* courses = nullptr;
        for (auto* l : w.findChildren<QLabel*>()) {
            if (l->accessibleName() == "My courses") courses = l;
        }
        ASSERT_NE(courses, nullptr);
        EXPECT_TRUE(courses->text().contains("CS101"));
    }
    auto lecturer = signIn("lecturer1");
    ASSERT_TRUE(lecturer);
    {
        MainWindow w(*ctx, std::move(lecturer));
        EXPECT_EQ(table(w, "My assignments")->rowCount(), 2);
        EXPECT_NE(table(w, "Submissions to mark"), nullptr);
    }
    auto hod = signIn("hod1");
    ASSERT_TRUE(hod);
    {
        MainWindow w(*ctx, std::move(hod));
        EXPECT_EQ(w.findChild<QComboBox*>()->count(), 2);
        EXPECT_NE(table(w, "Moderation sheet"), nullptr);
        EXPECT_NE(table(w, "Audit log"), nullptr);
    }
    auto admin = signIn("admin");
    ASSERT_TRUE(admin);
    {
        MainWindow w(*ctx, std::move(admin));
        EXPECT_EQ(table(w, "User accounts")->rowCount(), 6);
        EXPECT_EQ(table(w, "Departments")->rowCount(), 1);
        EXPECT_EQ(table(w, "Courses")->rowCount(), 2);
    }
}

TEST_F(GuiSmokeTest, ReleasedMarkAppearsOnStudentScreen) {
    auto student = signIn("student1");
    auto lecturer = signIn("lecturer1");
    auto hod = signIn("hod1");
    const int assignment = ctx->submissionService.myAssignments(*student)[0].assignment.id();
    const auto receipt = ctx->submissionService.submit(*student, assignment, work.write("a.pdf", "%PDF"));
    ctx->marking.saveMark(*lecturer, receipt.submissionId, 77, "Good use of loops", "");

    {
        MainWindow w(*ctx, UserFactory::create(Role::STUDENT, *ctx->users.findById(student->id())));
        EXPECT_EQ(table(w, "My released results")->rowCount(), 0); // hidden until released
    }
    ctx->moderation.approve(*hod, {receipt.submissionId});
    ctx->moderation.release(*hod, {receipt.submissionId});
    {
        MainWindow w(*ctx, UserFactory::create(Role::STUDENT, *ctx->users.findById(student->id())));
        auto* results = table(w, "My released results");
        ASSERT_EQ(results->rowCount(), 1);
        EXPECT_TRUE(results->item(0, 4)->text().startsWith("77"));
    }
    {
        MainWindow w(*ctx, std::move(hod));
        EXPECT_EQ(table(w, "Audit log")->rowCount(), 5);
    }
}

int main(int argc, char** argv) {
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
