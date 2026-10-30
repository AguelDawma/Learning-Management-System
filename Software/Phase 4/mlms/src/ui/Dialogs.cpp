#include "Dialogs.hpp"
#include "UiHelpers.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>

using namespace ui;

namespace {

QDialogButtonBox* okCancel(QDialog* dlg, const QString& okText) {
    auto* box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    box->button(QDialogButtonBox::Ok)->setText(okText);
    QObject::connect(box, &QDialogButtonBox::rejected, dlg, &QDialog::reject);
    return box;
}

QLabel* hint(const QString& text) {
    auto* l = new QLabel(text);
    l->setWordWrap(true);
    l->setStyleSheet("color: #555;");
    return l;
}

QDateTimeEdit* deadlineEdit(const QDateTime& initial) {
    auto* e = new QDateTimeEdit(initial);
    e->setCalendarPopup(true);
    e->setDisplayFormat("ddd d MMM yyyy  HH:mm");
    return e;
}

} // namespace

// ---------------------------------------------------------------- password

ChangePasswordDialog::ChangePasswordDialog(AuthService& auth, const User& user, bool forced, QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(forced ? "Choose a new password" : "Change password");
    auto* layout = new QVBoxLayout(this);
    if (forced) {
        layout->addWidget(hint("Your password was set by the administrator. Please choose your own password "
                               "before continuing."));
    }
    auto* form = new QFormLayout;
    auto* current = new QLineEdit;
    auto* next = new QLineEdit;
    auto* repeat = new QLineEdit;
    for (auto* e : {current, next, repeat}) e->setEchoMode(QLineEdit::Password);
    form->addRow("Current password:", current);
    form->addRow("New password:", next);
    form->addRow("Repeat new password:", repeat);
    layout->addLayout(form);
    layout->addWidget(hint("At least 8 characters, with at least one letter and one digit."));
    auto* box = okCancel(this, "Change password");
    layout->addWidget(box);

    connect(box, &QDialogButtonBox::accepted, this, [=, &auth, &user] {
        if (next->text() != repeat->text()) {
            QMessageBox::warning(this, "Passwords differ", "The two new passwords are not the same.");
            return;
        }
        if (guarded(this, [&] { auth.changePassword(user, s(current->text()), s(next->text())); })) {
            QMessageBox::information(this, "Password changed", "Your password has been changed.");
            accept();
        }
    });
}

// ---------------------------------------------------------------- assignment

AssignmentDialog::AssignmentDialog(AppContext& ctx, const User& lecturer, QWidget* parent) : QDialog(parent) {
    setWindowTitle("New assignment");
    setMinimumWidth(460);
    auto* layout = new QVBoxLayout(this);
    auto* form = new QFormLayout;
    auto* course = new QComboBox;
    guarded(this, [&] {
        for (const auto& c : ctx.assignmentService.myCourses(lecturer)) {
            course->addItem(QString("%1 - %2").arg(q(c.code), q(c.title)), c.id);
        }
    });
    auto* title = new QLineEdit;
    auto* description = new QTextEdit;
    description->setAcceptRichText(false);
    description->setMaximumHeight(110);
    auto* deadline = deadlineEdit(QDateTime::currentDateTime().addDays(7));
    auto* maxMark = new QSpinBox;
    maxMark->setRange(Assignment::kMinMaxMark, Assignment::kMaxMaxMark);
    maxMark->setValue(100);
    auto* publish = new QCheckBox("Publish now (students can see it and submit)");
    publish->setChecked(true);

    form->addRow("Course:", course);
    form->addRow("Title:", title);
    form->addRow("Instructions:", description);
    form->addRow("Deadline:", deadline);
    form->addRow("Maximum mark:", maxMark);
    form->addRow("", publish);
    layout->addLayout(form);
    if (course->count() == 0) layout->addWidget(hint("You are not assigned to any course yet. Ask the administrator."));
    auto* box = okCancel(this, "Create assignment");
    layout->addWidget(box);

    connect(box, &QDialogButtonBox::accepted, this, [=, &ctx, &lecturer] {
        if (course->currentIndex() < 0) return;
        if (guarded(this, [&] {
                ctx.assignmentService.create(lecturer, course->currentData().toInt(), s(title->text()),
                                             s(description->toPlainText()), fromQDateTime(deadline->dateTime()),
                                             maxMark->value(), publish->isChecked());
            })) {
            accept();
        }
    });
}

