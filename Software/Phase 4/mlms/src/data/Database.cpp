#include "Database.hpp"
#include "../domain/Errors.hpp"
#include "SchemaSql.hpp"

#include <sqlite3.h>

namespace {

[[noreturn]] void fail(sqlite3* db, const std::string& what) {
    throw DatabaseError(what + ": " + (db ? sqlite3_errmsg(db) : "unknown error"));
}

} // namespace

// ---------------------------------------------------------------- Statement

Statement::Statement(sqlite3* db, const std::string& sql) : db_(db) {
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt_, nullptr) != SQLITE_OK) {
        fail(db_, "Could not prepare SQL");
    }
}

Statement::~Statement() { sqlite3_finalize(stmt_); }

Statement& Statement::bind(int index, int value) {
    sqlite3_bind_int(stmt_, index, value);
    return *this;
}

Statement& Statement::bind(int index, std::int64_t value) {
    sqlite3_bind_int64(stmt_, index, value);
    return *this;
}

Statement& Statement::bind(int index, double value) {
    sqlite3_bind_double(stmt_, index, value);
    return *this;
}

Statement& Statement::bind(int index, bool value) { return bind(index, value ? 1 : 0); }

Statement& Statement::bind(int index, const std::string& value) {
    sqlite3_bind_text(stmt_, index, value.c_str(), static_cast<int>(value.size()), SQLITE_TRANSIENT);
    return *this;
}

Statement& Statement::bind(int index, const char* value) { return bind(index, std::string(value)); }

Statement& Statement::bind(int index, DateTime value) { return bind(index, value.toIso()); }

Statement& Statement::bindNull(int index) {
    sqlite3_bind_null(stmt_, index);
    return *this;
}

bool Statement::step() {
    const int rc = sqlite3_step(stmt_);
    if (rc == SQLITE_ROW) return true;
    if (rc == SQLITE_DONE) return false;
    if (rc == SQLITE_BUSY) {
        throw DatabaseError("The database is busy (another computer is saving). Please try again.");
    }
    fail(db_, "Database error");
}

void Statement::run() {
    while (step()) {
    }
}

bool Statement::isNull(int col) const { return sqlite3_column_type(stmt_, col) == SQLITE_NULL; }

int Statement::getInt(int col) const { return sqlite3_column_int(stmt_, col); }

double Statement::getDouble(int col) const { return sqlite3_column_double(stmt_, col); }

std::string Statement::getText(int col) const {
    const auto* text = sqlite3_column_text(stmt_, col);
    return text ? std::string(reinterpret_cast<const char*>(text)) : std::string();
}

std::optional<std::string> Statement::getOptText(int col) const {
    if (isNull(col)) return std::nullopt;
    return getText(col);
}

std::optional<int> Statement::getOptInt(int col) const {
    if (isNull(col)) return std::nullopt;
    return getInt(col);
}

DateTime Statement::getDateTime(int col) const { return DateTime::parseIso(getText(col)); }

std::optional<DateTime> Statement::getOptDateTime(int col) const {
    if (isNull(col)) return std::nullopt;
    return getDateTime(col);
}

// ---------------------------------------------------------------- Transaction

// The outermost transaction uses BEGIN IMMEDIATE so a writer takes the lock
// up front (no deadlock between two lab computers); nested ones use savepoints
// so a service can call another service inside its own transaction.
class SqliteTransaction : public ITransaction {
    Database& db_;
    int level_;
    bool done_ = false;

    std::string savepoint() const { return "sp" + std::to_string(level_); }

public:
    explicit SqliteTransaction(Database& db) : db_(db), level_(db.depth_) {
        db_.exec(level_ == 0 ? "BEGIN IMMEDIATE" : "SAVEPOINT " + savepoint());
        ++db_.depth_;
    }

    ~SqliteTransaction() override {
        if (done_) return;
        --db_.depth_;
        // Destructors must not throw; the rollback result is deliberately ignored.
        const std::string sql =
            level_ == 0 ? "ROLLBACK" : "ROLLBACK TO " + savepoint() + "; RELEASE " + savepoint();
        sqlite3_exec(db_.db_, sql.c_str(), nullptr, nullptr, nullptr);
    }

    void commit() override {
        if (done_) return;
        db_.exec(level_ == 0 ? "COMMIT" : "RELEASE " + savepoint());
        done_ = true;
        --db_.depth_;
    }
};

// ---------------------------------------------------------------- Database

Database::Database(const std::string& path) {
    if (sqlite3_open_v2(path.c_str(), &db_, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK) {
        const std::string msg = db_ ? sqlite3_errmsg(db_) : "out of memory";
        sqlite3_close(db_);
        db_ = nullptr;
        throw DatabaseError("Could not open database '" + path + "': " + msg);
    }
    sqlite3_busy_timeout(db_, 5000);
    exec("PRAGMA foreign_keys = ON");
}

Database::~Database() { sqlite3_close(db_); }

void Database::exec(const std::string& sql) {
    char* err = nullptr;
    if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK) {
        const std::string msg = err ? err : "unknown error";
        sqlite3_free(err);
        throw DatabaseError("Database error: " + msg);
    }
}

void Database::initialiseSchema() {
    int version = 0;
    {
        auto st = prepare("PRAGMA user_version");
        if (st.step()) version = st.getInt(0);
    }
    if (version == kSchemaVersion) return;
    if (version > kSchemaVersion) {
        throw DatabaseError("The database was created by a newer version of MLMS");
    }
    int tables = 0;
    {
        auto st = prepare("SELECT COUNT(*) FROM sqlite_master WHERE type = 'table'");
        if (st.step()) tables = st.getInt(0);
    }
    if (tables > 0) {
        throw DatabaseError("The database file uses an unknown schema (e.g. an old prototype)");
    }
    exec(kSchemaSql);
    exec("PRAGMA user_version = " + std::to_string(kSchemaVersion));
}

std::int64_t Database::lastInsertId() const { return sqlite3_last_insert_rowid(db_); }

int Database::changes() const { return sqlite3_changes(db_); }

std::unique_ptr<ITransaction> Database::begin() { return std::make_unique<SqliteTransaction>(*this); }
