#pragma once

#include "../domain/Assignment.hpp"
#include "../domain/AuditEntry.hpp"
#include "../domain/Mark.hpp"
#include "../domain/Organisation.hpp"
#include "../domain/Submission.hpp"
#include "../domain/UserFactory.hpp"

#include <optional>
#include <string>
#include <vector>

// Repository pattern: services depend only on these interfaces. The SQLite
// classes implement them; tests can substitute fakes or an in-memory database.

class IUserRepository {
public:
    virtual ~IUserRepository() = default;
    virtual std::optional<UserRecord> findByUsername(const std::string& username) = 0;
    virtual std::optional<UserRecord> findById(int id) = 0;
    virtual std::vector<UserRecord> listAll() = 0;
    virtual int add(const UserRecord& user) = 0;
    virtual void updatePassword(int id, const std::string& hash, bool mustChange) = 0;
    virtual void updateLoginState(int id, int failedAttempts, std::optional<DateTime> lockedUntil) = 0;
    virtual void setActive(int id, bool active) = 0;
};

// Departments, courses and the two junction tables (course_lecturer, enrolment).
class ICourseRepository {
public:
    virtual ~ICourseRepository() = default;
    virtual int addDepartment(const std::string& name) = 0;
    virtual void setHod(int departmentId, std::optional<int> hodId) = 0;
    virtual std::vector<Department> listDepartments() = 0;
    virtual std::optional<Department> findDepartment(int id) = 0;

    virtual int addCourse(const Course& course) = 0;
    virtual std::optional<Course> findCourse(int id) = 0;
    virtual std::vector<Course> listCourses() = 0;
    virtual std::vector<Course> coursesForLecturer(int lecturerId) = 0;
    virtual std::vector<Course> coursesForStudent(int studentId) = 0;
    virtual std::vector<Course> coursesForDepartment(int departmentId) = 0;

    virtual void assignLecturer(int courseId, int lecturerId) = 0;
    virtual void removeLecturer(int courseId, int lecturerId) = 0;
    virtual bool isLecturerOf(int courseId, int lecturerId) = 0;
    virtual std::vector<UserRecord> lecturersOf(int courseId) = 0;

    virtual void enrol(int courseId, int studentId, DateTime at) = 0;
    virtual void unenrol(int courseId, int studentId) = 0;
    virtual bool isEnrolled(int courseId, int studentId) = 0;
    virtual std::vector<UserRecord> studentsIn(int courseId) = 0;
};

class IAssignmentRepository {
public:
    virtual ~IAssignmentRepository() = default;
    virtual std::optional<Assignment> find(int id) = 0;
    virtual std::vector<Assignment> findByCourse(int courseId) = 0;
    virtual int add(const Assignment& a) = 0;
    virtual void setPublished(int id, bool published) = 0;
    virtual std::optional<Extension> findExtension(int assignmentId, int studentId) = 0;
    virtual std::vector<Extension> findExtensions(int assignmentId) = 0;
    // Inserts or replaces the single extension a student may hold for an assignment.
    virtual void saveExtension(const Extension& e) = 0;
};

class ISubmissionRepository {
public:
    virtual ~ISubmissionRepository() = default;
    virtual std::optional<Submission> find(int id) = 0;
    // The active submission is the newest version (BR-03).
    virtual std::optional<Submission> findActive(int assignmentId, int studentId) = 0;
    virtual std::vector<Submission> findActiveByAssignment(int assignmentId) = 0;
    virtual std::vector<Submission> findVersions(int assignmentId, int studentId) = 0;
    virtual int add(const Submission& s) = 0;
    virtual void updateState(int id, SubmissionState state) = 0;
};

class IMarkRepository {
public:
    virtual ~IMarkRepository() = default;
    virtual std::optional<Mark> find(int submissionId) = 0;
    virtual void insert(const Mark& m) = 0;
    // Optimistic locking: succeeds only if the stored row_version still equals
    // m.rowVersion(); returns the new row_version, else throws ConcurrencyError.
    virtual int update(const Mark& m) = 0;
};

class IAuditRepository {
public:
    virtual ~IAuditRepository() = default;
    virtual void append(const AuditEntry& e) = 0;
    virtual std::vector<AuditEntry> findBySubmission(int submissionId) = 0;
    virtual std::vector<AuditEntry> findByDepartment(int departmentId) = 0;
    virtual std::vector<AuditEntry> findAll() = 0;
};
