#pragma once

#include "domain/DateTime.hpp"
#include "domain/Submission.hpp"
#include "services/AppContext.hpp"

#include <QDateTime>
#include <QString>
#include <QStringList>

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

class QTableWidget;
class QWidget;
class QPushButton;
class QHBoxLayout;

namespace ui {

inline QString q(const std::string& s) { return QString::fromStdString(s); }
inline std::string s(const QString& q) { return q.toStdString(); }

std::filesystem::path toPath(const QString& file);
QString fromPath(const std::filesystem::path& p);

// Times are stored in UTC and shown in the computer's local time zone.
QString localTime(DateTime t);
QDateTime toQDateTime(DateTime t);
DateTime fromQDateTime(const QDateTime& t);

QString stateLabel(SubmissionState s);
QString markText(double v);

// Runs a user action; any exception becomes a message box instead of a crash.
// Returns true when the action completed.
bool guarded(QWidget* parent, const std::function<void()>& action);

// A read-only, row-selecting table with the given column headings.
QTableWidget* makeTable(const QStringList& headers, QWidget* parent, bool multiSelect = false);
// Appends a row; `id` is stored on the first cell and read back by selectedIds().
void addRow(QTableWidget* table, int id, const QStringList& cells);
std::optional<int> currentId(QTableWidget* table);
std::vector<int> selectedIds(QTableWidget* table);

QPushButton* addButton(QHBoxLayout* row, const QString& text, const QString& tooltip = {});

// Shared by the HoD and Administrator screens.
QWidget* makeAuditLogTab(AppContext& ctx, const User& viewer, QWidget* parent);

} // namespace ui
