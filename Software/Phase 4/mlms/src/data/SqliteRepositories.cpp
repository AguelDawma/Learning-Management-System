#include "SqliteRepositories.hpp"
#include "../domain/Errors.hpp"

namespace {

// ---------------------------------------------------------------- row mappers

const char* const kUserColumns =
    "SELECT u.user_id, u.username, u.full_name, u.role, u.password_hash, u.is_active, u.failed_attempts, "
    "u.locked_until, u.must_change_password, d.department_id "
    "FROM app_user u LEFT JOIN department d ON d.hod_id = u.user_id ";

UserRecord readUser(const Statement& st) {
    UserRecord r;
    r.id = st.getInt(0);
    r.username = st.getText(1);
    r.fullName = st.getText(2);
    r.role = roleFromString(st.getText(3));
    r.passwordHash = st.getText(4);
    r.active = st.getInt(5) != 0;
    r.failedAttempts = st.getInt(6);
    r.lockedUntil = st.getOptDateTime(7);
    r.mustChangePassword = st.getInt(8) != 0;
    r.departmentId = st.getOptInt(9).value_or(0);
    return r;
}

std::vector<UserRecord> readUsers(Statement& st) {
    std::vector<UserRecord> out;
    while (st.step()) out.push_back(readUser(st));
    return out;
}

const char* const kCourseColumns = "SELECT c.course_id, c.code, c.title, c.department_id FROM course c ";

Course readCourse(const Statement& st) { return Course{st.getInt(0), st.getText(1), st.getText(2), st.getInt(3)}; }

std::vector<Course> readCourses(Statement& st) {
    std::vector<Course> out;
    while (st.step()) out.push_back(readCourse(st));
    return out;
}

const char* const kAssignmentColumns =
    "SELECT assignment_id, course_id, title, description, deadline, max_mark, is_published, created_by "
    "FROM assignment ";

Assignment readAssignment(const Statement& st) {
    return Assignment(st.getInt(0), st.getInt(1), st.getText(2), st.getText(3), st.getDateTime(4), st.getInt(5),
                      st.getInt(6) != 0, st.getInt(7));
}

const char* const kExtensionColumns =
    "SELECT assignment_id, student_id, new_deadline, reason, granted_by, granted_at FROM extension ";

Extension readExtension(const Statement& st) {
    return Extension{st.getInt(0), st.getInt(1), st.getDateTime(2), st.getText(3), st.getInt(4), st.getDateTime(5)};
}

const char* const kSubmissionColumns =
    "SELECT submission_id, assignment_id, student_id, version, file_path, checksum, submitted_at, state ";

Submission readSubmission(const Statement& st) {
    return Submission(st.getInt(0), st.getInt(1), st.getInt(2), st.getInt(3), st.getText(4), st.getText(5),
                      st.getDateTime(6), submissionStateFromString(st.getText(7)));
}

std::vector<Submission> readSubmissions(Statement& st) {
    std::vector<Submission> out;
    while (st.step()) out.push_back(readSubmission(st));
    return out;
}

const char* const kAuditColumns =
    "SELECT l.audit_id, l.user_id, l.submission_id, l.field, l.old_value, l.new_value, l.reason, l.occurred_at "
    "FROM audit_log l ";

AuditEntry readAudit(const Statement& st) {
    AuditEntry e;
    e.id = st.getInt(0);
    e.userId = st.getInt(1);
    e.submissionId = st.getInt(2);
    e.field = st.getText(3);
    e.oldValue = st.getOptText(4);
    e.newValue = st.getText(5);
    e.reason = st.getText(6);
    e.timestamp = st.getDateTime(7);
    return e;
}

std::vector<AuditEntry> readAudits(Statement& st) {
    std::vector<AuditEntry> out;
    while (st.step()) out.push_back(readAudit(st));
    return out;
}

} // namespace

// ---------------------------------------------------------------- users

std::optional<UserRecord> SqliteUserRepository::findByUsername(const std::string& username) {
    auto st = db_.prepare(std::string(kUserColumns) + "WHERE u.username = ?");
    st.bind(1, username);
    if (!st.step()) return std::nullopt;
    return readUser(st);
}

