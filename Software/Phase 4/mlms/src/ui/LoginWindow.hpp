#pragma once

#include <QWidget>
#include "../services/AuthService.hpp"
#include <sqlite3.h>

class QLineEdit;
class QPushButton;

class LoginWindow : public QWidget {
    Q_OBJECT
    AuthService& authService_;
    sqlite3* db_;
    QLineEdit* usernameEdit_;
    QLineEdit* passwordEdit_;

public:
    explicit LoginWindow(AuthService& authService, sqlite3* db, QWidget* parent = nullptr);

private slots:
    void handleLogin();
};