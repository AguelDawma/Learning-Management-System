#pragma once

#include <optional>
#include <string>

struct Department {
    int id = 0;
    std::string name;
    std::optional<int> hodId;
};

struct Course {
    int id = 0;
    std::string code;
    std::string title;
    int departmentId = 0;
};