std::optional<UserRecord> SqliteUserRepository::findById(int id) {
    auto st = db_.prepare(std::string(kUserColumns) + "WHERE u.user_id = ?");
    st.bind(1, id);
    if (!st.step()) return std::nullopt;
    return readUser(st);
}

std::vector<UserRecord> SqliteUserRepository::listAll() {
    auto st = db_.prepare(std::string(kUserColumns) + "ORDER BY u.role, u.full_name");
    return readUsers(st);
}

int SqliteUserRepository::add(const UserRecord& u) {
    auto st = db_.prepare(
        "INSERT INTO app_user (username, full_name, role, password_hash, is_active, must_change_password) "
        "VALUES (?, ?, ?, ?, ?, ?)");
    st.bind(1, u.username).bind(2, u.fullName).bind(3, toString(u.role)).bind(4, u.passwordHash);
    st.bind(5, u.active).bind(6, u.mustChangePassword);
    st.run();
    return static_cast<int>(db_.lastInsertId());
}

void SqliteUserRepository::updatePassword(int id, const std::string& hash, bool mustChange) {
    auto st = db_.prepare(
        "UPDATE app_user SET password_hash = ?, must_change_password = ?, failed_attempts = 0, "
        "locked_until = NULL WHERE user_id = ?");
    st.bind(1, hash).bind(2, mustChange).bind(3, id);
    st.run();
}

void SqliteUserRepository::updateLoginState(int id, int failedAttempts, std::optional<DateTime> lockedUntil) {
    auto st = db_.prepare("UPDATE app_user SET failed_attempts = ?, locked_until = ? WHERE user_id = ?");
    st.bind(1, failedAttempts).bind(2, lockedUntil).bind(3, id);
    st.run();
}

void SqliteUserRepository::setActive(int id, bool active) {
    auto st = db_.prepare("UPDATE app_user SET is_active = ? WHERE user_id = ?");
    st.bind(1, active).bind(2, id);
    st.run();
}

// ---------------------------------------------------------------- courses

int SqliteCourseRepository::addDepartment(const std::string& name) {
    auto st = db_.prepare("INSERT INTO department (name) VALUES (?)");
    st.bind(1, name);
    st.run();
    return static_cast<int>(db_.lastInsertId());
}

void SqliteCourseRepository::setHod(int departmentId, std::optional<int> hodId) {
    auto st = db_.prepare("UPDATE department SET hod_id = ? WHERE department_id = ?");
    st.bind(1, hodId).bind(2, departmentId);
    st.run();
}

std::vector<Department> SqliteCourseRepository::listDepartments() {
    auto st = db_.prepare("SELECT department_id, name, hod_id FROM department ORDER BY name");
    std::vector<Department> out;
    while (st.step()) out.push_back(Department{st.getInt(0), st.getText(1), st.getOptInt(2)});
    return out;
}

std::optional<Department> SqliteCourseRepository::findDepartment(int id) {
    auto st = db_.prepare("SELECT department_id, name, hod_id FROM department WHERE department_id = ?");
    st.bind(1, id);
    if (!st.step()) return std::nullopt;
    return Department{st.getInt(0), st.getText(1), st.getOptInt(2)};
}

int SqliteCourseRepository::addCourse(const Course& c) {
    auto st = db_.prepare("INSERT INTO course (code, title, department_id) VALUES (?, ?, ?)");
    st.bind(1, c.code).bind(2, c.title).bind(3, c.departmentId);
    st.run();
    return static_cast<int>(db_.lastInsertId());
}

std::optional<Course> SqliteCourseRepository::findCourse(int id) {
    auto st = db_.prepare(std::string(kCourseColumns) + "WHERE c.course_id = ?");
    st.bind(1, id);
    if (!st.step()) return std::nullopt;
    return readCourse(st);
}

std::vector<Course> SqliteCourseRepository::listCourses() {
    auto st = db_.prepare(std::string(kCourseColumns) + "ORDER BY c.code");
    return readCourses(st);
}

