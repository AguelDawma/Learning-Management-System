#pragma once

#include <string>

enum class Role { STUDENT, LECTURER, HOD, ADMINISTRATOR };

std::string toString(Role role);
Role roleFromString(const std::string& text); // throws ValidationError

// Actions guarded by role-based access control (FR-02).
enum class Permission {
    ViewOwnCourses,
    SubmitAssignment,
    ViewReleasedMarks,
    CreateAssignment,
    GrantExtension,
    MarkSubmission,
    ModerateMarks,
    ReleaseMarks,
    ViewAuditLog,
    ManageUsers,
    ManageCourses,
    ManageEnrolments,
};

// Abstract base of the role hierarchy. Subclasses add behaviour (role/can),
// not data, which is why one app_user table stores them all.
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
    virtual bool can(Permission p) const = 0;

    int id() const { return id_; }
    const std::string& username() const { return username_; }
    const std::string& fullName() const { return fullName_; }
    const std::string& passwordHash() const { return passwordHash_; }
    bool isActive() const { return active_; }

    // Throws AccessDenied unless this user is active and holds the permission.
    void require(Permission p) const;
};

class Student : public User {
public:
    using User::User;
    Role role() const override { return Role::STUDENT; }
    bool can(Permission p) const override;
};

class Lecturer : public User {
public:
    using User::User;
    Role role() const override { return Role::LECTURER; }
    bool can(Permission p) const override;
};

class HeadOfDepartment : public User {
    int departmentId_;

public:
    HeadOfDepartment(int id, std::string username, std::string fullName, std::string passwordHash, bool active,
                     int departmentId)
        : User(id, std::move(username), std::move(fullName), std::move(passwordHash), active),
          departmentId_(departmentId) {}

    Role role() const override { return Role::HOD; }
    bool can(Permission p) const override;
    // 0 while the HoD has not been assigned to a department.
    int departmentId() const { return departmentId_; }
};

class Administrator : public User {
public:
    using User::User;
    Role role() const override { return Role::ADMINISTRATOR; }
    bool can(Permission p) const override;
};
