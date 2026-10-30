#include "RoleViews.hpp"
#include "UiHelpers.hpp"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>

using namespace ui;

StudentView::StudentView(AppContext& ctx, const User& user, QWidget* parent)
    : QTabWidget(parent), ctx_(ctx), user_(user) {
    // ---- My assignments
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    courses_ = new QLabel;
    courses_->setWordWrap(true);
    courses_->setAccessibleName("My courses");
    layout->addWidget(courses_);
    layout->addWidget(new QLabel("You may resubmit as often as you like before the deadline; the newest version "
                                 "is the one that is marked."));
    auto* split = new QSplitter(Qt::Vertical);
    assignments_ = makeTable({"Course", "Assignment", "Deadline", "Max mark", "Status", "Version", "Submitted"}, page);
    assignments_->setAccessibleName("My assignments");
    details_ = new QTextEdit;
    details_->setReadOnly(true);
    details_->setAccessibleName("Assignment details");
    split->addWidget(assignments_);
    split->addWidget(details_);
    split->setStretchFactor(0, 3);
    split->setStretchFactor(1, 1);
    layout->addWidget(split);
    auto* buttons = new QHBoxLayout;
    auto* submitBtn = addButton(buttons, "&Submit or resubmit work...", "Upload a file for the selected assignment");
    auto* versionsBtn = addButton(buttons, "My &versions...", "List every version you have submitted");
    auto* refreshBtn = addButton(buttons, "&Refresh");
    buttons->addStretch();
    layout->addLayout(buttons);
    addTab(page, "My assignments");

    // ---- My results
    auto* resultsPage = new QWidget;
    auto* rl = new QVBoxLayout(resultsPage);
    rl->addWidget(new QLabel("Marks appear here once your Head of Department has checked and released them."));
    results_ = makeTable({"Course", "Assignment", "Raw mark", "Late penalty", "Final mark", "Days late", "Released"},
                         resultsPage);
    results_->setAccessibleName("My released results");
    feedback_ = new QTextEdit;
    feedback_->setReadOnly(true);
    feedback_->setAccessibleName("Feedback from the lecturer");
    rl->addWidget(results_, 3);
    rl->addWidget(new QLabel("<b>Feedback</b>"));
    rl->addWidget(feedback_, 1);
    addTab(resultsPage, "My results");

    connect(assignments_, &QTableWidget::itemSelectionChanged, this, [this] { showDetails(); });
    connect(assignments_, &QTableWidget::cellDoubleClicked, this, [this] { submit(); });
    connect(results_, &QTableWidget::itemSelectionChanged, this, [this] {
        feedback_->clear();
        const auto id = currentId(results_);
        for (const auto& v : rows_) {
            if (id && v.assignment.id() == *id && v.result) feedback_->setPlainText(q(v.result->feedback));
        }
    });
    connect(submitBtn, &QPushButton::clicked, this, [this] { submit(); });
    connect(versionsBtn, &QPushButton::clicked, this, [this] { showVersions(); });
    connect(refreshBtn, &QPushButton::clicked, this, [this] { reload(); });
    reload();
}

void StudentView::reload() {
    guarded(this, [&] {
        QStringList names;
        for (const auto& c : ctx_.submissionService.myCourses(user_)) {
            names << QString("%1 %2").arg(q(c.code), q(c.title)).toHtmlEscaped();
        }
        courses_->setText("<b>My courses:</b> " + (names.isEmpty() ? "you are not enrolled in any course yet"
                                                                   : names.join(" &nbsp;|&nbsp; ")));
        rows_ = ctx_.submissionService.myAssignments(user_);
        assignments_->setRowCount(0);
        results_->setRowCount(0);
        for (const auto& v : rows_) {
            QString deadline = localTime(v.effectiveDeadline);
            if (v.extension) deadline += "  (extended)";
            const QString status = v.active ? stateLabel(v.active->state()) : "Not submitted";
            addRow(assignments_, v.assignment.id(),
                   {q(v.course.code), q(v.assignment.title()), deadline, QString::number(v.assignment.maxMark()),
                    status, v.active ? QString::number(v.active->version()) : "",
                    v.active ? localTime(v.active->submittedAt()) : ""});
            if (v.result) {
                addRow(results_, v.assignment.id(),
                       {q(v.course.code), q(v.assignment.title()),
                        QString("%1 / %2").arg(markText(v.result->rawMark)).arg(v.assignment.maxMark()),
                        markText(v.result->penalty),
                        QString("%1 / %2").arg(markText(v.result->finalMark)).arg(v.assignment.maxMark()),
                        QString::number(v.result->daysLate), localTime(v.result->releasedAt)});
            }
        }
        details_->clear();
    });
}