std::vector<Course> SqliteCourseRepository::coursesForLecturer(int lecturerId) {
    auto st = db_.prepare(std::string(kCourseColumns) +
                          "JOIN course_lecturer cl ON cl.course_id = c.course_id WHERE cl.lecturer_id = ? "
                          "ORDER BY c.code");
    st.bind(1, lecturerId);
    return readCourses(st);
}

std::vector<Course> SqliteCourseRepository::coursesForStudent(int studentId) {
    auto st = db_.prepare(std::string(kCourseColumns) +
                          "JOIN enrolment e ON e.course_id = c.course_id WHERE e.student_id = ? ORDER BY c.code");
    st.bind(1, studentId);
    return readCourses(st);
}

std::vector<Course> SqliteCourseRepository::coursesForDepartment(int departmentId) {
    auto st = db_.prepare(std::string(kCourseColumns) + "WHERE c.department_id = ? ORDER BY c.code");
    st.bind(1, departmentId);
    return readCourses(st);
}

void SqliteCourseRepository::assignLecturer(int courseId, int lecturerId) {
    auto st = db_.prepare("INSERT OR IGNORE INTO course_lecturer (course_id, lecturer_id) VALUES (?, ?)");
    st.bind(1, courseId).bind(2, lecturerId);
    st.run();
}

void SqliteCourseRepository::removeLecturer(int courseId, int lecturerId) {
    auto st = db_.prepare("DELETE FROM course_lecturer WHERE course_id = ? AND lecturer_id = ?");
    st.bind(1, courseId).bind(2, lecturerId);
    st.run();
}

bool SqliteCourseRepository::isLecturerOf(int courseId, int lecturerId) {
    auto st = db_.prepare("SELECT 1 FROM course_lecturer WHERE course_id = ? AND lecturer_id = ?");
    st.bind(1, courseId).bind(2, lecturerId);
    return st.step();
}

std::vector<UserRecord> SqliteCourseRepository::lecturersOf(int courseId) {
    auto st = db_.prepare(std::string(kUserColumns) +
                          "JOIN course_lecturer cl ON cl.lecturer_id = u.user_id WHERE cl.course_id = ? "
                          "ORDER BY u.full_name");
    st.bind(1, courseId);
    return readUsers(st);
}

void SqliteCourseRepository::enrol(int courseId, int studentId, DateTime at) {
    auto st = db_.prepare("INSERT OR IGNORE INTO enrolment (course_id, student_id, enrolled_at) VALUES (?, ?, ?)");
    st.bind(1, courseId).bind(2, studentId).bind(3, at);
    st.run();
}

void SqliteCourseRepository::unenrol(int courseId, int studentId) {
    auto st = db_.prepare("DELETE FROM enrolment WHERE course_id = ? AND student_id = ?");
    st.bind(1, courseId).bind(2, studentId);
    st.run();
}

bool SqliteCourseRepository::isEnrolled(int courseId, int studentId) {
    auto st = db_.prepare("SELECT 1 FROM enrolment WHERE course_id = ? AND student_id = ?");
    st.bind(1, courseId).bind(2, studentId);
    return st.step();
}

std::vector<UserRecord> SqliteCourseRepository::studentsIn(int courseId) {
    auto st = db_.prepare(std::string(kUserColumns) +
                          "JOIN enrolment e ON e.student_id = u.user_id WHERE e.course_id = ? ORDER BY u.full_name");
    st.bind(1, courseId);
    return readUsers(st);
}

// ---------------------------------------------------------------- assignments

std::optional<Assignment> SqliteAssignmentRepository::find(int id) {
    auto st = db_.prepare(std::string(kAssignmentColumns) + "WHERE assignment_id = ?");
    st.bind(1, id);
    if (!st.step()) return std::nullopt;
    return readAssignment(st);
}

std::vector<Assignment> SqliteAssignmentRepository::findByCourse(int courseId) {
    auto st = db_.prepare(std::string(kAssignmentColumns) + "WHERE course_id = ? ORDER BY deadline");
    st.bind(1, courseId);
    std::vector<Assignment> out;
    while (st.step()) out.push_back(readAssignment(st));
    return out;
}

