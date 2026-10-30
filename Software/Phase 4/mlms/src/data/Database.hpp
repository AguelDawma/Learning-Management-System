#pragma once

#include "../domain/DateTime.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

struct sqlite3;
struct sqlite3_stmt;

// A unit of work. Destroying it without commit() rolls everything back, so a
// use case either fully succeeds or leaves the database unchanged (NFR-07).
class ITransaction {
public:
    virtual ~ITransaction() = default;
    virtual void commit() = 0;
};

class ITransactionManager {
public:
    virtual ~ITransactionManager() = default;
    virtual std::unique_ptr<ITransaction> begin() = 0;
};

// RAII wrapper over a prepared statement. Values are only ever bound as
// parameters, never concatenated into SQL (NFR-05).
class Statement {
    sqlite3* db_;
    sqlite3_stmt* stmt_ = nullptr;

public:
    Statement(sqlite3* db, const std::string& sql);
    ~Statement();
    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;

    Statement& bind(int index, int value);
    Statement& bind(int index, std::int64_t value);
    Statement& bind(int index, double value);
    Statement& bind(int index, bool value);
    Statement& bind(int index, const std::string& value);
    Statement& bind(int index, const char* value);
    Statement& bind(int index, DateTime value);
    Statement& bindNull(int index);
    template <typename T>
    Statement& bind(int index, const std::optional<T>& value) {
        return value ? bind(index, *value) : bindNull(index);
    }

    // Returns true while a row is available.
    bool step();
    // Runs a statement that returns no rows.
    void run();

    bool isNull(int col) const;
    int getInt(int col) const;
    double getDouble(int col) const;
    std::string getText(int col) const;
    std::optional<std::string> getOptText(int col) const;
    std::optional<int> getOptInt(int col) const;
    DateTime getDateTime(int col) const;
    std::optional<DateTime> getOptDateTime(int col) const;
};

class Database : public ITransactionManager {
    sqlite3* db_ = nullptr;
    int depth_ = 0;

    friend class SqliteTransaction;

public:
    static constexpr int kSchemaVersion = 1;

    // Opens (or creates) the file; ":memory:" gives a private in-memory database for tests.
    explicit Database(const std::string& path);
    ~Database() override;
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    // Creates the schema in an empty database. Throws DatabaseError if the file
    // holds tables from an unknown (e.g. prototype) schema.
    void initialiseSchema();

    void exec(const std::string& sql);
    Statement prepare(const std::string& sql) { return Statement(db_, sql); }
    std::int64_t lastInsertId() const;
    int changes() const;

    std::unique_ptr<ITransaction> begin() override;
};
