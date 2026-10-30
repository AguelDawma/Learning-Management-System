#include "UserFactory.hpp"

std::unique_ptr<User> UserFactory::create(Role role, const UserRecord& r) {
    switch (role) {
        case Role::STUDENT:
            return std::make_unique<Student>(r.id, r.username, r.fullName, r.passwordHash, r.active);
        case Role::LECTURER:
            return std::make_unique<Lecturer>(r.id, r.username, r.fullName, r.passwordHash, r.active);
        case Role::HOD:
            return std::make_unique<HeadOfDepartment>(r.id, r.username, r.fullName, r.passwordHash, r.active,
                                                      r.departmentId);
        case Role::ADMINISTRATOR:
            return std::make_unique<Administrator>(r.id, r.username, r.fullName, r.passwordHash, r.active);
    }
    return nullptr;
}