int SqliteAssignmentRepository::add(const Assignment& a) {
    auto st = db_.prepare(
        "INSERT INTO assignment (course_id, title, description, deadline, max_mark, is_published, created_by) "
        "VALUES (?, ?, ?, ?, ?, ?, ?)");
    st.bind(1, a.courseId()).bind(2, a.title()).bind(3, a.description()).bind(4, a.deadline());
    st.bind(5, a.maxMark()).bind(6, a.isPublished()).bind(7, a.createdBy());
    st.run();
    return static_cast<int>(db_.lastInsertId());
}

void SqliteAssignmentRepository::setPublished(int id, bool published) {
    auto st = db_.prepare("UPDATE assignment SET is_published = ? WHERE assignment_id = ?");
    st.bind(1, published).bind(2, id);
    st.run();
}

std::optional<Extension> SqliteAssignmentRepository::findExtension(int assignmentId, int studentId) {
    auto st = db_.prepare(std::string(kExtensionColumns) + "WHERE assignment_id = ? AND student_id = ?");
    st.bind(1, assignmentId).bind(2, studentId);
    if (!st.step()) return std::nullopt;
    return readExtension(st);
}

std::vector<Extension> SqliteAssignmentRepository::findExtensions(int assignmentId) {
    auto st = db_.prepare(std::string(kExtensionColumns) + "WHERE assignment_id = ?");
    st.bind(1, assignmentId);
    std::vector<Extension> out;
    while (st.step()) out.push_back(readExtension(st));
    return out;
}

void SqliteAssignmentRepository::saveExtension(const Extension& e) {
    auto st = db_.prepare(
        "INSERT INTO extension (assignment_id, student_id, new_deadline, reason, granted_by, granted_at) "
        "VALUES (?, ?, ?, ?, ?, ?) "
        "ON CONFLICT (assignment_id, student_id) DO UPDATE SET new_deadline = excluded.new_deadline, "
        "reason = excluded.reason, granted_by = excluded.granted_by, granted_at = excluded.granted_at");
    st.bind(1, e.assignmentId).bind(2, e.studentId).bind(3, e.newDeadline).bind(4, e.reason);
    st.bind(5, e.grantedBy).bind(6, e.grantedAt);
    st.run();
}

// ---------------------------------------------------------------- submissions

std::optional<Submission> SqliteSubmissionRepository::find(int id) {
    auto st = db_.prepare(std::string(kSubmissionColumns) + "FROM submission WHERE submission_id = ?");
    st.bind(1, id);
    if (!st.step()) return std::nullopt;
    return readSubmission(st);
}

std::optional<Submission> SqliteSubmissionRepository::findActive(int assignmentId, int studentId) {
    auto st = db_.prepare(std::string(kSubmissionColumns) +
                          "FROM active_submission WHERE assignment_id = ? AND student_id = ?");
    st.bind(1, assignmentId).bind(2, studentId);
    if (!st.step()) return std::nullopt;
    return readSubmission(st);
}

std::vector<Submission> SqliteSubmissionRepository::findActiveByAssignment(int assignmentId) {
    auto st = db_.prepare(std::string(kSubmissionColumns) +
                          "FROM active_submission WHERE assignment_id = ? ORDER BY submitted_at");
    st.bind(1, assignmentId);
    return readSubmissions(st);
}

std::vector<Submission> SqliteSubmissionRepository::findVersions(int assignmentId, int studentId) {
    auto st = db_.prepare(std::string(kSubmissionColumns) +
                          "FROM submission WHERE assignment_id = ? AND student_id = ? ORDER BY version");
    st.bind(1, assignmentId).bind(2, studentId);
    return readSubmissions(st);
}

int SqliteSubmissionRepository::add(const Submission& s) {
    auto st = db_.prepare(
        "INSERT INTO submission (assignment_id, student_id, version, file_path, checksum, submitted_at, state) "
        "VALUES (?, ?, ?, ?, ?, ?, ?)");
    st.bind(1, s.assignmentId()).bind(2, s.studentId()).bind(3, s.version()).bind(4, s.filePath());
    st.bind(5, s.checksum()).bind(6, s.submittedAt()).bind(7, toString(s.state()));
    st.run();
    return static_cast<int>(db_.lastInsertId());
}

