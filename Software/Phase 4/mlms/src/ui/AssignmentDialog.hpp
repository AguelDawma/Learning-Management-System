#pragma once

#include <QDialog>
#include <sqlite3.h>

class QLineEdit;
class QTextEdit;
class QSpinBox;

class AssignmentDialog : public QDialog {
    Q_OBJECT
    sqlite3* db_;
    int lecturerId_;

    QLineEdit* courseIdEdit_;
    QLineEdit* titleEdit_;
    QTextEdit* descriptionEdit_;
    QLineEdit* deadlineEdit_;
    QSpinBox* maxMarkSpin_;

private slots:
    void handleSave();

public:
    explicit AssignmentDialog(sqlite3* db, int lecturerId, QWidget* parent = nullptr);
};