// ---------------------------------------------------------------- extensions

ExtensionDialog::ExtensionDialog(AppContext& ctx, const User& lecturer, const Assignment& assignment, QWidget* parent)
    : QDialog(parent) {
    setWindowTitle("Extensions - " + q(assignment.title()));
    resize(620, 460);
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(hint(QString("Original deadline: <b>%1</b>. An extension replaces the deadline for one "
                                   "student only; late penalties then count from the new deadline.")
                               .arg(localTime(assignment.deadline()))));
    auto* table = makeTable({"Student", "Extended to", "Reason"}, this);
    table->setAccessibleName("Students and their extensions");
    layout->addWidget(table);

    auto* form = new QFormLayout;
    auto* newDeadline = deadlineEdit(toQDateTime(assignment.deadline()).addDays(2));
    auto* reason = new QLineEdit;
    reason->setPlaceholderText("e.g. medical certificate received");
    form->addRow("New deadline:", newDeadline);
    form->addRow("Reason (required):", reason);
    layout->addLayout(form);
    auto* box = okCancel(this, "Grant extension to selected student");
    box->button(QDialogButtonBox::Cancel)->setText("Close");
    layout->addWidget(box);

    auto load = [=, &ctx, &lecturer, &assignment] {
        guarded(this, [&] {
            table->setRowCount(0);
            for (const auto& r : ctx.assignmentService.extensionsFor(lecturer, assignment.id())) {
                addRow(table, r.studentId,
                       {q(r.studentName), r.extension ? localTime(r.extension->newDeadline) : "-",
                        r.extension ? q(r.extension->reason) : ""});
            }
        });
    };
    load();

    connect(box, &QDialogButtonBox::accepted, this, [=, &ctx, &lecturer, &assignment] {
        const auto student = currentId(table);
        if (!student) {
            QMessageBox::information(this, "Select a student", "Select the student who should get the extension.");
            return;
        }
        if (guarded(this, [&] {
                ctx.assignmentService.grantExtension(lecturer, assignment.id(), *student,
                                                     fromQDateTime(newDeadline->dateTime()), s(reason->text()));
            })) {
            reason->clear();
            load();
            QMessageBox::information(this, "Extension granted", "The extension has been saved.");
        }
    });
}

// ---------------------------------------------------------------- marking

MarkDialog::MarkDialog(AppContext& ctx, const User& lecturer, const Assignment& assignment, const MarkingRow& row,
                       QWidget* parent)
    : QDialog(parent) {
    const bool first = !row.mark.has_value();
    setWindowTitle(first ? "Enter mark" : "Edit mark");
    setMinimumWidth(520);
    auto* layout = new QVBoxLayout(this);

    QString info = QString("<b>%1</b> - %2, version %3<br>Submitted %4")
                       .arg(q(row.studentName), q(assignment.title()))
                       .arg(row.submission.version())
                       .arg(localTime(row.submission.submittedAt()));
    if (row.daysLate > 0) info += QString("<br><b>%1 day(s) late</b> - the late penalty is applied automatically.").arg(row.daysLate);
    layout->addWidget(new QLabel(info));
    if (row.mark && row.mark->returnComment()) {
        auto* c = new QLabel("<b>Returned by the HoD:</b> " + q(*row.mark->returnComment()).toHtmlEscaped());
        c->setWordWrap(true);
        c->setStyleSheet("background: #fff3cd; padding: 6px;");
        layout->addWidget(c);
    }

    auto* form = new QFormLayout;
    auto* raw = new QDoubleSpinBox;
    raw->setRange(0, assignment.maxMark());
    raw->setDecimals(1);
    raw->setSingleStep(0.5);
    raw->setSuffix(QString(" / %1").arg(assignment.maxMark()));
    raw->setValue(row.mark ? row.mark->rawMark() : 0);
    auto* penalty = new QLabel;
    auto* feedback = new QTextEdit;
    feedback->setAcceptRichText(false);
    feedback->setPlainText(row.mark ? q(row.mark->feedback()) : "");
    auto* reason = new QLineEdit;
    reason->setPlaceholderText(first ? "optional for the first mark" : "required - why is the mark changing?");
    form->addRow("Raw mark:", raw);
    form->addRow("After late penalty:", penalty);
    form->addRow("Written feedback:", feedback);
    form->addRow(first ? "Note:" : "Reason for change:", reason);
    layout->addLayout(form);
    layout->addWidget(hint("Feedback must be at least 10 characters. The student will only see the mark after "
                           "the Head of Department has checked and released it."));
    auto* box = okCancel(this, "Save mark");
    layout->addWidget(box);

    auto updatePenalty = [=, &ctx, &assignment] {
        const auto r = ctx.marking.previewPenalty(raw->value(), assignment.maxMark(), row.daysLate);
        penalty->setText(QString("%1  (penalty %2)").arg(markText(r.finalMark), markText(r.penalty)));
    };
    connect(raw, &QDoubleSpinBox::valueChanged, this, updatePenalty);
    updatePenalty();

    const std::optional<int> loadedVersion = row.mark ? std::optional<int>(row.mark->rowVersion()) : std::nullopt;
    const int submissionId = row.submission.id();
    connect(box, &QDialogButtonBox::accepted, this, [=, &ctx, &lecturer] {
        if (guarded(this, [&] {
                ctx.marking.saveMark(lecturer, submissionId, raw->value(), s(feedback->toPlainText()),
                                     s(reason->text()), loadedVersion);
            })) {
            accept();
        }
    });
}

