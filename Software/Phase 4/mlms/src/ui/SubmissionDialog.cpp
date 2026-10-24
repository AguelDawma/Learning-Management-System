#include "SubmissionDialog.hpp"
#include "../data/SubmissionRepository.hpp"
#include "../domain/Submission.hpp"
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <ctime>

SubmissionDialog::SubmissionDialog(sqlite3* db, int assignmentId, int studentId, QWidget* parent)
    : QDialog(parent), db_(db), assignmentId_(assignmentId), studentId_(studentId) {
    
    setWindowTitle("Upload Submission");
    resize(400, 250);

    auto* layout = new QVBoxLayout(this);

    filePathEdit_ = new QLineEdit(this);
    filePathEdit_->setPlaceholderText("File Path (e.g. solution.cpp)");

    checksumEdit_ = new QLineEdit(this);
    checksumEdit_->setPlaceholderText("64-character SHA-256 checksum");
    checksumEdit_->setMaxLength(64);
    checksumEdit_->setText("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");

    layout->addWidget(new QLabel("<b>File Path:</b>", this));
    layout->addWidget(filePathEdit_);
    layout->addWidget(new QLabel("<b>SHA-256 Checksum:</b>", this));
    layout->addWidget(checksumEdit_);

    auto* uploadButton = new QPushButton("Submit Work", this);
    layout->addWidget(uploadButton);

    connect(uploadButton, &QPushButton::clicked, this, &SubmissionDialog::handleUpload);
}

void SubmissionDialog::handleUpload() {
    std::string filePath = filePathEdit_->text().toStdString();
    if (filePath.empty()) {
        QMessageBox::warning(this, "Validation Error", "File path cannot be empty.");
        return;
    }

    std::string checksum = checksumEdit_->text().toStdString();
    if (checksum.length() != 64) {
        QMessageBox::warning(this, "Validation Error", "Checksum must be exactly 64 hex characters.");
        return;
    }

    int subId = static_cast<int>(time(nullptr)) % 100000;
    int version = 1;
    std::string submittedAt = "2026-06-01T12:00:00Z";

    Submission submission(subId, assignmentId_, studentId_, version, filePath, checksum, submittedAt, SubmissionState::SUBMITTED);

    SubmissionRepository repo(db_);
    repo.save(submission);

    QMessageBox::information(this, "Success", "Submission uploaded successfully!");
    accept();
}