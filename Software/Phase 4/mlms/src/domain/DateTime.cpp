#include "DateTime.hpp"
#include "Errors.hpp"

#include <cstdio>

namespace {

// Howard Hinnant's days_from_civil / civil_from_days (proleptic Gregorian).
std::int64_t daysFromCivil(std::int64_t y, unsigned m, unsigned d) {
    y -= m <= 2;
    const std::int64_t era = (y >= 0 ? y : y - 399) / 400;
    const auto yoe = static_cast<unsigned>(y - era * 400);
    const unsigned doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + static_cast<std::int64_t>(doe) - 719468;
}

void civilFromDays(std::int64_t z, int& year, unsigned& month, unsigned& day) {
    z += 719468;
    const std::int64_t era = (z >= 0 ? z : z - 146096) / 146097;
    const auto doe = static_cast<unsigned>(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp = (5 * doy + 2) / 153;
    day = doy - (153 * mp + 2) / 5 + 1;
    month = mp < 10 ? mp + 3 : mp - 9;
    year = static_cast<int>(static_cast<std::int64_t>(yoe) + era * 400 + (month <= 2));
}

bool isLeap(int y) { return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0; }

int daysInMonth(int y, int m) {
    static const int kDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return (m == 2 && isLeap(y)) ? 29 : kDays[m - 1];
}

bool allDigits(const std::string& s, std::size_t pos, std::size_t len) {
    if (pos + len > s.size()) return false;
    for (std::size_t i = pos; i < pos + len; ++i) {
        if (s[i] < '0' || s[i] > '9') return false;
    }
    return true;
}

int num(const std::string& s, std::size_t pos, std::size_t len) {
    return std::stoi(s.substr(pos, len));
}

[[noreturn]] void badFormat(const std::string& text) {
    throw ValidationError("Invalid date/time '" + text + "' (expected YYYY-MM-DDTHH:MM:SSZ)");
}

} // namespace

DateTime DateTime::fromCivil(int year, int month, int day, int hour, int minute, int second) {
    if (month < 1 || month > 12 || day < 1 || day > daysInMonth(year, month) || hour < 0 || hour > 23 ||
        minute < 0 || minute > 59 || second < 0 || second > 59) {
        throw ValidationError("Invalid date or time");
    }
    const std::int64_t days = daysFromCivil(year, static_cast<unsigned>(month), static_cast<unsigned>(day));
    return DateTime(days * kSecondsPerDay + hour * 3600 + minute * 60 + second);
}

DateTime DateTime::parseIso(const std::string& text) {
    if (text.size() < 10 || !allDigits(text, 0, 4) || text[4] != '-' || !allDigits(text, 5, 2) ||
        text[7] != '-' || !allDigits(text, 8, 2)) {
        badFormat(text);
    }
    const int y = num(text, 0, 4), mo = num(text, 5, 2), d = num(text, 8, 2);
    if (text.size() == 10) {
        return fromCivil(y, mo, d, 23, 59, 59);
    }
    if ((text[10] != 'T' && text[10] != ' ') || text.size() < 16 || !allDigits(text, 11, 2) ||
        text[13] != ':' || !allDigits(text, 14, 2)) {
        badFormat(text);
    }
    const int h = num(text, 11, 2), mi = num(text, 14, 2);
    int s = 0;
    std::size_t pos = 16;
    if (pos < text.size() && text[pos] == ':') {
        if (!allDigits(text, pos + 1, 2)) badFormat(text);
        s = num(text, pos + 1, 2);
        pos += 3;
    }
    if (pos < text.size() && text[pos] == 'Z') ++pos;
    if (pos != text.size()) badFormat(text);
    return fromCivil(y, mo, d, h, mi, s);
}

std::string DateTime::toIso() const {
    std::int64_t days = epochSeconds_ / kSecondsPerDay;
    std::int64_t secs = epochSeconds_ % kSecondsPerDay;
    if (secs < 0) {
        secs += kSecondsPerDay;
        --days;
    }
    int y = 0;
    unsigned m = 0, d = 0;
    civilFromDays(days, y, m, d);
    char buf[40];
    std::snprintf(buf, sizeof(buf), "%04d-%02u-%02uT%02d:%02d:%02dZ", y, m, d, static_cast<int>(secs / 3600),
                  static_cast<int>((secs % 3600) / 60), static_cast<int>(secs % 60));
    return buf;
}
