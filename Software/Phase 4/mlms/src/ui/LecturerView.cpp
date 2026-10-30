#include "Dialogs.hpp"
#include "RoleViews.hpp"
#include "UiHelpers.hpp"

#include <QComboBox>
#include <QDesktopServices>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QUrl>
#include <QVBoxLayout>

using namespace ui;

LecturerView::LecturerView(AppContext& ctx, const User& user, QWidget* parent)
    : QTabWidget(parent), ctx_(ctx), user_(user) {
    // ---- Assignments
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    assignments_ = makeTable({"Course", "Title", "Deadline", "Max mark", "Published", "Submissions"}, page);
    assignments_->setAccessibleName("My assignments");
    layout->addWidget(assignments_);
    auto* buttons = new QHBoxLayout;
    auto* createBtn = addButton(buttons, "&New assignment...");
    auto* publishBtn = addButton(buttons, "&Publish / hide", "Show or hide the selected assignment for students");
    auto* extBtn = addButton(buttons, "&Extensions...", "Give one student more time for the selected assignment");
    auto* refreshBtn = addButton(buttons, "&Refresh");
    buttons->addStretch();
    layout->addLayout(buttons);
    addTab(page, "Assignments");

    // ---- Marking
    auto* markPage = new QWidget;
    auto* ml = new QVBoxLayout(markPage);
    auto* top = new QHBoxLayout;
    top->addWidget(new QLabel("Assignment:"));
    markingAssignment_ = new QComboBox;
    markingAssignment_->setAccessibleName("Assignment to mark");
    top->addWidget(markingAssignment_, 1);
    ml->addLayout(top);
    marking_ = makeTable({"Student", "Version", "Submitted", "Deadline", "Days late", "Status", "Raw mark", "Penalty",
                          "Final mark", "HoD comment"},
                         markPage);
    marking_->setAccessibleName("Submissions to mark");
    ml->addWidget(marking_);
    markingInfo_ = new QLabel("Marks you save go to the Head of Department for moderation before students see them.");
    markingInfo_->setWordWrap(true);
    ml->addWidget(markingInfo_);
    auto* mb = new QHBoxLayout;
    auto* openBtn = addButton(mb, "&Open submitted file", "Open the student's file with the default program");
    auto* markBtn = addButton(mb, "Enter / edit &mark...");
    auto* refreshMarkBtn = addButton(mb, "Re&fresh");
    mb->addStretch();
    ml->addLayout(mb);
    addTab(markPage, "Marking");

    connect(createBtn, &QPushButton::clicked, this, [this] { createAssignment(); });
    connect(publishBtn, &QPushButton::clicked, this, [this] { togglePublished(); });
    connect(extBtn, &QPushButton::clicked, this, [this] { manageExtensions(); });
    connect(refreshBtn, &QPushButton::clicked, this, [this] { reloadAssignments(); });
    connect(markingAssignment_, &QComboBox::currentIndexChanged, this, [this] { reloadMarking(); });
    connect(openBtn, &QPushButton::clicked, this, [this] { openFile(); });
    connect(markBtn, &QPushButton::clicked, this, [this] { markSelected(); });
    connect(marking_, &QTableWidget::cellDoubleClicked, this, [this] { markSelected(); });
    connect(refreshMarkBtn, &QPushButton::clicked, this, [this] { reloadMarking(); });
    reloadAssignments();
}

const Assignment* LecturerView::assignmentById(int id) const {
    for (const auto& a : myAssignments_) {
        if (a.assignment.id() == id) return &a.assignment;
    }
    return nullptr;
}

void LecturerView::reloadAssignments() {
    guarded(this, [&] {
        myAssignments_ = ctx_.assignmentService.myAssignments(user_);
        assignments_->setRowCount(0);
        const QVariant keep = markingAssignment_->currentData();
        markingAssignment_->blockSignals(true);
        markingAssignment_->clear();
        for (const auto& a : myAssignments_) {
            addRow(assignments_, a.assignment.id(),
                   {q(a.course.code), q(a.assignment.title()), localTime(a.assignment.deadline()),
                    QString::number(a.assignment.maxMark()), a.assignment.isPublished() ? "Yes" : "No",
                    QString::number(a.submissionCount)});
            markingAssignment_->addItem(QString("%1 - %2").arg(q(a.course.code), q(a.assignment.title())),
                                        a.assignment.id());
        }
        const int idx = markingAssignment_->findData(keep);
        markingAssignment_->setCurrentIndex(idx >= 0 ? idx : 0);
        markingAssignment_->blockSignals(false);
    });
    reloadMarking();
}

void LecturerView::reloadMarking() {
    marking_->setRowCount(0);
    markingRows_.clear();
    if (markingAssignment_->currentIndex() < 0) return;
    guarded(this, [&] {
        markingRows_ = ctx_.marking.markingList(user_, markingAssignment_->currentData().toInt());
        for (const auto& r : markingRows_) {
            addRow(marking_, r.submission.id(),
                   {q(r.studentName), QString::number(r.submission.version()), localTime(r.submission.submittedAt()),
                    localTime(r.effectiveDeadline) + (r.hasExtension ? " (ext.)" : ""), QString::number(r.daysLate),
                    stateLabel(r.submission.state()), r.mark ? markText(r.mark->rawMark()) : "",
                    r.result ? markText(r.result->penalty) : "", r.result ? markText(r.result->finalMark) : "",
                    r.mark && r.mark->returnComment() ? q(*r.mark->returnComment()) : ""});
        }
    });
}

void LecturerView::createAssignment() {
    AssignmentDialog dlg(ctx_, user_, this);
    if (dlg.exec() == QDialog::Accepted) reloadAssignments();
}

void LecturerView::togglePublished() {
    const auto id = currentId(assignments_);
    if (!id) return;
    const auto* a = assignmentById(*id);
    if (a && guarded(this, [&] { ctx_.assignmentService.setPublished(user_, *id, !a->isPublished()); })) {
        reloadAssignments();
    }
}

void LecturerView::manageExtensions() {
    const auto id = currentId(assignments_);
    if (!id) {
        QMessageBox::information(this, "Select an assignment", "First select an assignment in the list.");
        return;
    }
    const auto* a = assignmentById(*id);
    if (!a) return;
    const Assignment assignment = *a;
    ExtensionDialog dlg(ctx_, user_, assignment, this);
    dlg.exec();
    reloadMarking();
}

void LecturerView::openFile() {
    const auto id = currentId(marking_);
    for (const auto& r : markingRows_) {
        if (!id || r.submission.id() != *id) continue;
        const auto path = ctx_.submissionService.pathOf(r.submission);
        if (!QDesktopServices::openUrl(QUrl::fromLocalFile(fromPath(path)))) {
            QMessageBox::warning(this, "Cannot open file", "The file could not be opened:\n" + fromPath(path));
        }
    }
}

void LecturerView::markSelected() {
    const auto id = currentId(marking_);
    if (!id) {
        QMessageBox::information(this, "Select a submission", "First select a student's submission.");
        return;
    }
    for (const auto& r : markingRows_) {
        if (r.submission.id() != *id) continue;
        const auto* a = assignmentById(r.submission.assignmentId());
        if (!a) return;
        const Assignment assignment = *a;
        const MarkingRow row = r; // reloadMarking() below replaces markingRows_
        MarkDialog dlg(ctx_, user_, assignment, row, this);
        if (dlg.exec() == QDialog::Accepted) reloadMarking();
        return;
    }
}
