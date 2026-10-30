#pragma once

#include "Database.hpp"
#include "Repositories.hpp"

class SqliteUserRepository : public IUserRepository {
    Database& db_;

public:
    explicit SqliteUserRepository(Database& db) : db_(db) {}
    std::optional<UserRecord> findByUsername(const std::string& username) override;
    std::optional<UserRecord> findById(int id) override;
    std::vector<UserRecord> listAll() override;
    int add(const UserRecord& user) override;
    void updatePassword(int id, const std::string& hash, bool mustChange) override;
    void updateLoginState(int id, int failedAttempts, std::optional<DateTime> lockedUntil) override;
    void setActive(int id, bool active) override;
};

class SqliteCourseRepository : public ICourseRepository {
    Database& db_;

public:
    explicit SqliteCourseRepository(Database& db) : db_(db) {}
    int addDepartment(const std::string& name) override;
    void setHod(int departmentId, std::optional<int> hodId) override;
    std::vector<Department> listDepartments() override;
    std::optional<Department> findDepartment(int id) override;
    int addCourse(const Course& course) override;
    std::optional<Course> findCourse(int id) override;
    std::vector<Course> listCourses() override;
    std::vector<Course> coursesForLecturer(int lecturerId) override;
    std::vector<Course> coursesForStudent(int studentId) override;
    std::vector<Course> coursesForDepartment(int departmentId) override;
    void assignLecturer(int courseId, int lecturerId) override;
    void removeLecturer(int courseId, int lecturerId) override;
    bool isLecturerOf(int courseId, int lecturerId) override;
    std::vector<UserRecord> lecturersOf(int courseId) override;
    void enrol(int courseId, int studentId, DateTime at) override;
    void unenrol(int courseId, int studentId) override;
    bool isEnrolled(int courseId, int studentId) override;
    std::vector<UserRecord> studentsIn(int courseId) override;
};

class SqliteAssignmentRepository : public IAssignmentRepository {
    Database& db_;

public:
    explicit SqliteAssignmentRepository(Database& db) : db_(db) {}
    std::optional<Assignment> find(int id) override;
    std::vector<Assignment> findByCourse(int courseId) override;
    int add(const Assignment& a) override;
    void setPublished(int id, bool published) override;
    std::optional<Extension> findExtension(int assignmentId, int studentId) override;
    std::vector<Extension> findExtensions(int assignmentId) override;
    void saveExtension(const Extension& e) override;
};

class SqliteSubmissionRepository : public ISubmissionRepository {
    Database& db_;

public:
    explicit SqliteSubmissionRepository(Database& db) : db_(db) {}
    std::optional<Submission> find(int id) override;
    std::optional<Submission> findActive(int assignmentId, int studentId) override;
    std::vector<Submission> findActiveByAssignment(int assignmentId) override;
    std::vector<Submission> findVersions(int assignmentId, int studentId) override;
    int add(const Submission& s) override;
    void updateState(int id, SubmissionState state) override;
};

class SqliteMarkRepository : public IMarkRepository {
    Database& db_;

public:
    explicit SqliteMarkRepository(Database& db) : db_(db) {}
    std::optional<Mark> find(int submissionId) override;
    void insert(const Mark& m) override;
    int update(const Mark& m) override;
};

class SqliteAuditRepository : public IAuditRepository {
    Database& db_;

public:
    explicit SqliteAuditRepository(Database& db) : db_(db) {}
    void append(const AuditEntry& e) override;
    std::vector<AuditEntry> findBySubmission(int submissionId) override;
    std::vector<AuditEntry> findByDepartment(int departmentId) override;
    std::vector<AuditEntry> findAll() override;
};
