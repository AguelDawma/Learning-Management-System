// MLMS entry point and composition root: chooses the data folder, opens the
// database, creates the concrete classes and wires the sign-in flow.
//
// Usage: mlms [--data <folder>] [--demo]
//   --data  folder for mlms.db and submitted files (default: MLMS_DATA_DIR, else <exe folder>/data)
//   --demo  fill an empty database with demo accounts (password "Password1")

#include "domain/Errors.hpp"
#include "services/AppContext.hpp"
#include "services/Bootstrap.hpp"
#include "ui/Dialogs.hpp"
#include "ui/LoginWindow.hpp"
#include "ui/MainWindow.hpp"
#include "ui/UiHelpers.hpp"

#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QStandardPaths>

#include <memory>

namespace {

QString chooseDataDir(const QCommandLineParser& args) {
    if (args.isSet("data")) return args.value("data");
    const QString env = qEnvironmentVariable("MLMS_DATA_DIR");
    if (!env.isEmpty()) return env;
    const QString besideExe = QCoreApplication::applicationDirPath() + "/data";
    if (QDir().mkpath(besideExe) && QFileInfo(besideExe).isWritable()) return besideExe;
    // e.g. installed under Program Files, which ordinary users cannot write to
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

std::unique_ptr<AppContext> openContext(const QString& dataDir) {
    const QString dbPath = dataDir + "/mlms.db";
    auto make = [&] {
        return std::make_unique<AppContext>(ui::s(dbPath), ui::toPath(dataDir + "/submissions"),
                                            std::make_unique<SystemClock>(), std::make_unique<Argon2Hasher>());
    };
    try {
        return make();
    } catch (const DatabaseError& e) {
        if (QString(e.what()).contains("unknown schema")) {
            const QString backup = dbPath + ".old";
            const auto answer = QMessageBox::question(
                nullptr, "Old database found",
                QString("The file %1 was created by an older version of the program and cannot be used.\n\n"
                        "Rename it to %2 and start with a new, empty database?")
                    .arg(QDir::toNativeSeparators(dbPath), QFileInfo(backup).fileName()));
            if (answer == QMessageBox::Yes) {
                QFile::remove(backup);
                if (QFile::rename(dbPath, backup)) return make();
            }
        }
        throw;
    }
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("MLMS");
    QApplication::setOrganizationName("School");
    QApplication::setApplicationVersion(PROJECT_VERSION);
    QFont font = QApplication::font();
    font.setPointSize(std::max(font.pointSize(), 10));
    QApplication::setFont(font);

    QCommandLineParser args;
    args.addHelpOption();
    args.addVersionOption();
    args.addOption({"data", "Folder for the database and submitted files.", "folder"});
    args.addOption({"demo", "Fill an empty database with demo accounts and courses."});
    args.process(app);

    std::unique_ptr<AppContext> ctx;
    try {
        const QString dataDir = chooseDataDir(args);
        if (!QDir().mkpath(dataDir)) throw MlmsError("Cannot create the data folder " + ui::s(dataDir));
        ctx = openContext(dataDir);
        if (args.isSet("demo")) seedDemoData(*ctx);
        if (auto password = ensureAdministrator(*ctx)) {
            QMessageBox::information(nullptr, "First start",
                                     QString("An administrator account has been created.\n\nUsername: admin\n"
                                             "Temporary password: %1\n\nWrite it down now. You will be asked to "
                                             "choose a new password when you sign in.")
                                         .arg(ui::q(*password)));
        }
    } catch (const std::exception& e) {
        QMessageBox::critical(nullptr, "MLMS cannot start", e.what());
        return 1;
    }

    // Sign-in loop: signing out returns to a fresh login window.
    std::function<void()> showLogin;
    showLogin = [&] {
        auto* login = new LoginWindow(ctx->auth);
        login->setAttribute(Qt::WA_DeleteOnClose);
        QObject::connect(login, &LoginWindow::loggedIn, login, [&, login](User* raw, bool mustChange) {
            std::unique_ptr<User> user(raw);
            if (mustChange) {
                ChangePasswordDialog dlg(ctx->auth, *user, true, login);
                if (dlg.exec() != QDialog::Accepted) return; // stay on the sign-in screen
            }
            auto* main = new MainWindow(*ctx, std::move(user));
            main->setAttribute(Qt::WA_DeleteOnClose);
            QObject::connect(main, &MainWindow::signedOut, [&] { showLogin(); });
            main->show();
            login->close();
        });
        login->show();
    };
    showLogin();
    return QApplication::exec();
}
