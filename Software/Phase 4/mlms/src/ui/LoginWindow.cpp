#include "LoginWindow.hpp"
#include "UiHelpers.hpp"
#include "domain/Errors.hpp"

#include <QFormLayout>
#include <QGuiApplication>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

LoginWindow::LoginWindow(AuthService& auth, QWidget* parent) : QWidget(parent), auth_(auth) {
    setWindowTitle("MLMS - Sign in");
    setMinimumWidth(380);

    auto* layout = new QVBoxLayout(this);
    auto* title = new QLabel("<h2>Moderated Learning Management System</h2>");
    title->setWordWrap(true);
    layout->addWidget(title);
    layout->addWidget(new QLabel("Sign in with the username and password given by the administrator."));

    auto* form = new QFormLayout;
    usernameEdit_ = new QLineEdit;
    usernameEdit_->setAccessibleName("Username");
    passwordEdit_ = new QLineEdit;
    passwordEdit_->setEchoMode(QLineEdit::Password);
    passwordEdit_->setAccessibleName("Password");
    form->addRow("&Username:", usernameEdit_);
    form->addRow("&Password:", passwordEdit_);
    layout->addLayout(form);

    errorLabel_ = new QLabel;
    errorLabel_->setStyleSheet("color: #b00020;");
    errorLabel_->setWordWrap(true);
    errorLabel_->setAccessibleName("Sign-in error");
    layout->addWidget(errorLabel_);

    auto* loginButton = new QPushButton("&Sign in");
    loginButton->setDefault(true);
    layout->addWidget(loginButton);

    connect(loginButton, &QPushButton::clicked, this, &LoginWindow::handleLogin);
    connect(passwordEdit_, &QLineEdit::returnPressed, this, &LoginWindow::handleLogin);
    connect(usernameEdit_, &QLineEdit::returnPressed, passwordEdit_, qOverload<>(&QWidget::setFocus));
    usernameEdit_->setFocus();
}

void LoginWindow::handleLogin() {
    errorLabel_->clear();
    QGuiApplication::setOverrideCursor(Qt::WaitCursor);
    try {
        auto result = auth_.login(ui::s(usernameEdit_->text()), ui::s(passwordEdit_->text()));
        QGuiApplication::restoreOverrideCursor();
        passwordEdit_->clear();
        emit loggedIn(result.user.release(), result.mustChangePassword);
    } catch (const AuthenticationError& e) {
        QGuiApplication::restoreOverrideCursor();
        errorLabel_->setText(e.what());
        passwordEdit_->clear();
        passwordEdit_->setFocus();
    } catch (const std::exception& e) {
        QGuiApplication::restoreOverrideCursor();
        errorLabel_->setText(QString("Sign-in failed: %1").arg(e.what()));
    }
}
