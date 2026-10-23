#include "LoginWindow.hpp"
#include <QVBoxLayout>
#include <QMessageBox>

LoginWindow::LoginWindow(AuthService& authService, QWidget* parent) 
    : QWidget(parent), authService_(authService) {
    
    setWindowTitle("MLMS - Login");
    resize(300, 200);

    auto* layout = new QVBoxLayout(this);

    auto* titleLabel = new QLabel("Welcome to MLMS", this);
    layout->addWidget(titleLabel);

    usernameEdit_ = new QLineEdit(this);
    usernameEdit_->setPlaceholderText("Username");
    layout->addWidget(usernameEdit_);

    passwordEdit_ = new QLineEdit(this);
    passwordEdit_->setPlaceholderText("Password");
    passwordEdit_->setEchoMode(QLineEdit::Password);
    layout->addWidget(passwordEdit_);

    loginButton_ = new QPushButton("Login", this);
    layout->addWidget(loginButton_);

    connect(loginButton_, &QPushButton::clicked, this, &LoginWindow::handleLogin);
}

void LoginWindow::handleLogin() {
    std::string username = usernameEdit_->text().toStdString();
    std::string password = passwordEdit_->text().toStdString();

    if (authService_.authenticate(username, password)) {
        QMessageBox::information(this, "Success", "Login successful!");
    } else {
        QMessageBox::warning(this, "Failure", "Invalid username or password, or account inactive.");
    }
}