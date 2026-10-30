#pragma once

#include <cstdint>
#include <string>

// A UTC instant with one-second precision. Stored in the database as
// ISO 8601 text ("2026-11-07T23:59:00Z") so that text order equals time order.
class DateTime {
    std::int64_t epochSeconds_ = 0;

public:
    static constexpr std::int64_t kSecondsPerDay = 86400;

    constexpr DateTime() = default;
    constexpr explicit DateTime(std::int64_t epochSeconds) : epochSeconds_(epochSeconds) {}

    static DateTime fromCivil(int year, int month, int day, int hour = 0, int minute = 0, int second = 0);

    // Accepts "YYYY-MM-DDTHH:MM:SSZ", "YYYY-MM-DD HH:MM:SS" and "YYYY-MM-DD"
    // (date only means 23:59:59 that day). Throws ValidationError otherwise.
    static DateTime parseIso(const std::string& text);

    std::string toIso() const;
    constexpr std::int64_t epochSeconds() const { return epochSeconds_; }

    DateTime plusSeconds(std::int64_t s) const { return DateTime(epochSeconds_ + s); }
    DateTime plusDays(std::int64_t d) const { return DateTime(epochSeconds_ + d * kSecondsPerDay); }

    friend constexpr bool operator==(DateTime a, DateTime b) { return a.epochSeconds_ == b.epochSeconds_; }
    friend constexpr bool operator!=(DateTime a, DateTime b) { return !(a == b); }
    friend constexpr bool operator<(DateTime a, DateTime b) { return a.epochSeconds_ < b.epochSeconds_; }
    friend constexpr bool operator<=(DateTime a, DateTime b) { return a.epochSeconds_ <= b.epochSeconds_; }
    friend constexpr bool operator>(DateTime a, DateTime b) { return a.epochSeconds_ > b.epochSeconds_; }
    friend constexpr bool operator>=(DateTime a, DateTime b) { return a.epochSeconds_ >= b.epochSeconds_; }
};
