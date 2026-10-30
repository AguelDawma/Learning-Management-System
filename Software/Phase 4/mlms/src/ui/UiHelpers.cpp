#include "UiHelpers.hpp"
#include "domain/Errors.hpp"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include <set>

namespace ui {

std::filesystem::path toPath(const QString& file) {
#ifdef _WIN32
    return std::filesystem::path(file.toStdWString());
#else
    return std::filesystem::u8path(file.toStdString());
#endif
}

QString fromPath(const std::filesystem::path& p) {
#ifdef _WIN32
    return QString::fromStdWString(p.wstring());
#else
    return QString::fromStdString(p.u8string());
#endif
}

QDateTime toQDateTime(DateTime t) { return QDateTime::fromSecsSinceEpoch(t.epochSeconds()); } // local time

DateTime fromQDateTime(const QDateTime& t) { return DateTime(t.toSecsSinceEpoch()); }

QString localTime(DateTime t) { return toQDateTime(t).toString("ddd d MMM yyyy, HH:mm"); }

QString stateLabel(SubmissionState st) {
    switch (st) {
        case SubmissionState::SUBMITTED: return "Submitted";
        case SubmissionState::LATE: return "Submitted late";
        case SubmissionState::MARKED: return "Marked - awaiting HoD";
        case SubmissionState::RETURNED: return "Returned to lecturer";
        case SubmissionState::MODERATED: return "Moderated - not yet released";
        case SubmissionState::RELEASED: return "Released";
    }
    return {};
}

QString markText(double v) { return q(formatMark(v)); }

bool guarded(QWidget* parent, const std::function<void()>& action) {
    try {
        action();
        return true;
    } catch (const ValidationError& e) {
        QMessageBox::warning(parent, "Please check your input", e.what());
    } catch (const AccessDenied& e) {
        QMessageBox::warning(parent, "Not allowed", e.what());
    } catch (const SubmissionRefused& e) {
        QMessageBox::warning(parent, "Submission refused", e.what());
    } catch (const InvalidTransition& e) {
        QMessageBox::warning(parent, "Not possible now", e.what());
    } catch (const ConcurrencyError& e) {
        QMessageBox::warning(parent, "Changed by someone else", e.what());
    } catch (const NotFound& e) {
        QMessageBox::warning(parent, "Not found", e.what());
    } catch (const std::exception& e) {
        QMessageBox::critical(parent, "Error", QString("The action could not be completed.\n\n%1").arg(e.what()));
    }
    return false;
}

QTableWidget* makeTable(const QStringList& headers, QWidget* parent, bool multiSelect) {
    auto* t = new QTableWidget(0, static_cast<int>(headers.size()), parent);
    t->setHorizontalHeaderLabels(headers);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setSelectionMode(multiSelect ? QAbstractItemView::ExtendedSelection : QAbstractItemView::SingleSelection);
    t->setAlternatingRowColors(true);
    t->verticalHeader()->setVisible(false);
    t->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    t->horizontalHeader()->setStretchLastSection(true);
    t->setWordWrap(false);
    return t;
}

void addRow(QTableWidget* table, int id, const QStringList& cells) {
    const int row = table->rowCount();
    table->insertRow(row);
    for (int c = 0; c < cells.size() && c < table->columnCount(); ++c) {
        auto* item = new QTableWidgetItem(cells[c]);
        item->setToolTip(cells[c]);
        if (c == 0) item->setData(Qt::UserRole, id);
        table->setItem(row, c, item);
    }
}

std::optional<int> currentId(QTableWidget* table) {
    const int row = table->currentRow();
    if (row < 0 || table->item(row, 0) == nullptr || !table->item(row, 0)->isSelected()) return std::nullopt;
    return table->item(row, 0)->data(Qt::UserRole).toInt();
}

std::vector<int> selectedIds(QTableWidget* table) {
    std::set<int> rows;
    for (auto* item : table->selectedItems()) rows.insert(item->row());
    std::vector<int> ids;
    for (int r : rows) ids.push_back(table->item(r, 0)->data(Qt::UserRole).toInt());
    return ids;
}

QPushButton* addButton(QHBoxLayout* row, const QString& text, const QString& tooltip) {
    auto* b = new QPushButton(text);
    b->setToolTip(tooltip);
    b->setAccessibleDescription(tooltip);
    row->addWidget(b);
    return b;
}

QWidget* makeAuditLogTab(AppContext& ctx, const User& viewer, QWidget* parent) {
    auto* page = new QWidget(parent);
    auto* layout = new QVBoxLayout(page);
    layout->addWidget(new QLabel("Every change to a mark or submission state: who changed it, when, the old and "
                                 "new value, and why. This log cannot be edited or deleted."));
    auto* table = makeTable({"When", "Changed by", "Student", "Assignment", "What", "Old value", "New value", "Reason"},
                            page);
    table->setAccessibleName("Audit log");
    layout->addWidget(table);
    auto* buttons = new QHBoxLayout;
    auto* refresh = addButton(buttons, "&Refresh");
    buttons->addStretch();
    layout->addLayout(buttons);

    auto load = [&ctx, &viewer, table, page] {
        guarded(page, [&] {
            table->setRowCount(0);
            for (const auto& v : ctx.auditService.entries(viewer)) {
                addRow(table, v.entry.id,
                       {localTime(v.entry.timestamp), q(v.changedBy), q(v.studentName), q(v.assignmentTitle),
                        q(v.entry.field), q(v.entry.oldValue.value_or("-")), q(v.entry.newValue), q(v.entry.reason)});
            }
        });
    };
    QObject::connect(refresh, &QPushButton::clicked, page, load);
    load();
    return page;
}

} // namespace ui
