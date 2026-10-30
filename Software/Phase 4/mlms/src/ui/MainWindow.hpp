#pragma once

#include "services/AppContext.hpp"

#include <QMainWindow>

#include <memory>

// Shell shown after sign-in: the signed-in user, sign-out, and the tabs of
// the one role view that matches the user's role (FR-02).
class MainWindow : public QMainWindow {
    Q_OBJECT
    AppContext& ctx_;
    std::unique_ptr<User> user_;

public:
    MainWindow(AppContext& ctx, std::unique_ptr<User> user, QWidget* parent = nullptr);
    ~MainWindow() override;

signals:
    void signedOut();
};
