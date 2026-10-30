#include "Dialogs.hpp"
#include "RoleViews.hpp"
#include "UiHelpers.hpp"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include <map>

using namespace ui;

namespace {

QString roleText(Role r) {
    switch (r) {
        case Role::STUDENT: return "Student";
        case Role::LECTURER: return "Lecturer";
        case Role::HOD: return "Head of Department";
        case Role::ADMINISTRATOR: return "Administrator";
    }
    return {};
}

} // namespace

AdminView::AdminView(AppContext& ctx, const User& user, QWidget* parent) : QTabWidget(parent), ctx_(ctx), user_(user) {
    // ---- Users
    auto* usersPage = new QWidget;
    auto* ul = new QVBoxLayout(usersPage);
    users_ = makeTable({"Username", "Full name", "Role", "Active", "Must change password", "Locked until"}, usersPage);
    users_->setAccessibleName("User accounts");
    ul->addWidget(users_);
    auto* ub = new QHBoxLayout;
    auto* newUser = addButton(ub, "&New user...");
    auto* resetPw = addButton(ub, "&Reset password...", "Set a temporary password; also unlocks the account");
    auto* toggle = addButton(ub, "&Activate / deactivate");
    ub->addStretch();
    ul->addLayout(ub);
    addTab(usersPage, "Users");

    // ---- Departments and courses
    auto* orgPage = new QWidget;
    auto* grid = new QGridLayout(orgPage);
    departments_ = makeTable({"Department", "Head of Department"}, orgPage);
    courses_ = makeTable({"Code", "Title", "Department"}, orgPage);
    lecturers_ = makeTable({"Lecturers of the selected course"}, orgPage);
    students_ = makeTable({"Students enrolled in the selected course"}, orgPage);
    departments_->setAccessibleName("Departments");
    courses_->setAccessibleName("Courses");
    lecturers_->setAccessibleName("Lecturers of the selected course");
    students_->setAccessibleName("Students of the selected course");

    auto* db = new QHBoxLayout;
    auto* newDept = addButton(db, "New &department...");
    auto* setHod = addButton(db, "Set &Head...");
    db->addStretch();
    auto* cb = new QHBoxLayout;
    auto* newCourse = addButton(cb, "New &course...");
    cb->addStretch();
    auto* lb = new QHBoxLayout;
    auto* addLec = addButton(lb, "Assign &lecturer...");
    auto* remLec = addButton(lb, "Remove lecturer");
    lb->addStretch();
    auto* sb = new QHBoxLayout;
    auto* enrol = addButton(sb, "&Enrol student...");
    auto* unenrol = addButton(sb, "Unenrol student");
    sb->addStretch();

    grid->addWidget(new QLabel("<b>Departments</b>"), 0, 0);
    grid->addWidget(new QLabel("<b>Courses</b>"), 0, 1);
    grid->addWidget(departments_, 1, 0);
    grid->addWidget(courses_, 1, 1);
    grid->addLayout(db, 2, 0);
    grid->addLayout(cb, 2, 1);
    grid->addWidget(lecturers_, 3, 0);
    grid->addWidget(students_, 3, 1);
    grid->addLayout(lb, 4, 0);
    grid->addLayout(sb, 4, 1);
    addTab(orgPage, "Departments and courses");
    addTab(makeAuditLogTab(ctx_, user_, this), "Audit log");

    connect(newUser, &QPushButton::clicked, this, [this] {
        UserDialog dlg(ctx_, user_, this);
        if (dlg.exec() == QDialog::Accepted) reloadUsers();
    });
    connect(resetPw, &QPushButton::clicked, this, [this] {
        const auto id = currentId(users_);
        if (!id) return;
        bool ok = false;
        const QString pw = QInputDialog::getText(this, "Reset password",
                                                 "Temporary password (the user must change it at next sign-in):",
                                                 QLineEdit::Normal, {}, &ok);
        if (ok && guarded(this, [&] { ctx_.admin.resetPassword(user_, *id, s(pw)); })) {
            QMessageBox::information(this, "Password reset", "The temporary password has been set.");
            reloadUsers();
        }
    });
    connect(toggle, &QPushButton::clicked, this, [this] {
        const auto id = currentId(users_);
        if (!id) return;
        const auto* rec = userById(*id);
        if (rec && guarded(this, [&] { ctx_.admin.setActive(user_, *id, !rec->active); })) reloadUsers();
    });
    connect(newDept, &QPushButton::clicked, this, [this] {
        bool ok = false;
        const QString name = QInputDialog::getText(this, "New department", "Department name:", QLineEdit::Normal, {}, &ok);
        if (ok && guarded(this, [&] { ctx_.admin.createDepartment(user_, s(name)); })) reloadOrganisation();
    });
    connect(setHod, &QPushButton::clicked, this, [this] {
        const auto dept = currentId(departments_);
        if (!dept) return;
        const auto hod = pickUser(Role::HOD, "Choose the Head of Department");
        if (hod && guarded(this, [&] { ctx_.admin.setHeadOfDepartment(user_, *dept, *hod); })) reloadOrganisation();
    });
    connect(newCourse, &QPushButton::clicked, this, [this] {
        CourseDialog dlg(ctx_, user_, this);
        if (dlg.exec() == QDialog::Accepted) reloadOrganisation();
    });
    connect(courses_, &QTableWidget::itemSelectionChanged, this, [this] { reloadCourseMembers(); });
    connect(addLec, &QPushButton::clicked, this, [this] {
        const auto course = currentId(courses_);
        if (!course) return;
        const auto lec = pickUser(Role::LECTURER, "Choose the lecturer");
        if (lec && guarded(this, [&] { ctx_.admin.assignLecturer(user_, *course, *lec); })) reloadCourseMembers();
    });
    connect(remLec, &QPushButton::clicked, this, [this] {
        const auto course = currentId(courses_);
        const auto lec = currentId(lecturers_);
        if (course && lec && guarded(this, [&] { ctx_.admin.removeLecturer(user_, *course, *lec); })) {
            reloadCourseMembers();
        }
    });
    connect(enrol, &QPushButton::clicked, this, [this] {
        const auto course = currentId(courses_);
        if (!course) return;
        const auto st = pickUser(Role::STUDENT, "Choose the student to enrol");
        if (st && guarded(this, [&] { ctx_.admin.enrolStudent(user_, *course, *st); })) reloadCourseMembers();
    });
    connect(unenrol, &QPushButton::clicked, this, [this] {
        const auto course = currentId(courses_);
        const auto st = currentId(students_);
        if (course && st && guarded(this, [&] { ctx_.admin.unenrolStudent(user_, *course, *st); })) {
            reloadCourseMembers();
        }
    });

    reloadUsers();
    reloadOrganisation();
}

