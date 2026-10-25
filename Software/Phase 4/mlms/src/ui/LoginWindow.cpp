#include "LoginWindow.hpp"
#include "DashboardWindow.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>

LoginWindow::LoginWindow(AuthService& authService, sqlite3* db, QWidget* parent)
    : QWidget(parent), authService_(authService), db_(db) {
    
    setWindowTitle("MLMS - Login");
    resize(350, 200);

    auto* layout = new QVBoxLayout(this);

    usernameEdit_ = new QLineEdit(this);
    usernameEdit_->setPlaceholderText("Username");
    passwordEdit_ = new QLineEdit(this);
    passwordEdit_->setPlaceholderText("Password");
    passwordEdit_->setEchoMode(QLineEdit::Password);

    auto* loginButton = new QPushButton("Login", this);

    layout->addWidget(new QLabel("<b>System Login</b>", this));
    layout->addWidget(usernameEdit_);
    layout->addWidget(passwordEdit_);
    layout->addWidget(loginButton);

    connect(loginButton, &QPushButton::clicked, this, &LoginWindow::handleLogin);
}

void LoginWindow::handleLogin() {
    std::string username = usernameEdit_->text().toStdString();
    std::string password = passwordEdit_->text().toStdString();

    auto user = authService_.authenticate(username, password);
    if (user) {
        auto* dashboard = new DashboardWindow(std::move(user), db_);
        dashboard->setAttribute(Qt::WA_DeleteOnClose);
        dashboard->show();
        this->close();
    } else {
        QMessageBox::warning(this, "Failure", "Invalid username or password, or account inactive.");
    }
}