// ---------------------------------------------------------------- users and courses

UserDialog::UserDialog(AppContext& ctx, const User& admin, QWidget* parent) : QDialog(parent) {
    setWindowTitle("New user account");
    auto* layout = new QVBoxLayout(this);
    auto* form = new QFormLayout;
    auto* username = new QLineEdit;
    username->setPlaceholderText("e.g. t.mokoena");
    auto* fullName = new QLineEdit;
    auto* role = new QComboBox;
    role->addItem("Student", static_cast<int>(Role::STUDENT));
    role->addItem("Lecturer", static_cast<int>(Role::LECTURER));
    role->addItem("Head of Department", static_cast<int>(Role::HOD));
    role->addItem("Administrator", static_cast<int>(Role::ADMINISTRATOR));
    auto* password = new QLineEdit;
    form->addRow("Username:", username);
    form->addRow("Full name:", fullName);
    form->addRow("Role:", role);
    form->addRow("Temporary password:", password);
    layout->addLayout(form);
    layout->addWidget(hint("Usernames use lower-case letters, digits, '.', '_' or '-'. The user must choose "
                           "a new password at first sign-in."));
    auto* box = okCancel(this, "Create account");
    layout->addWidget(box);

    connect(box, &QDialogButtonBox::accepted, this, [=, &ctx, &admin] {
        if (guarded(this, [&] {
                ctx.admin.createUser(admin, s(username->text()), s(fullName->text()),
                                     static_cast<Role>(role->currentData().toInt()), s(password->text()));
            })) {
            accept();
        }
    });
}

CourseDialog::CourseDialog(AppContext& ctx, const User& admin, QWidget* parent) : QDialog(parent) {
    setWindowTitle("New course");
    auto* layout = new QVBoxLayout(this);
    auto* form = new QFormLayout;
    auto* code = new QLineEdit;
    code->setPlaceholderText("e.g. CS101");
    auto* title = new QLineEdit;
    auto* dept = new QComboBox;
    guarded(this, [&] {
        for (const auto& d : ctx.admin.listDepartments(admin)) dept->addItem(q(d.name), d.id);
    });
    form->addRow("Code:", code);
    form->addRow("Title:", title);
    form->addRow("Department:", dept);
    layout->addLayout(form);
    if (dept->count() == 0) layout->addWidget(hint("Create a department first."));
    auto* box = okCancel(this, "Create course");
    layout->addWidget(box);

    connect(box, &QDialogButtonBox::accepted, this, [=, &ctx, &admin] {
        if (dept->currentIndex() < 0) return;
        if (guarded(this, [&] {
                ctx.admin.createCourse(admin, s(code->text()), s(title->text()), dept->currentData().toInt());
            })) {
            accept();
        }
    });
}
