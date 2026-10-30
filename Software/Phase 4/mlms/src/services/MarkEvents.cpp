#include "MarkEvents.hpp"

#include <cstdio>

std::string formatMark(double value) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.2f", value);
    std::string s = buf;
    while (!s.empty() && s.back() == '0') s.pop_back();
    if (!s.empty() && s.back() == '.') s.pop_back();
    return s;
}
