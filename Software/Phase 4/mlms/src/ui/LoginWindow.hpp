#pragma once

#include "services/AuthService.hpp"

#include <QWidget>

class QLineEdit;
class QLabel;

class LoginWindow : public QWidget {
    Q_OBJECT
    AuthService& auth_;
    QLineEdit* usernameEdit_;
    QLineEdit* passwordEdit_;
    QLabel* errorLabel_;

public:
    explicit LoginWindow(AuthService& auth, QWidget* parent = nullptr);

signals:
    // Ownership of the user passes to the receiver.
    void loggedIn(User* user, bool mustChangePassword);

private slots:
    void handleLogin();
};