const UserRecord* AdminView::userById(int id) const {
    for (const auto& u : userList_) {
        if (u.id == id) return &u;
    }
    return nullptr;
}

void AdminView::reloadUsers() {
    guarded(this, [&] {
        users_->setRowCount(0);
        userList_ = ctx_.admin.listUsers(user_);
        for (const auto& u : userList_) {
            addRow(users_, u.id,
                   {q(u.username), q(u.fullName), roleText(u.role), u.active ? "Yes" : "No",
                    u.mustChangePassword ? "Yes" : "No", u.lockedUntil ? localTime(*u.lockedUntil) : ""});
        }
    });
}

void AdminView::reloadOrganisation() {
    guarded(this, [&] {
        departments_->setRowCount(0);
        courses_->setRowCount(0);
        std::map<int, QString> deptNames;
        for (const auto& d : ctx_.admin.listDepartments(user_)) {
            deptNames[d.id] = q(d.name);
            QString hod = "-";
            if (d.hodId) {
                if (const auto* u = userById(*d.hodId)) hod = q(u->fullName);
            }
            addRow(departments_, d.id, {q(d.name), hod});
        }
        for (const auto& c : ctx_.admin.listCourses(user_)) {
            addRow(courses_, c.id, {q(c.code), q(c.title), deptNames[c.departmentId]});
        }
    });
    reloadCourseMembers();
}

void AdminView::reloadCourseMembers() {
    lecturers_->setRowCount(0);
    students_->setRowCount(0);
    const auto course = currentId(courses_);
    if (!course) return;
    guarded(this, [&] {
        for (const auto& u : ctx_.admin.lecturersOf(user_, *course)) addRow(lecturers_, u.id, {q(u.fullName)});
        for (const auto& u : ctx_.admin.studentsIn(user_, *course)) {
            addRow(students_, u.id, {QString("%1 (%2)").arg(q(u.fullName), q(u.username))});
        }
    });
}

std::optional<int> AdminView::pickUser(Role role, const QString& title) {
    QStringList names;
    std::vector<int> ids;
    guarded(this, [&] {
        for (const auto& u : ctx_.admin.listUsers(user_)) {
            if (u.role != role || !u.active) continue;
            names << QString("%1 (%2)").arg(q(u.fullName), q(u.username));
            ids.push_back(u.id);
        }
    });
    if (names.isEmpty()) {
        QMessageBox::information(this, title, "There are no active " + roleText(role).toLower() +
                                                  " accounts. Create one on the Users tab first.");
        return std::nullopt;
    }
    bool ok = false;
    const QString choice = QInputDialog::getItem(this, title, title + ":", names, 0, false, &ok);
    if (!ok) return std::nullopt;
    return ids[static_cast<std::size_t>(names.indexOf(choice))];
}