void SqliteSubmissionRepository::updateState(int id, SubmissionState state) {
    auto st = db_.prepare("UPDATE submission SET state = ? WHERE submission_id = ?");
    st.bind(1, toString(state)).bind(2, id);
    st.run();
}

// ---------------------------------------------------------------- marks

std::optional<Mark> SqliteMarkRepository::find(int submissionId) {
    auto st = db_.prepare(
        "SELECT submission_id, raw_mark, feedback, marked_by, marked_at, moderated_by, released_at, "
        "return_comment, row_version FROM mark WHERE submission_id = ?");
    st.bind(1, submissionId);
    if (!st.step()) return std::nullopt;
    return Mark(st.getInt(0), st.getDouble(1), st.getText(2), st.getInt(3), st.getDateTime(4), st.getOptInt(5),
                st.getOptDateTime(6), st.getOptText(7), st.getInt(8));
}

void SqliteMarkRepository::insert(const Mark& m) {
    auto st = db_.prepare(
        "INSERT INTO mark (submission_id, raw_mark, feedback, marked_by, marked_at, moderated_by, released_at, "
        "return_comment, row_version) VALUES (?, ?, ?, ?, ?, ?, ?, ?, 1)");
    st.bind(1, m.submissionId()).bind(2, m.rawMark()).bind(3, m.feedback()).bind(4, m.markedBy());
    st.bind(5, m.markedAt()).bind(6, m.moderatedBy()).bind(7, m.releasedAt()).bind(8, m.returnComment());
    st.run();
}

int SqliteMarkRepository::update(const Mark& m) {
    auto st = db_.prepare(
        "UPDATE mark SET raw_mark = ?, feedback = ?, marked_by = ?, marked_at = ?, moderated_by = ?, "
        "released_at = ?, return_comment = ?, row_version = row_version + 1 "
        "WHERE submission_id = ? AND row_version = ?");
    st.bind(1, m.rawMark()).bind(2, m.feedback()).bind(3, m.markedBy()).bind(4, m.markedAt());
    st.bind(5, m.moderatedBy()).bind(6, m.releasedAt()).bind(7, m.returnComment());
    st.bind(8, m.submissionId()).bind(9, m.rowVersion());
    st.run();
    if (db_.changes() == 0) {
        throw ConcurrencyError("This mark was changed by someone else since you opened it. Reload and try again.");
    }
    return m.rowVersion() + 1;
}

// ---------------------------------------------------------------- audit

void SqliteAuditRepository::append(const AuditEntry& e) {
    auto st = db_.prepare(
        "INSERT INTO audit_log (occurred_at, user_id, submission_id, field, old_value, new_value, reason) "
        "VALUES (?, ?, ?, ?, ?, ?, ?)");
    st.bind(1, e.timestamp).bind(2, e.userId).bind(3, e.submissionId).bind(4, e.field);
    st.bind(5, e.oldValue).bind(6, e.newValue).bind(7, e.reason);
    st.run();
}

std::vector<AuditEntry> SqliteAuditRepository::findBySubmission(int submissionId) {
    auto st = db_.prepare(std::string(kAuditColumns) + "WHERE l.submission_id = ? ORDER BY l.audit_id");
    st.bind(1, submissionId);
    return readAudits(st);
}

std::vector<AuditEntry> SqliteAuditRepository::findByDepartment(int departmentId) {
    auto st = db_.prepare(std::string(kAuditColumns) +
                          "JOIN submission s ON s.submission_id = l.submission_id "
                          "JOIN assignment a ON a.assignment_id = s.assignment_id "
                          "JOIN course c ON c.course_id = a.course_id "
                          "WHERE c.department_id = ? ORDER BY l.audit_id DESC");
    st.bind(1, departmentId);
    return readAudits(st);
}

std::vector<AuditEntry> SqliteAuditRepository::findAll() {
    auto st = db_.prepare(std::string(kAuditColumns) + "ORDER BY l.audit_id DESC");
    return readAudits(st);
}
