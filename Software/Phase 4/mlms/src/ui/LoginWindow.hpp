#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include "../services/AuthService.hpp"

class LoginWindow : public QWidget {
    Q_OBJECT

    AuthService& authService_;
    QLineEdit* usernameEdit_;
    QLineEdit* passwordEdit_;
    QPushButton* loginButton_;

public:
    explicit LoginWindow(AuthService& authService, QWidget* parent = nullptr);

private slots:
    void handleLogin();
};