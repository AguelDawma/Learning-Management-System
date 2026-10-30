# MLMS coding standard

Based on the C++ Core Guidelines, adapted to this project. The compiler
(`-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror`), cppcheck and code
review enforce it.

## Layout and naming
- 4-space indentation, 120-column limit, braces on the same line.
- Types and enums: `PascalCase`. Functions and variables: `camelCase`.
  Private members end with `_`. Constants: `kPascalCase`.
- Interfaces start with `I` (`ISubmissionRepository`) and contain only pure
  virtual functions and a virtual destructor.
- One main class per file pair (`Name.hpp` / `Name.cpp`); headers use `#pragma once`.

## Architecture rules
- Layers call downwards only: `ui` -> `services` -> `data` / `infrastructure` -> `domain`.
  `domain` depends on nothing but the standard library.
- The UI never contains SQL or business rules; it calls services and shows their results.
- Services receive dependencies as interfaces through their constructors.
  Concrete classes are chosen only in `AppContext` / `main()`.
- Each use case that writes runs in one transaction (`ITransactionManager::begin()`).

## Safety and security
- SQL only through `Statement::bind`; never build SQL from user input.
- No raw `new`/`delete` outside Qt parent-owned widgets; use RAII (`std::unique_ptr`,
  `Statement`, `Transaction`).
- Errors are reported with exceptions from `domain/Errors.hpp`; every UI action runs
  inside `ui::guarded()` so no exception reaches the event loop.
- Validate all input in the domain or service layer, even if the UI also checks it.
- Passwords only as Argon2id hashes; never log or display them.
- Times are UTC `DateTime` from `IClock`; never take a submission time from the user.

## Comments and tests
- Comment *why*, and reference the requirement or rule (`FR-12`, `BR-02`) where one applies.
- Every business rule has a unit or integration test; boundary values are tested explicitly.
- A change is complete when the build has no warnings and `ctest` passes.
