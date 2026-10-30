#pragma once

#include <stdexcept>
#include <string>

// Exception hierarchy shared by every layer. The UI catches std::exception
// around each user action so that no error crashes the program.
class MlmsError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Input that breaks a validation rule (bad mark, short feedback, blank reason).
class ValidationError : public MlmsError {
public:
    using MlmsError::MlmsError;
};

// The actor's role or ownership does not allow the action (FR-02, BR-06).
class AccessDenied : public MlmsError {
public:
    using MlmsError::MlmsError;
};

// A Submission state change that the life cycle does not allow (FR-15).
class InvalidTransition : public MlmsError {
public:
    using MlmsError::MlmsError;
};

// A submission the rules refuse, e.g. a late replacement of an on-time version (BR-03).
class SubmissionRefused : public MlmsError {
public:
    using MlmsError::MlmsError;
};

class NotFound : public MlmsError {
public:
    using MlmsError::MlmsError;
};

// Another user changed the mark since it was loaded (row_version mismatch).
class ConcurrencyError : public MlmsError {
public:
    using MlmsError::MlmsError;
};

class AuthenticationError : public MlmsError {
public:
    using MlmsError::MlmsError;
};

class DatabaseError : public MlmsError {
public:
    using MlmsError::MlmsError;
};

class FileStoreError : public MlmsError {
public:
    using MlmsError::MlmsError;
};
