# Moderated Learning Management System (MLMS)

A desktop Learning Management System for a secondary school, built in C++17 with Qt 6 and SQLite.
It manages courses, assignments, submissions and marks, and makes sure **no mark reaches a student
until the Head of Department has checked and released it**. Every change to a mark is recorded.

> CS4433 Software Engineering, National University of Lesotho. Individual final project by
> **Sekhonyana T.P. (202321146)**. Developed with a Waterfall life cycle and V-Model verification.

![C++17](https://img.shields.io/badge/C%2B%2B-17-blue) ![Qt 6](https://img.shields.io/badge/Qt-6-green)
![SQLite](https://img.shields.io/badge/SQLite-3-lightgrey) ![Tests](https://img.shields.io/badge/tests-94%20passing-brightgreen)
![Coverage](https://img.shields.io/badge/business%20logic%20coverage-95%25-brightgreen)

---

## Contents

- [Features](#features)
- [Business rules](#business-rules)
- [Quick start](#quick-start)
- [Building from source](#building-from-source)
- [Testing](#testing)
- [Architecture](#architecture)
- [Security](#security)
- [Project structure](#project-structure)
- [Development workflow](#development-workflow)
- [Roadmap](#roadmap)

---

## Features

Each user signs in with a username and password and sees only the screens for their role.

| Role | What they can do |
|---|---|
| **Student** | See enrolled courses and published assignments (with any extension shown), submit work, resubmit before the deadline, get a submission receipt with a SHA-256 checksum, and view released marks, late penalties and written feedback. |
| **Lecturer** | Create and publish assignments with a deadline and maximum mark, grant an individual student an extension (reason required), open submitted files, and enter or edit marks and feedback. The marking screen shows the late penalty while the mark is typed. |
| **Head of Department** | Review every mark in their own department, approve marks, return a mark to the lecturer with a comment, adjust a mark with a reason, release approved marks to students, and view the audit log. |
| **Administrator** | Create and deactivate accounts, reset passwords (which also unlocks the account), create departments and assign their Head, create courses, assign lecturers and enrol students, and view the full audit log. |

The system runs **fully offline** on a single school computer or a shared folder on the local network.

## Business rules

These rules resolve the conflicts found between the stakeholder statements during requirements engineering.

| ID | Rule |
|---|---|
| BR-01 | A student's effective deadline is their extension deadline if they have one, otherwise the assignment deadline. |
| BR-02 | Late work loses **10% of the maximum mark per day** (one second late counts as one day) for up to **3 days**. After that it receives **zero**. The final mark is never negative. Late work is never rejected. |
| BR-03 | Students may resubmit as often as they like before the deadline. Each upload is a new version and the newest is marked. After the deadline, an on-time version cannot be replaced by a late one. |
| BR-06 | A Head of Department can only moderate courses in their own department. |
| Release | Lecturers cannot release marks. Students see a mark only after the Head of Department releases it. |
| Audit | Every change to a mark or submission state records who, when, the old value, the new value and why. The audit log cannot be edited or deleted. |

Each submission follows this life cycle. Any other change is rejected.

```
 on time ──► SUBMITTED ──mark──► MARKED ──approve──► MODERATED ──release──► RELEASED
                 ▲                ▲   │
       extension │                │   │ return (with comment)
                 │      mark      │   ▼
 late ─────► LATE ────────────────┘  RETURNED ──re-mark (with reason)──► MARKED
```

Marks can still change after marking, always with a reason that goes into the audit log. The
lecturer can edit while the state is MARKED. The Head of Department can adjust in MODERATED and
correct in RELEASED.

## Quick start

On Windows, after [building](#building-from-source):

```bat
cd "Software\Phase 4\mlms"
build\src\mlms.exe --demo --data demo-data
```

`--demo` fills an empty database with sample data. Every demo account uses the password **`Password1`**:

| Username | Role |
|---|---|
| `admin` | Administrator |
| `hod1` | Head of Department (Computer Science) |
| `lecturer1` | Lecturer (CS101, CS102) |
| `student1`, `student2`, `student3` | Students enrolled in both courses |

**A typical walkthrough:** `student1` submits work, then `lecturer1` marks it, then `hod1` approves and
releases it, and finally `student1` sees the mark and feedback.

### Command-line options

| Option | Meaning |
|---|---|
| `--data <folder>` | Where the database (`mlms.db`) and submitted files (`submissions/`) are kept. |
| `--demo` | Add demo accounts and courses. Only works on an empty database. |
| `--help`, `--version` | Show help or the version. |

If `--data` is not given, the program uses the `MLMS_DATA_DIR` environment variable, then a `data`
folder next to `mlms.exe`. If that folder is not writable, it falls back to the user's AppData folder.
Back up the data folder to back up the whole system.

**First start without `--demo`:** the program creates an `admin` account and shows its temporary
password once. The administrator must choose a new password at first sign-in.

## Building from source

### Requirements

| Component | Version | Windows (MSYS2 **MINGW64**) | Ubuntu 24.04 |
|---|---|---|---|
| C++17 compiler | GCC 13+ | `mingw-w64-x86_64-gcc` | `g++` |
| CMake, Ninja | CMake 3.21+ | `mingw-w64-x86_64-cmake` `mingw-w64-x86_64-ninja` | `cmake ninja-build` |
| pkg-config | | `mingw-w64-x86_64-pkgconf` | `pkg-config` |
| SQLite 3 | 3.35+ | `mingw-w64-x86_64-sqlite3` | `libsqlite3-dev` |
| libargon2 | 20190702+ | `mingw-w64-x86_64-argon2` | `libargon2-dev` |
| Qt 6 Widgets | 6.4+ | `mingw-w64-x86_64-qt6-base` | `qt6-base-dev` |
| GoogleTest | 1.14+ | downloaded automatically if missing | `libgtest-dev` |

On Windows, install the packages with `pacman -S <package>` in the MSYS2 shell. Then use the MSYS2
**MINGW64** shell, or put `C:\msys64\mingw64\bin` first on your `PATH` while building.

### Build

```bash
cd "Software/Phase 4/mlms"
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

On Windows, the build copies Qt, its plugins and every other required DLL next to `build/src/mlms.exe`.
That folder then runs on any Windows PC, by double-click or from any Command Prompt, without MSYS2.

| CMake option | Default | Meaning |
|---|---|---|
| `MLMS_BUILD_GUI` | `ON` | Build the Qt application. `OFF` builds only the core library and tests. |
| `MLMS_BUILD_TESTS` | `ON` | Build the GoogleTest suites. |
| `MLMS_WARNINGS_AS_ERRORS` | `ON` | Build project code with `-Werror`. |
| `MLMS_COVERAGE` | `OFF` | Add gcov instrumentation (used by `scripts/coverage.sh`). |

More detail: [docs/BUILD.md](Software/Phase%204/mlms/docs/BUILD.md).

## Testing

```bash
ctest --test-dir build --output-on-failure
```

There are **94 tests**. They need no display or network.

| Level | What is tested | Where |
|---|---|---|
| Unit | Dates, the late-penalty strategy, the state machine, validation, roles, SHA-256, Argon2id, the file store. Includes boundary-value tests for deadlines and penalties. | `tests/unit/` |
| Integration | Services against an in-memory SQLite database: login and lockout, administration, submission paths, the full mark-to-release workflow, transactions, audit, concurrency. | `tests/integration/` |
| System (GUI smoke) | Signs in through the real login window and opens every role's screens on Qt's offscreen platform. | `tests/gui/` |

Line coverage of the business-logic layer (domain and services) is **95.2%**. To measure it:

```bash
bash scripts/coverage.sh        # writes build-cov/coverage.txt
```

## Architecture

MLMS uses a strict **layered architecture**. Each layer calls only the layer below it, through
interfaces. The core (everything except the UI) has no Qt dependency, so it is tested without a GUI.

```
┌─────────────────────────────────────────────────────────────┐
│ Presentation    Qt Widgets: Login, Student, Lecturer, HoD,  │  src/ui
│                 Administrator views. No SQL, no rules.      │
├─────────────────────────────────────────────────────────────┤
│ Application     AuthService, AdminService, AssignmentService│  src/services
│                 SubmissionService, MarkingService,          │
│                 ModerationService, AuditService             │
├──────────────────────────────┬──────────────────────────────┤
│ Data access                  │ Infrastructure               │  src/data
│ SQLite repositories,         │ Argon2id hasher, system      │  src/infrastructure
│ Database/Transaction, files  │ clock, SHA-256               │
├──────────────────────────────┴──────────────────────────────┤
│ Domain          User hierarchy, Assignment, Submission,     │  src/domain
│                 Mark, penalty policies. Depends on nothing. │
└─────────────────────────────────────────────────────────────┘
```

| Design pattern | Where | Why |
|---|---|---|
| **Strategy** | `ILatePenaltyPolicy`, `TenPercentPerDayPolicy`, `NoPenaltyPolicy` | The penalty rule is the most likely to change. A new rule is a new class. |
| **Repository** | `IUserRepository`, `ISubmissionRepository`, ... and their SQLite classes | Keeps all SQL in one layer and lets services be tested with an in-memory database. |
| **Factory** | `UserFactory` | Builds the right `User` subclass from the stored role, so no other code switches on role. |
| **Observer** | `IMarkObserver`, `AuditLogger` | Services announce every mark change; the audit row is written in the same transaction. |

Concrete classes are chosen only in the composition root, `AppContext` (dependency inversion).
Each use case that writes data runs in one transaction, so it either fully succeeds or changes nothing.

## Security

- **Passwords** are stored only as **Argon2id** hashes, each with a random 16-byte salt, using the
  reference `libargon2` (RFC 9106). Settings follow the OWASP minimum: 19 MiB memory, 2 iterations.
- **Account lockout:** 5 failed sign-ins lock an account for 15 minutes. A password set by the
  administrator must be changed at the next sign-in.
- **SQL injection:** every query uses bound parameters.
- **Role checks** are made in the service layer on every action, not just by hiding buttons.
- **Integrity:** the audit log is append-only (enforced by database triggers). Every submitted file
  gets a SHA-256 checksum, and older versions are never overwritten.
- **Error handling:** invalid input and database errors raise exceptions that the UI shows as
  messages. They never crash the program.

## Project structure

```
Software/Phase 4/mlms/
├── CMakeLists.txt           build definition
├── sql/schema.sql           database schema (embedded into the program at build time)
├── src/
│   ├── domain/              entities, business rules, state machine, penalty strategy
│   ├── data/                Database, repositories, file store
│   ├── infrastructure/      Argon2id hasher, clock, SHA-256
│   ├── services/            use cases, role checks, audit observer, AppContext
│   ├── ui/                  Qt windows, dialogs and role views
│   └── main.cpp             entry point and sign-in flow
├── tests/                   unit/, integration/, gui/
├── scripts/coverage.sh      coverage report
├── cmake/                   Windows DLL deployment helper
└── docs/                    BUILD.md, CODING_STANDARD.md
.github/workflows/ci.yml     continuous integration
```

## Development workflow

- **Branches:** each feature is developed on its own branch and merged into `main` through a pull request.
- **Continuous integration:** GitHub Actions runs on every push and pull request. It builds on Ubuntu,
  runs all tests, runs cppcheck static analysis and measures coverage.
- **Coding standard:** see [docs/CODING_STANDARD.md](Software/Phase%204/mlms/docs/CODING_STANDARD.md).
  Code builds with `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror`.
- **Releases** are tagged in Git (`v1.0`, `v1.1`).

## Roadmap

| Release | Content |
|---|---|
| v1.0 | All four roles, moderation workflow, late penalties, individual extensions, audit log. |
| v1.1 | Change request CR1: a lecturer can extend the deadline for a whole class, and extensions are shown on the student's assignment list. |

Out of scope: online payment, video conferencing, automated grading, plagiarism detection, and web or mobile interfaces.
