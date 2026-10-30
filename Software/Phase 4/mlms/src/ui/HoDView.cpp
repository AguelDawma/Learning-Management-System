#include "RoleViews.hpp"
#include "UiHelpers.hpp"

#include <QComboBox>
#include <QDesktopServices>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTextEdit>
#include <QUrl>
#include <QVBoxLayout>

using namespace ui;

HoDView::HoDView(AppContext& ctx, const User& user, QWidget* parent) : QTabWidget(parent), ctx_(ctx), user_(user) {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->addWidget(new QLabel("Check every mark in your department. Approve the marks you accept, return "
                                 "others to the lecturer with a comment, then release approved marks to students."));
    auto* top = new QHBoxLayout;
    top->addWidget(new QLabel("Assignment:"));
    assignment_ = new QComboBox;
    assignment_->setAccessibleName("Assignment to moderate");
    top->addWidget(assignment_, 1);
    layout->addLayout(top);

    sheet_ = makeTable({"Student", "Version", "Submitted", "Days late", "Status", "Raw mark", "Penalty", "Final mark",
                        "Released"},
                       page, true);
    sheet_->setAccessibleName("Moderation sheet");
    layout->addWidget(sheet_, 3);
    layout->addWidget(new QLabel("<b>Lecturer's feedback for the selected student</b>"));
    feedback_ = new QTextEdit;
    feedback_->setReadOnly(true);
    feedback_->setMaximumHeight(110);
    layout->addWidget(feedback_, 1);

    auto* buttons = new QHBoxLayout;
    auto* approveBtn = addButton(buttons, "&Approve selected", "Marked -> Moderated (you may select several rows)");
    auto* returnBtn = addButton(buttons, "Re&turn to lecturer...", "Send the mark back with a comment");
    auto* adjustBtn = addButton(buttons, "A&djust mark...", "Change a moderated or released mark with a reason");
    auto* releaseBtn = addButton(buttons, "&Release selected", "Moderated -> Released: students can see the mark");
    auto* openBtn = addButton(buttons, "&Open file");
    auto* refreshBtn = addButton(buttons, "Re&fresh");
    buttons->addStretch();
    layout->addLayout(buttons);
    addTab(page, "Moderation");
    addTab(makeAuditLogTab(ctx_, user_, this), "Audit log");

    connect(assignment_, &QComboBox::currentIndexChanged, this, [this] { reloadSheet(); });
    connect(sheet_, &QTableWidget::itemSelectionChanged, this, [this] {
        const auto id = currentId(sheet_);
        const auto* r = id ? rowFor(*id) : nullptr;
        feedback_->setPlainText(r && r->mark ? q(r->mark->feedback()) : "");
    });
    connect(approveBtn, &QPushButton::clicked, this, [this] { approve(); });
    connect(returnBtn, &QPushButton::clicked, this, [this] { returnToLecturer(); });
    connect(adjustBtn, &QPushButton::clicked, this, [this] { adjust(); });
    connect(releaseBtn, &QPushButton::clicked, this, [this] { release(); });
    connect(openBtn, &QPushButton::clicked, this, [this] { openFile(); });
    connect(refreshBtn, &QPushButton::clicked, this, [this] { reloadAssignments(); });
    reloadAssignments();
}

const MarkingRow* HoDView::rowFor(int submissionId) const {
    for (const auto& r : rows_) {
        if (r.submission.id() == submissionId) return &r;
    }
    return nullptr;
}

void HoDView::reloadAssignments() {
    guarded(this, [&] {
        const QVariant keep = assignment_->currentData();
        assignment_->blockSignals(true);
        assignment_->clear();
        deptAssignments_ = ctx_.moderation.departmentAssignments(user_);
        for (const auto& a : deptAssignments_) {
            assignment_->addItem(QString("%1 - %2 (%3 submissions)")
                                     .arg(q(a.course.code), q(a.assignment.title()))
                                     .arg(a.submissionCount),
                                 a.assignment.id());
        }
        const int idx = assignment_->findData(keep);
        assignment_->setCurrentIndex(idx >= 0 ? idx : 0);
        assignment_->blockSignals(false);
    });
    reloadSheet();
}

