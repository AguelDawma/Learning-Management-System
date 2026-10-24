#include "DashboardWindow.hpp"
#include "../data/AssignmentRepository.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTabWidget>
#include <QListWidget>
#include <QPushButton>

DashboardWindow::DashboardWindow(std::unique_ptr<User> user, sqlite3* db, QWidget* parent)
    : QWidget(parent), user_(std::move(user)), db_(db) {
    
    setWindowTitle(QString("MLMS - Dashboard (%1)").arg(QString::fromStdString(user_->fullName())));
    resize(700, 450);

    auto* mainLayout = new QVBoxLayout(this);

    // Welcome Header
    QString roleStr = (user_->role() == Role::STUDENT) ? "Student" : 
                      (user_->role() == Role::LECTURER) ? "Lecturer" : "Administrator";
    auto* welcomeLabel = new QLabel(QString("Welcome, %1 (%2)").arg(QString::fromStdString(user_->fullName()), roleStr), this);
    welcomeLabel->setStyleSheet("font-weight: bold; font-size: 14pt; margin-bottom: 5px;");
    mainLayout->addWidget(welcomeLabel);

    // Tab Widget for Flexible Navigation
    tabWidget_ = new QTabWidget(this);
    mainLayout->addWidget(tabWidget_);

    if (user_->role() == Role::STUDENT) {
        setupStudentUI();
    } else if (user_->role() == Role::LECTURER) {
        setupLecturerUI();
    } else {
        auto* adminTab = new QWidget();
        tabWidget_->addTab(adminTab, "System Administration");
    }
}

void DashboardWindow::setupStudentUI() {
    // Tab 1: Assignments View
    auto* assignmentsTab = new QWidget();
    auto* tab1Layout = new QVBoxLayout(assignmentsTab);

    auto* listWidget = new QListWidget(this);
    AssignmentRepository assignmentRepo(db_);
    auto assignments = assignmentRepo.findAll();

    for (const auto& a : assignments) {
        QString itemText = QString("Course %1: %2 (Deadline: %3)")
                           .arg(a.courseId())
                           .arg(QString::fromStdString(a.title()))
                           .arg(QString::fromStdString(a.deadline()));
        listWidget->addItem(itemText);
    }

    tab1Layout->addWidget(new QLabel("<b>Active Assignments:</b>", this));
    tab1Layout->addWidget(listWidget);
    
    auto* submitButton = new QPushButton("Upload Submission for Selected", this);
    tab1Layout->addWidget(submitButton);

    tabWidget_->addTab(assignmentsTab, "Assignments");

    // Tab 2: Grades View
    auto* gradesTab = new QWidget();
    auto* tab2Layout = new QVBoxLayout(gradesTab);
    tab2Layout->addWidget(new QLabel("Your graded submissions and feedback will appear here.", this));
    tabWidget_->addTab(gradesTab, "Grades & Feedback");
}

void DashboardWindow::setupLecturerUI() {
    auto* manageTab = new QWidget();
    auto* tabLayout = new QVBoxLayout(manageTab);
    tabLayout->addWidget(new QLabel("<b>Lecturer Controls:</b> Create assignments and review submissions.", this));
    
    auto* createButton = new QPushButton("Create New Assignment", this);
    tabLayout->addWidget(createButton);

    tabWidget_->addTab(manageTab, "Manage Assignments");
}