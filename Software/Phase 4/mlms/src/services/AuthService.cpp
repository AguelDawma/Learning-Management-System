#include "AuthService.hpp"
#include "../domain/Errors.hpp"
#include "../domain/Validation.hpp"

AuthService::AuthService(IUserRepository& users, IPasswordHasher& hasher, IClock& clock)
    : users_(users), hasher_(hasher), clock_(clock), dummyHash_(hasher.hash("not-a-real-password-1")) {}

LoginResult AuthService::login(const std::string& username, const std::string& password) {
    const std::string kInvalid = "Invalid username or password";
    auto record = users_.findByUsername(trim(username));
    if (!record) {
        // Spend the same time as a real check so response time does not reveal valid usernames.
        hasher_.verify(password, dummyHash_);
        throw AuthenticationError(kInvalid);
    }
    const DateTime now = clock_.now();
    if (record->lockedUntil && *record->lockedUntil > now) {
        throw AuthenticationError("This account is locked after too many failed attempts. Try again after " +
                                  record->lockedUntil->toIso() + " (UTC) or ask the administrator.");
    }
    if (!hasher_.verify(password, record->passwordHash)) {
        const int attempts = record->failedAttempts + 1;
        if (attempts >= kMaxFailedAttempts) {
            users_.updateLoginState(record->id, 0, now.plusSeconds(kLockoutMinutes * 60));
            throw AuthenticationError("Too many failed attempts. The account is locked for " +
                                      std::to_string(kLockoutMinutes) + " minutes.");
        }
        users_.updateLoginState(record->id, attempts, std::nullopt);
        throw AuthenticationError(kInvalid);
    }
    if (!record->active) {
        throw AuthenticationError("This account has been deactivated. Please contact the administrator.");
    }
    if (record->failedAttempts != 0 || record->lockedUntil) {
        users_.updateLoginState(record->id, 0, std::nullopt);
    }
    LoginResult result;
    result.user = UserFactory::create(record->role, *record);
    result.mustChangePassword = record->mustChangePassword;
    return result;
}

void AuthService::changePassword(const User& user, const std::string& currentPassword,
                                 const std::string& newPassword) {
    auto record = users_.findById(user.id());
    if (!record) throw NotFound("User not found");
    if (!hasher_.verify(currentPassword, record->passwordHash)) {
        throw ValidationError("The current password is not correct");
    }
    if (newPassword == currentPassword) {
        throw ValidationError("The new password must be different from the current one");
    }
    validatePasswordStrength(newPassword);
    users_.updatePassword(user.id(), hasher_.hash(newPassword), false);
}
