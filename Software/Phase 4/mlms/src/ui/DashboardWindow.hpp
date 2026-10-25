#pragma once

#include <QWidget>
#include <memory>
#include <sqlite3.h>
#include "../domain/User.hpp"

class QTabWidget;

class DashboardWindow : public QWidget {
    Q_OBJECT
    std::unique_ptr<User> user_;
    sqlite3* db_;
    QTabWidget* tabWidget_;

    void setupStudentUI();
    void setupLecturerUI();

public:
    explicit DashboardWindow(std::unique_ptr<User> user, sqlite3* db, QWidget* parent = nullptr);
};