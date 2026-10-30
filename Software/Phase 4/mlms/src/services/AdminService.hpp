#pragma once

#include "../data/Database.hpp"
#include "../data/Repositories.hpp"
#include "../infrastructure/Infrastructure.hpp"

#include <optional>
#include <string>
#include <vector>

// Administrator use cases: user accounts, departments, courses, lecturer
// assignment and student enrolment.
class AdminService {
    IUserRepository& users_;
    ICourseRepository& courses_;
    IPasswordHasher& hasher_;
    IClock& clock_;
    ITransactionManager& tx_;

    UserRecord requireUser(int id, std::optional<Role> role);

public:
    AdminService(IUserRepository& users, ICourseRepository& courses, IPasswordHasher& hasher, IClock& clock,
                 ITransactionManager& tx)
        : users_(users), courses_(courses), hasher_(hasher), clock_(clock), tx_(tx) {}

    std::vector<UserRecord> listUsers(const User& admin);
    // New accounts must change the temporary password at first login (FR-04).
    int createUser(const User& admin, const std::string& username, const std::string& fullName, Role role,
                   const std::string& temporaryPassword);
    void resetPassword(const User& admin, int userId, const std::string& temporaryPassword);
    void setActive(const User& admin, int userId, bool active);

    std::vector<Department> listDepartments(const User& admin);
    int createDepartment(const User& admin, const std::string& name);
    void setHeadOfDepartment(const User& admin, int departmentId, std::optional<int> hodUserId);

    std::vector<Course> listCourses(const User& admin);
    int createCourse(const User& admin, const std::string& code, const std::string& title, int departmentId);
    void assignLecturer(const User& admin, int courseId, int lecturerId);
    void removeLecturer(const User& admin, int courseId, int lecturerId);
    std::vector<UserRecord> lecturersOf(const User& admin, int courseId);
    void enrolStudent(const User& admin, int courseId, int studentId);
    void unenrolStudent(const User& admin, int courseId, int studentId);
    std::vector<UserRecord> studentsIn(const User& admin, int courseId);
};
