#include "AssignmentDialog.hpp"
#include "../data/AssignmentRepository.hpp"
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QMessageBox>
#include <ctime>

AssignmentDialog::AssignmentDialog(sqlite3* db, int lecturerId, QWidget* parent)
    : QDialog(parent), db_(db), lecturerId_(lecturerId) {
    
    setWindowTitle("Create New Assignment");
    resize(400, 450);

    auto* layout = new QVBoxLayout(this);

    courseIdEdit_ = new QLineEdit(this);
    courseIdEdit_->setPlaceholderText("Course ID (e.g. 101)");
    
    titleEdit_ = new QLineEdit(this);
    titleEdit_->setPlaceholderText("Assignment Title");

    descriptionEdit_ = new QTextEdit(this);
    descriptionEdit_->setPlaceholderText("Assignment Description...");

    deadlineEdit_ = new QLineEdit(this);
    deadlineEdit_->setPlaceholderText("Deadline (YYYY-MM-DD)");

    maxMarkSpin_ = new QSpinBox(this);
    maxMarkSpin_->setRange(1, 1000);
    maxMarkSpin_->setValue(100);

    layout->addWidget(new QLabel("<b>Course ID:</b>", this));
    layout->addWidget(courseIdEdit_);
    layout->addWidget(new QLabel("<b>Title:</b>", this));
    layout->addWidget(titleEdit_);
    layout->addWidget(new QLabel("<b>Description:</b>", this));
    layout->addWidget(descriptionEdit_);
    layout->addWidget(new QLabel("<b>Deadline (YYYY-MM-DD):</b>", this));
    layout->addWidget(deadlineEdit_);
    layout->addWidget(new QLabel("<b>Maximum Mark:</b>", this));
    layout->addWidget(maxMarkSpin_);

    auto* saveButton = new QPushButton("Save & Publish", this);
    layout->addWidget(saveButton);

    connect(saveButton, &QPushButton::clicked, this, &AssignmentDialog::handleSave);
}

void AssignmentDialog::handleSave() {
    bool ok;
    int courseId = courseIdEdit_->text().toInt(&ok);
    if (!ok || courseId <= 0) {
        QMessageBox::warning(this, "Validation Error", "Please enter a valid numeric Course ID.");
        return;
    }

    std::string title = titleEdit_->text().toStdString();
    if (title.empty()) {
        QMessageBox::warning(this, "Validation Error", "Title cannot be empty.");
        return;
    }

    std::string desc = descriptionEdit_->toPlainText().toStdString();
    std::string deadline = deadlineEdit_->text().toStdString();
    if (deadline.empty()) {
        QMessageBox::warning(this, "Validation Error", "Deadline cannot be empty.");
        return;
    }

    int maxMark = maxMarkSpin_->value();
    int assignmentId = static_cast<int>(time(nullptr)) % 100000;

    Assignment assignment(assignmentId, courseId, title, desc, deadline, maxMark, true, lecturerId_);

    AssignmentRepository repo(db_);
    repo.save(assignment);

    QMessageBox::information(this, "Success", "Assignment created successfully!");
    accept();
}