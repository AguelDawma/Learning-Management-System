# Building and running MLMS

MLMS is a C++17 desktop application (Qt 6 Widgets) that stores its data in a
local SQLite database. It works fully offline.

## Requirements

| Tool / library | Tested version | Windows (MSYS2 **MINGW64**) package | Ubuntu 24.04 package |
|---|---|---|---|
| C++17 compiler | GCC 13-16 | `mingw-w64-x86_64-gcc` | `g++` |
| CMake + Ninja | CMake 3.28+ | `mingw-w64-x86_64-cmake`, `mingw-w64-x86_64-ninja` | `cmake ninja-build` |
| pkg-config | | `mingw-w64-x86_64-pkgconf` | `pkg-config` |
| SQLite 3 | 3.45+ | `mingw-w64-x86_64-sqlite3` | `libsqlite3-dev` |
| libargon2 | 20190702+ | `mingw-w64-x86_64-argon2` | `libargon2-dev` |
| Qt 6 Widgets | 6.4+ | `mingw-w64-x86_64-qt6-base` | `qt6-base-dev` |
| GoogleTest | 1.14+ | optional; downloaded automatically if missing | `libgtest-dev` |

On Windows, use the **MSYS2 MINGW64** shell (or put `C:\msys64\mingw64\bin` first on `PATH`).

## Build and test

```bash
cd "Software/Phase 4/mlms"
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure      # 94 tests: unit, integration, GUI smoke
```

CMake options:

| Option | Default | Meaning |
|---|---|---|
| `MLMS_BUILD_GUI` | `ON` | Build the Qt application (`OFF` builds only the core library and tests) |
| `MLMS_BUILD_TESTS` | `ON` | Build the GoogleTest suite |
| `MLMS_WARNINGS_AS_ERRORS` | `ON` | `-Werror` on project code |
| `MLMS_COVERAGE` | `OFF` | gcov instrumentation; used by `scripts/coverage.sh` |

Line coverage of the business logic: `bash scripts/coverage.sh` (writes `build-cov/coverage.txt`).

## Running

```bash
build/src/mlms                 # Linux
build\src\mlms.exe             # Windows
build/src/mlms --demo          # first run with demo accounts and courses
build/src/mlms --data D:/MLMS  # choose where the database and submitted files live
```

Data folder: `--data <folder>`, else the `MLMS_DATA_DIR` environment variable, else
`data/` next to the executable (or the user's AppData folder if that is not writable).
It contains `mlms.db` and `submissions/`. Back up this folder to back up the system.

**First start:** an `admin` account is created and its temporary password is shown once.
The administrator must choose a new password at first sign-in.

**Demo data (`--demo`, empty database only):** users `admin`, `hod1`, `lecturer1`,
`student1`-`student3`, all with password `Password1`.

## Layout

```
sql/schema.sql            database schema (embedded into the program at build time)
src/domain/               entities, rules, state machine, penalty strategy (no dependencies)
src/data/                 SQLite repositories, Database/Transaction, file store
src/infrastructure/       Argon2id hasher, system clock, SHA-256
src/services/             use cases, role checks, transactions, audit observer
src/ui/                   Qt screens (call services only)
tests/unit, integration   GoogleTest suites;  tests/gui: offscreen GUI smoke tests
```
