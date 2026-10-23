PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;

CREATE TABLE app_user (
  user_id              INTEGER PRIMARY KEY,
  username             TEXT    NOT NULL UNIQUE,
  full_name            TEXT    NOT NULL,
  role                 TEXT    NOT NULL CHECK (role IN ('STUDENT','LECTURER','HOD','ADMINISTRATOR')),
  password_hash        TEXT    NOT NULL,
  is_active            INTEGER NOT NULL DEFAULT 1 CHECK (is_active IN (0,1)),
  failed_attempts      INTEGER NOT NULL DEFAULT 0 CHECK (failed_attempts >= 0),
  locked_until         TEXT,
  must_change_password INTEGER NOT NULL DEFAULT 0 CHECK (must_change_password IN (0,1))
);

CREATE TABLE department (
  department_id INTEGER PRIMARY KEY,
  name          TEXT    NOT NULL UNIQUE,
  hod_id        INTEGER UNIQUE REFERENCES app_user(user_id)
);

CREATE TABLE course (
  course_id     INTEGER PRIMARY KEY,
  code          TEXT    NOT NULL UNIQUE,
  title         TEXT    NOT NULL,
  department_id INTEGER NOT NULL REFERENCES department(department_id)
);

CREATE TABLE course_lecturer (
  course_id   INTEGER NOT NULL REFERENCES course(course_id) ON DELETE CASCADE,
  lecturer_id INTEGER NOT NULL REFERENCES app_user(user_id),
  PRIMARY KEY (course_id, lecturer_id)
);

CREATE TABLE enrolment (
  course_id   INTEGER NOT NULL REFERENCES course(course_id),
  student_id  INTEGER NOT NULL REFERENCES app_user(user_id),
  enrolled_at TEXT    NOT NULL,
  PRIMARY KEY (course_id, student_id)
);

CREATE TABLE assignment (
  assignment_id INTEGER PRIMARY KEY,
  course_id     INTEGER NOT NULL REFERENCES course(course_id),
  title         TEXT    NOT NULL,
  description   TEXT,
  deadline      TEXT    NOT NULL,
  max_mark      INTEGER NOT NULL CHECK (max_mark BETWEEN 1 AND 1000),
  is_published  INTEGER NOT NULL DEFAULT 0 CHECK (is_published IN (0,1)),
  created_by    INTEGER NOT NULL REFERENCES app_user(user_id)
);

CREATE TABLE extension (
  assignment_id INTEGER NOT NULL REFERENCES assignment(assignment_id),
  student_id    INTEGER NOT NULL REFERENCES app_user(user_id),
  new_deadline  TEXT    NOT NULL,
  reason        TEXT    NOT NULL CHECK (length(trim(reason)) > 0),
  granted_by    INTEGER NOT NULL REFERENCES app_user(user_id),
  granted_at    TEXT    NOT NULL,
  PRIMARY KEY (assignment_id, student_id)
);

CREATE TABLE submission (
  submission_id INTEGER PRIMARY KEY,
  assignment_id INTEGER NOT NULL REFERENCES assignment(assignment_id),
  student_id    INTEGER NOT NULL REFERENCES app_user(user_id),
  version       INTEGER NOT NULL CHECK (version >= 1),
  file_path     TEXT    NOT NULL,
  checksum      TEXT    NOT NULL CHECK (length(checksum) = 64),
  submitted_at  TEXT    NOT NULL,
  state         TEXT    NOT NULL CHECK (state IN
                ('SUBMITTED','LATE','MARKED','RETURNED','MODERATED','RELEASED')),
  UNIQUE (assignment_id, student_id, version)
);

CREATE TABLE mark (
  submission_id  INTEGER PRIMARY KEY REFERENCES submission(submission_id),
  raw_mark       REAL    NOT NULL CHECK (raw_mark >= 0),
  feedback       TEXT    NOT NULL CHECK (length(feedback) >= 10),
  marked_by      INTEGER NOT NULL REFERENCES app_user(user_id),
  marked_at      TEXT    NOT NULL,
  moderated_by   INTEGER REFERENCES app_user(user_id),
  released_at    TEXT,
  return_comment TEXT,
  row_version    INTEGER NOT NULL DEFAULT 1
);

CREATE TABLE audit_log (
  audit_id      INTEGER PRIMARY KEY,
  occurred_at   TEXT    NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ','now')),
  user_id       INTEGER NOT NULL REFERENCES app_user(user_id),
  submission_id INTEGER NOT NULL REFERENCES submission(submission_id),
  field         TEXT    NOT NULL,
  old_value     TEXT,
  new_value     TEXT    NOT NULL,
  reason        TEXT    NOT NULL
);

CREATE TRIGGER audit_no_update BEFORE UPDATE ON audit_log
BEGIN SELECT RAISE(ABORT, 'audit_log is append-only'); END;
CREATE TRIGGER audit_no_delete BEFORE DELETE ON audit_log
BEGIN SELECT RAISE(ABORT, 'audit_log is append-only'); END;

CREATE VIEW active_submission AS
SELECT s.* FROM submission s
WHERE s.version = (SELECT MAX(version) FROM submission x
                   WHERE x.assignment_id = s.assignment_id AND x.student_id = s.student_id);

CREATE INDEX idx_submission_assignment ON submission(assignment_id, student_id);
CREATE INDEX idx_audit_submission ON audit_log(submission_id);