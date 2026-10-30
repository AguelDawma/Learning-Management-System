#pragma once

#include "services/AppContext.hpp"

#include <QDialog>

class QComboBox;
class QDateTimeEdit;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QTextEdit;

// FR-04: also shown straight after sign-in when a password change is required.
class ChangePasswordDialog : public QDialog {
public:
    ChangePasswordDialog(AuthService& auth, const User& user, bool forced, QWidget* parent = nullptr);
};

class AssignmentDialog : public QDialog {
public:
    AssignmentDialog(AppContext& ctx, const User& lecturer, QWidget* parent = nullptr);
};

class ExtensionDialog : public QDialog {
public:
    ExtensionDialog(AppContext& ctx, const User& lecturer, const Assignment& assignment, QWidget* parent = nullptr);
};

// Lecturer marking. Shows the late penalty that BR-02 will apply as the mark is typed.
class MarkDialog : public QDialog {
public:
    MarkDialog(AppContext& ctx, const User& lecturer, const Assignment& assignment, const MarkingRow& row,
               QWidget* parent = nullptr);
};

class UserDialog : public QDialog {
public:
    UserDialog(AppContext& ctx, const User& admin, QWidget* parent = nullptr);
};

class CourseDialog : public QDialog {
public:
    CourseDialog(AppContext& ctx, const User& admin, QWidget* parent = nullptr);
};