void HoDView::reloadSheet() {
    sheet_->setRowCount(0);
    rows_.clear();
    feedback_->clear();
    if (assignment_->currentIndex() < 0) return;
    guarded(this, [&] {
        rows_ = ctx_.moderation.moderationSheet(user_, assignment_->currentData().toInt());
        for (const auto& r : rows_) {
            addRow(sheet_, r.submission.id(),
                   {q(r.studentName), QString::number(r.submission.version()), localTime(r.submission.submittedAt()),
                    QString::number(r.daysLate), stateLabel(r.submission.state()),
                    r.mark ? markText(r.mark->rawMark()) : "", r.result ? markText(r.result->penalty) : "",
                    r.result ? markText(r.result->finalMark) : "",
                    r.mark && r.mark->releasedAt() ? localTime(*r.mark->releasedAt()) : ""});
        }
    });
}

void HoDView::approve() {
    const auto ids = selectedIds(sheet_);
    if (ids.empty()) return;
    if (guarded(this, [&] {
            const int n = ctx_.moderation.approve(user_, ids);
            QMessageBox::information(this, "Approved", QString("%1 mark(s) approved.").arg(n));
        })) {
        reloadSheet();
    }
}

void HoDView::returnToLecturer() {
    const auto id = currentId(sheet_);
    if (!id) return;
    bool ok = false;
    const QString comment = QInputDialog::getMultiLineText(this, "Return to lecturer",
                                                           "Explain what the lecturer should re-check:", {}, &ok);
    if (!ok) return;
    if (guarded(this, [&] { ctx_.moderation.returnForRemarking(user_, *id, s(comment)); })) reloadSheet();
}

void HoDView::adjust() {
    const auto id = currentId(sheet_);
    const auto* r = id ? rowFor(*id) : nullptr;
    if (!r || !r->mark) return;
    int maxMark = 0;
    for (const auto& a : deptAssignments_) {
        if (a.assignment.id() == r->submission.assignmentId()) maxMark = a.assignment.maxMark();
    }
    bool ok = false;
    const double raw = QInputDialog::getDouble(this, "Adjust mark", QString("New raw mark (0 - %1):").arg(maxMark),
                                               r->mark->rawMark(), 0, maxMark, 1, &ok);
    if (!ok) return;
    const QString reason = QInputDialog::getText(this, "Adjust mark", "Reason for the change (required):",
                                                 QLineEdit::Normal, {}, &ok);
    if (!ok) return;
    const int version = r->mark->rowVersion();
    if (guarded(this, [&] { ctx_.moderation.adjustMark(user_, *id, raw, s(reason), version); })) reloadSheet();
}

void HoDView::release() {
    const auto ids = selectedIds(sheet_);
    if (ids.empty()) return;
    if (QMessageBox::question(this, "Release marks",
                              QString("Release %1 mark(s) to the students? They will be able to see the mark and "
                                      "feedback.")
                                  .arg(ids.size())) != QMessageBox::Yes) {
        return;
    }
    if (guarded(this, [&] {
            const int n = ctx_.moderation.release(user_, ids);
            QMessageBox::information(this, "Released", QString("%1 mark(s) released.").arg(n));
        })) {
        reloadSheet();
    }
}

void HoDView::openFile() {
    const auto id = currentId(sheet_);
    const auto* r = id ? rowFor(*id) : nullptr;
    if (!r) return;
    const auto path = ctx_.submissionService.pathOf(r->submission);
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(fromPath(path)))) {
        QMessageBox::warning(this, "Cannot open file", "The file could not be opened:\n" + fromPath(path));
    }
}
