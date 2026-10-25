#pragma once

#include <QDialog>
#include <sqlite3.h>

class QLineEdit;
class QTextEdit;

class SubmissionDialog : public QDialog {
    Q_OBJECT
    sqlite3* db_;
    int assignmentId_;
    int studentId_;

    QLineEdit* filePathEdit_;
    QTextEdit* additionalInfoEdit_;

private slots:
    void handleBrowse();
    void handleUpload();

public:
    explicit SubmissionDialog(sqlite3* db, int assignmentId, int studentId, QWidget* parent = nullptr);
};