void StudentView::showDetails() {
    const auto id = currentId(assignments_);
    for (const auto& v : rows_) {
        if (!id || v.assignment.id() != *id) continue;
        QString html = QString("<h3>%1</h3><p>%2</p><p><b>Deadline:</b> %3</p>")
                           .arg(q(v.assignment.title()).toHtmlEscaped(),
                                q(v.assignment.description()).toHtmlEscaped().replace("\n", "<br>"),
                                localTime(v.effectiveDeadline));
        if (v.extension) {
            html += QString("<p><b>Extension granted</b> (original deadline %1): %2</p>")
                        .arg(localTime(v.assignment.deadline()), q(v.extension->reason).toHtmlEscaped());
        }
        details_->setHtml(html);
    }
}

void StudentView::submit() {
    const auto id = currentId(assignments_);
    if (!id) {
        QMessageBox::information(this, "Select an assignment", "First select the assignment you want to submit.");
        return;
    }
    guarded(this, [&] {
        const auto plan = ctx_.submissionService.preview(user_, *id);
        if (plan.kind == SubmissionPlan::Kind::Refused) {
            QMessageBox::information(this, "Cannot submit", q(plan.refusal));
            return;
        }
        QString exts;
        for (const auto& e : ctx_.submissionService.allowedExtensions()) exts += " *" + q(e);
        const QString file = QFileDialog::getOpenFileName(this, "Choose the file to submit", {},
                                                          "Accepted files (" + exts.trimmed() + ")");
        if (file.isEmpty()) return;

        QString question;
        switch (plan.kind) {
            case SubmissionPlan::Kind::FirstOnTime:
                question = "Submit this file now?";
                break;
            case SubmissionPlan::Kind::Resubmission:
                question = QString("This will be version %1 and will replace your earlier version as the one "
                                   "that is marked. Your earlier versions are kept. Continue?")
                               .arg(plan.nextVersion);
                break;
            case SubmissionPlan::Kind::Late:
                question = QString("The deadline (%1) has passed. This submission is %2 day(s) late, so 10% of the "
                                   "maximum mark will be deducted per day, and work more than 3 days late "
                                   "receives zero. Submit anyway?")
                               .arg(localTime(plan.effectiveDeadline))
                               .arg(plan.daysLate);
                break;
            case SubmissionPlan::Kind::Refused:
                break;
        }
        if (QMessageBox::question(this, "Confirm submission", question) != QMessageBox::Yes) return;

        const auto r = ctx_.submissionService.submit(user_, *id, toPath(file));
        QMessageBox::information(
            this, "Submission receipt",
            QString("Your work was received.\n\nReceipt number: %1\nVersion: %2\nTime: %3\nStatus: %4\n"
                    "Checksum (SHA-256): %5")
                .arg(r.submissionId)
                .arg(r.version)
                .arg(localTime(r.submittedAt), stateLabel(r.state), q(r.checksum)));
        reload();
    });
}

void StudentView::showVersions() {
    const auto id = currentId(assignments_);
    if (!id) return;
    guarded(this, [&] {
        QString text;
        for (const auto& s : ctx_.submissionService.myVersions(user_, *id)) {
            text += QString("Version %1 - %2 - %3 - checksum %4\n")
                        .arg(s.version())
                        .arg(localTime(s.submittedAt()), stateLabel(s.state()), q(s.checksum().substr(0, 12)));
        }
        QMessageBox::information(this, "My versions", text.isEmpty() ? "You have not submitted anything yet." : text);
    });
}
