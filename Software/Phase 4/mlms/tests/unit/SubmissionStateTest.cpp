#include "domain/Errors.hpp"
#include "domain/Submission.hpp"

#include <gtest/gtest.h>

#include <set>
#include <utility>

namespace {

const DateTime kDeadline = DateTime::fromCivil(2026, 11, 7, 23, 59, 0);

Submission at(DateTime t, SubmissionState s = SubmissionState::SUBMITTED) {
    return Submission(1, 1, 1, 1, "a1/s1/v1_x.pdf", std::string(64, 'a'), t, s);
}

} // namespace

TEST(SubmissionDaysLate, BoundaryValues) {
    EXPECT_EQ(at(kDeadline.plusSeconds(-1)).daysLate(kDeadline), 0);
    EXPECT_EQ(at(kDeadline).daysLate(kDeadline), 0); // exactly on the deadline is on time
    EXPECT_EQ(at(kDeadline.plusSeconds(1)).daysLate(kDeadline), 1);
    EXPECT_EQ(at(kDeadline.plusDays(1)).daysLate(kDeadline), 1);
    EXPECT_EQ(at(kDeadline.plusDays(1).plusSeconds(1)).daysLate(kDeadline), 2);
    EXPECT_EQ(at(kDeadline.plusDays(3)).daysLate(kDeadline), 3);
    EXPECT_EQ(at(kDeadline.plusDays(3).plusSeconds(1)).daysLate(kDeadline), 4);
}

TEST(SubmissionStateMachine, AllowsExactlyTheDesignedTransitions) {
    using S = SubmissionState;
    const std::set<std::pair<S, S>> allowed = {
        {S::SUBMITTED, S::MARKED}, {S::LATE, S::MARKED},       {S::LATE, S::SUBMITTED},
        {S::MARKED, S::RETURNED},  {S::RETURNED, S::MARKED},   {S::MARKED, S::MODERATED},
        {S::MODERATED, S::RELEASED}};
    const S all[] = {S::SUBMITTED, S::LATE, S::MARKED, S::RETURNED, S::MODERATED, S::RELEASED};
    for (S from : all) {
        for (S to : all) {
            auto s = at(kDeadline, from);
            if (allowed.count({from, to})) {
                EXPECT_NO_THROW(s.transitionTo(to)) << toString(from) << "->" << toString(to);
                EXPECT_EQ(s.state(), to);
            } else {
                EXPECT_THROW(s.transitionTo(to), InvalidTransition) << toString(from) << "->" << toString(to);
                EXPECT_EQ(s.state(), from);
            }
        }
    }
}

TEST(SubmissionStateMachine, StateNamesRoundTrip) {
    for (auto s : {SubmissionState::SUBMITTED, SubmissionState::LATE, SubmissionState::MARKED,
                   SubmissionState::RETURNED, SubmissionState::MODERATED, SubmissionState::RELEASED}) {
        EXPECT_EQ(submissionStateFromString(toString(s)), s);
    }
    EXPECT_THROW(submissionStateFromString("DRAFT"), ValidationError);
}

TEST(Submission, VersionMustBePositive) {
    EXPECT_THROW(Submission(1, 1, 1, 0, "", "", kDeadline, SubmissionState::SUBMITTED), ValidationError);
}
