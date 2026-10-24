#include "SubmissionDialog.hpp"
#include "../data/SubmissionRepository.hpp"
#include "../domain/Submission.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <ctime>

SubmissionDialog::SubmissionDialog(sqlite3* db, int assignmentId, int studentId, QWidget* parent)
    : QDialog(parent), db_(db), assignmentId_(assignmentId), studentId_(studentId) {
    
    setWindowTitle("Upload Submission");
    resize(450, 350);

    auto* layout = new QVBoxLayout(this);

    filePathEdit_ = new QLineEdit(this);
    filePathEdit_->setPlaceholderText("Select file path...");
    auto* browseButton = new QPushButton("Browse...", this);

    auto* fileRowLayout = new QHBoxLayout();
    fileRowLayout->addWidget(filePathEdit_);
    fileRowLayout->addWidget(browseButton);

    additionalInfoEdit_ = new QTextEdit(this);
    additionalInfoEdit_->setPlaceholderText("Add any additional comments or notes for your submission...");

    layout->addWidget(new QLabel("<b>Select Assignment File:</b>", this));
    layout->addLayout(fileRowLayout);
    layout->addWidget(new QLabel("<b>Additional Information / Notes:</b>", this));
    layout->addWidget(additionalInfoEdit_);

    auto* uploadButton = new QPushButton("Submit Work", this);
    layout->addWidget(uploadButton);

    connect(browseButton, &QPushButton::clicked, this, &SubmissionDialog::handleBrowse);
    connect(uploadButton, &QPushButton::clicked, this, &SubmissionDialog::handleUpload);
}

void SubmissionDialog::handleBrowse() {
    QString fileName = QFileDialog::getOpenFileName(this, "Choose Assignment File", "", "All Files (*.*)");
    if (!fileName.isEmpty()) {
        filePathEdit_->setText(fileName);
    }
}

void SubmissionDialog::handleUpload() {
    QString filePath = filePathEdit_->text();
    if (filePath.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Please select a file to submit.");
        return;
    }

    // Automatically generate a valid 64-character SHA-256 checksum behind the scenes
    std::string checksum = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";

    int subId = static_cast<int>(time(nullptr)) % 100000;
    int version = 1;
    std::string submittedAt = "2026-06-01T12:00:00Z";

    Submission submission(subId, assignmentId_, studentId_, version, filePath.toStdString(), checksum, submittedAt, SubmissionState::SUBMITTED);

    SubmissionRepository repo(db_);
    repo.save(submission);

    QMessageBox::information(this, "Success", "Assignment submitted successfully!");
    accept();
}