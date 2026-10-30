#pragma once

#include "services/AppContext.hpp"

#include <QTabWidget>

class QComboBox;
class QLabel;
class QTableWidget;
class QTextEdit;

// One view per role. Views only call services; all rules live below them.

class StudentView : public QTabWidget {
    AppContext& ctx_;
    const User& user_;
    QLabel* courses_;
    QTableWidget* assignments_;
    QTextEdit* details_;
    QTableWidget* results_;
    QTextEdit* feedback_;
    std::vector<StudentAssignmentView> rows_;

    void reload();
    void showDetails();
    void submit();
    void showVersions();

public:
    StudentView(AppContext& ctx, const User& user, QWidget* parent = nullptr);
};

class LecturerView : public QTabWidget {
    AppContext& ctx_;
    const User& user_;
    QTableWidget* assignments_;
    QComboBox* markingAssignment_;
    QTableWidget* marking_;
    QLabel* markingInfo_;
    std::vector<AssignmentSummary> myAssignments_;
    std::vector<MarkingRow> markingRows_;

    const Assignment* assignmentById(int id) const;
    void reloadAssignments();
    void reloadMarking();
    void createAssignment();
    void togglePublished();
    void manageExtensions();
    void openFile();
    void markSelected();

public:
    LecturerView(AppContext& ctx, const User& user, QWidget* parent = nullptr);
};

class HoDView : public QTabWidget {
    AppContext& ctx_;
    const User& user_;
    QComboBox* assignment_;
    QTableWidget* sheet_;
    QTextEdit* feedback_;
    std::vector<AssignmentSummary> deptAssignments_;
    std::vector<MarkingRow> rows_;

    void reloadAssignments();
    void reloadSheet();
    void approve();
    void returnToLecturer();
    void adjust();
    void release();
    void openFile();
    const MarkingRow* rowFor(int submissionId) const;

public:
    HoDView(AppContext& ctx, const User& user, QWidget* parent = nullptr);
};

class AdminView : public QTabWidget {
    AppContext& ctx_;
    const User& user_;
    QTableWidget* users_;
    QTableWidget* departments_;
    QTableWidget* courses_;
    QTableWidget* lecturers_;
    QTableWidget* students_;
    std::vector<UserRecord> userList_;

    const UserRecord* userById(int id) const;
    void reloadUsers();
    void reloadOrganisation();
    void reloadCourseMembers();
    std::optional<int> pickUser(Role role, const QString& title);

public:
    AdminView(AppContext& ctx, const User& user, QWidget* parent = nullptr);
};
