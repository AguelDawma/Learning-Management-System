#include "domain/LatePenaltyPolicy.hpp"
#include "domain/Mark.hpp"

#include <gtest/gtest.h>

// Boundary value analysis for BR-02 (10% of the maximum mark per day, zero after day 3).
struct PenaltyCase {
    double raw;
    int maxMark;
    int daysLate;
    double penalty;
    double finalMark;
};

class TenPercentPolicyTest : public ::testing::TestWithParam<PenaltyCase> {};

TEST_P(TenPercentPolicyTest, AppliesRule) {
    const auto c = GetParam();
    const auto r = TenPercentPerDayPolicy().apply(c.raw, c.maxMark, c.daysLate);
    EXPECT_DOUBLE_EQ(r.penalty, c.penalty);
    EXPECT_DOUBLE_EQ(r.finalMark, c.finalMark);
}

INSTANTIATE_TEST_SUITE_P(Boundaries, TenPercentPolicyTest,
                         ::testing::Values(PenaltyCase{80, 100, -1, 0, 80},  // early
                                           PenaltyCase{80, 100, 0, 0, 80},   // on time
                                           PenaltyCase{80, 100, 1, 10, 70},  // first late day
                                           PenaltyCase{80, 100, 2, 20, 60},
                                           PenaltyCase{80, 100, 3, 30, 50},  // last penalised day
                                           PenaltyCase{80, 100, 4, 80, 0},   // beyond three days
                                           PenaltyCase{80, 100, 30, 80, 0},
                                           PenaltyCase{15, 100, 2, 15, 0},   // never below zero
                                           PenaltyCase{40, 50, 1, 5, 35},    // 10% of the max mark, not the raw
                                           PenaltyCase{0, 100, 1, 0, 0},
                                           PenaltyCase{100, 100, 0, 0, 100}));

TEST(NoPenaltyPolicy, NeverDeducts) {
    const auto r = NoPenaltyPolicy().apply(70, 100, 10);
    EXPECT_DOUBLE_EQ(r.penalty, 0);
    EXPECT_DOUBLE_EQ(r.finalMark, 70);
}

TEST(Mark, FinaliseUsesInjectedStrategy) {
    const Mark m(1, 90, "Good work overall", 2, DateTime(0));
    EXPECT_DOUBLE_EQ(m.finalise(TenPercentPerDayPolicy(), 100, 2).finalMark, 70);
    EXPECT_DOUBLE_EQ(m.finalise(NoPenaltyPolicy(), 100, 2).finalMark, 90);
}
