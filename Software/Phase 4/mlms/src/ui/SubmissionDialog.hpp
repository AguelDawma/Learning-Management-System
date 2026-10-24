#pragma once

#include <QDialog>
#include <sqlite3.h>

class QLineEdit;

class SubmissionDialog : public QDialog {
    Q_OBJECT
    sqlite3* db_;
    int assignmentId_;
    int studentId_;

    QLineEdit* filePathEdit_;
    QLineEdit* checksumEdit_;

private slots:
    void handleUpload();

public:
    explicit SubmissionDialog(sqlite3* db, int assignmentId, int studentId, QWidget* parent = nullptr);
};