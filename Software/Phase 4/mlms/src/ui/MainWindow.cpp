#include "MainWindow.hpp"
#include "Dialogs.hpp"
#include "RoleViews.hpp"
#include "UiHelpers.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

QString roleName(Role r) {
    switch (r) {
        case Role::STUDENT: return "Student";
        case Role::LECTURER: return "Lecturer";
        case Role::HOD: return "Head of Department";
        case Role::ADMINISTRATOR: return "Administrator";
    }
    return {};
}

} // namespace

MainWindow::MainWindow(AppContext& ctx, std::unique_ptr<User> user, QWidget* parent)
    : QMainWindow(parent), ctx_(ctx), user_(std::move(user)) {
    setWindowTitle(QString("MLMS - %1 (%2)").arg(ui::q(user_->fullName()), roleName(user_->role())));
    resize(1100, 680);

    auto* central = new QWidget;
    auto* layout = new QVBoxLayout(central);

    auto* header = new QHBoxLayout;
    auto* who = new QLabel(QString("<b>%1</b> &nbsp; %2").arg(ui::q(user_->fullName()), roleName(user_->role())));
    header->addWidget(who);
    header->addStretch();
    auto* changePw = ui::addButton(header, "Change &password...");
    auto* signOut = ui::addButton(header, "Sign &out");
    layout->addLayout(header);

    // Polymorphism: each role gets its own view; no other role's actions are shown.
    QWidget* view = nullptr;
    switch (user_->role()) {
        case Role::STUDENT: view = new StudentView(ctx_, *user_); break;
        case Role::LECTURER: view = new LecturerView(ctx_, *user_); break;
        case Role::HOD: view = new HoDView(ctx_, *user_); break;
        case Role::ADMINISTRATOR: view = new AdminView(ctx_, *user_); break;
    }
    layout->addWidget(view);
    setCentralWidget(central);

    connect(changePw, &QPushButton::clicked, this, [this] {
        ChangePasswordDialog dlg(ctx_.auth, *user_, false, this);
        dlg.exec();
    });
    connect(signOut, &QPushButton::clicked, this, [this] {
        emit signedOut();
        close();
    });
}

MainWindow::~MainWindow() = default;
