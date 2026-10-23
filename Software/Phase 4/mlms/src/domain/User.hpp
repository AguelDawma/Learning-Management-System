#pragma once

#include <string>

enum class Role {
    STUDENT,
    LECTURER,
    HOD,
    ADMINISTRATOR
};

class User {
protected:
    int id_;
    std::string username_;
    std::string fullName_;
    std::string passwordHash_;
    bool active_;

public:
    User(int id, std::string username, std::string fullName, std::string passwordHash, bool active)
        : id_(id), username_(std::move(username)), fullName_(std::move(fullName)),
          passwordHash_(std::move(passwordHash)), active_(active) {}

    virtual ~User() = default;

    virtual Role role() const = 0;
    virtual bool can(const std::string& permission) const = 0;

    int id() const { return id_; }
    const std::string& username() const { return username_; }
    const std::string& fullName() const { return fullName_; }
    const std::string& passwordHash() const { return passwordHash_; }
    bool isActive() const { return active_; }
};

class Student : public User {
public:
    using User::User;
    Role role() const override { return Role::STUDENT; }
    bool can(const std::string& permission) const override;
};

class Lecturer : public User {
public:
    using User::User;
    Role role() const override { return Role::LECTURER; }
    bool can(const std::string& permission) const override;
};

class HeadOfDepartment : public User {
    int departmentId_;
public:
    HeadOfDepartment(int id, std::string username, std::string fullName, std::string passwordHash, bool active, int departmentId)
        : User(id, std::move(username), std::move(fullName), std::move(passwordHash), active), departmentId_(departmentId) {}

    Role role() const override { return Role::HOD; }
    bool can(const std::string& permission) const override;
    int departmentId() const { return departmentId_; }
};

class Administrator : public User {
public:
    using User::User;
    Role role() const override { return Role::ADMINISTRATOR; }
    bool can(const std::string& permission) const override;
};