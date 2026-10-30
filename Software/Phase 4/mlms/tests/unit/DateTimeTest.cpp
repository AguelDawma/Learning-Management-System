#include "domain/DateTime.hpp"
#include "domain/Errors.hpp"

#include <gtest/gtest.h>

TEST(DateTime, EpochIsUnixEpoch) {
    EXPECT_EQ(DateTime::fromCivil(1970, 1, 1).epochSeconds(), 0);
    EXPECT_EQ(DateTime(0).toIso(), "1970-01-01T00:00:00Z");
}

TEST(DateTime, FormatsAndParsesRoundTrip) {
    const auto t = DateTime::fromCivil(2026, 11, 7, 23, 59, 30);
    EXPECT_EQ(t.toIso(), "2026-11-07T23:59:30Z");
    EXPECT_EQ(DateTime::parseIso(t.toIso()), t);
}

TEST(DateTime, AcceptsSpaceSeparatorAndMissingSeconds) {
    EXPECT_EQ(DateTime::parseIso("2026-11-07 23:59:30"), DateTime::fromCivil(2026, 11, 7, 23, 59, 30));
    EXPECT_EQ(DateTime::parseIso("2026-11-07T08:15Z"), DateTime::fromCivil(2026, 11, 7, 8, 15, 0));
}

TEST(DateTime, DateOnlyMeansEndOfDay) {
    EXPECT_EQ(DateTime::parseIso("2026-11-07"), DateTime::fromCivil(2026, 11, 7, 23, 59, 59));
}

TEST(DateTime, LeapYearsAreHandled) {
    EXPECT_NO_THROW(DateTime::fromCivil(2028, 2, 29));
    EXPECT_THROW(DateTime::fromCivil(2026, 2, 29), ValidationError);
    EXPECT_THROW(DateTime::fromCivil(2100, 2, 29), ValidationError);
    EXPECT_NO_THROW(DateTime::fromCivil(2000, 2, 29));
}

TEST(DateTime, RejectsMalformedText) {
    for (const char* bad : {"", "2026", "2026-13-01", "2026-11-31", "2026/11/07", "2026-11-07T25:00:00Z",
                            "2026-11-07T10:60Z", "2026-11-07T10:00:00+02:00", "yesterday"}) {
        EXPECT_THROW(DateTime::parseIso(bad), ValidationError) << bad;
    }
}

TEST(DateTime, OrderingAndArithmetic) {
    const auto a = DateTime::fromCivil(2026, 12, 31, 23, 0, 0);
    const auto b = a.plusDays(1);
    EXPECT_LT(a, b);
    EXPECT_EQ(b.toIso(), "2027-01-01T23:00:00Z");
    EXPECT_EQ(a.plusSeconds(3600).toIso(), "2027-01-01T00:00:00Z");
}
