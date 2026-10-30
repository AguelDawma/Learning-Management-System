#include "TestSupport.hpp"

// System tests for "Submit assignment" (UC: Submit assignment, Figure 7).
class SubmissionServiceTest : public LmsFixture {};

TEST_F(SubmissionServiceTest, FirstOnTimeSubmissionIsSubmittedVersionOne) {
    clock->t = deadline.plusDays(-1);
    EXPECT_EQ(ctx->submissionService.preview(*student, assignmentId).kind, SubmissionPlan::Kind::FirstOnTime);
    const auto r = ctx->submissionService.submit(*student, assignmentId, file());
    EXPECT_EQ(r.version, 1);
    EXPECT_EQ(r.state, SubmissionState::SUBMITTED);
    EXPECT_EQ(r.submittedAt, clock->t); // host clock, not user input
    EXPECT_EQ(r.checksum.size(), 64u);
    const auto s = ctx->submissions.find(r.submissionId);
    EXPECT_TRUE(std::filesystem::exists(ctx->files.resolve(s->filePath())));
}

TEST_F(SubmissionServiceTest, SubmittingExactlyAtDeadlineIsOnTime) {
    const int id = submitAt(*student, deadline);
    EXPECT_EQ(ctx->submissions.find(id)->state(), SubmissionState::SUBMITTED);
}

TEST_F(SubmissionServiceTest, ResubmissionBeforeDeadlineAddsNewActiveVersion) {
    const int v1 = submitAt(*student, deadline.plusDays(-2), "%PDF first");
    clock->t = deadline.plusDays(-1);
    EXPECT_EQ(ctx->submissionService.preview(*student, assignmentId).kind, SubmissionPlan::Kind::Resubmission);
    const auto r = ctx->submissionService.submit(*student, assignmentId, file("work.pdf", "%PDF second"));
    EXPECT_EQ(r.version, 2);
    EXPECT_EQ(ctx->submissions.findActive(assignmentId, student->id())->id(), r.submissionId);
    // The older version is kept read-only.
    EXPECT_TRUE(ctx->submissions.find(v1).has_value());
    EXPECT_EQ(ctx->submissionService.myVersions(*student, assignmentId).size(), 2u);
}

TEST_F(SubmissionServiceTest, LateSubmissionIsAcceptedAndFlagged) {
    clock->t = deadline.plusSeconds(1);
    const auto plan = ctx->submissionService.preview(*student, assignmentId);
    EXPECT_EQ(plan.kind, SubmissionPlan::Kind::Late);
    EXPECT_EQ(plan.daysLate, 1);
    const auto r = ctx->submissionService.submit(*student, assignmentId, file());
    EXPECT_EQ(r.state, SubmissionState::LATE);
    EXPECT_EQ(r.daysLate, 1);
}

TEST_F(SubmissionServiceTest, VeryLateWorkIsStillAccepted) {
    clock->t = deadline.plusDays(10);
    const auto r = ctx->submissionService.submit(*student, assignmentId, file());
    EXPECT_EQ(r.state, SubmissionState::LATE);
    EXPECT_EQ(r.daysLate, 10);
}

TEST_F(SubmissionServiceTest, LateReplacementOfOnTimeWorkIsRefused) {
    submitAt(*student, deadline.plusDays(-1));
    clock->t = deadline.plusSeconds(60);
    EXPECT_EQ(ctx->submissionService.preview(*student, assignmentId).kind, SubmissionPlan::Kind::Refused);
    EXPECT_THROW(ctx->submissionService.submit(*student, assignmentId, file()), SubmissionRefused);
    EXPECT_EQ(ctx->submissions.findVersions(assignmentId, student->id()).size(), 1u);
}

TEST_F(SubmissionServiceTest, MarkedWorkCannotBeReplaced) {
    const int id = submitAt(*student, deadline.plusDays(-2));
    ctx->marking.saveMark(*lecturer, id, 50, "Reasonable attempt", "");
    clock->t = deadline.plusDays(-1);
    EXPECT_THROW(ctx->submissionService.submit(*student, assignmentId, file()), SubmissionRefused);
}

TEST_F(SubmissionServiceTest, ExtensionMovesTheDeadlineForThatStudentOnly) {
    ctx->assignmentService.grantExtension(*lecturer, assignmentId, student->id(), deadline.plusDays(2), "Illness");
    clock->t = deadline.plusDays(1);
    EXPECT_EQ(ctx->submissionService.submit(*student, assignmentId, file()).state, SubmissionState::SUBMITTED);
    EXPECT_EQ(ctx->submissionService.submit(*student2, assignmentId, file()).state, SubmissionState::LATE);
}

TEST_F(SubmissionServiceTest, InvalidFilesAreRejectedBeforeAnythingIsSaved) {
    clock->t = deadline.plusDays(-1);
    EXPECT_THROW(ctx->submissionService.submit(*student, assignmentId, work.write("x.exe", "MZ")), ValidationError);
    EXPECT_THROW(ctx->submissionService.submit(*student, assignmentId, work.write("e.pdf", "")), ValidationError);
    EXPECT_THROW(ctx->submissionService.submit(*student, assignmentId, work.path() / "none.pdf"), ValidationError);
    EXPECT_FALSE(ctx->submissions.findActive(assignmentId, student->id()).has_value());
}

TEST_F(SubmissionServiceTest, OnlyEnrolledStudentsMaySubmit) {
    clock->t = deadline.plusDays(-1);
    EXPECT_THROW(ctx->submissionService.submit(*outsider, assignmentId, file()), AccessDenied);
    EXPECT_THROW(ctx->submissionService.submit(*lecturer, assignmentId, file()), AccessDenied);
}

TEST_F(SubmissionServiceTest, UnpublishedAssignmentsAreInvisible) {
    clock->t = deadline.plusDays(-3);
    const int hidden = ctx->assignmentService.create(*lecturer, courseId, "Draft", "", deadline, 10, false);
    EXPECT_THROW(ctx->submissionService.submit(*student, hidden, file()), NotFound);
    for (const auto& v : ctx->submissionService.myAssignments(*student)) EXPECT_NE(v.assignment.id(), hidden);
    ctx->assignmentService.setPublished(*lecturer, hidden, true);
    EXPECT_NO_THROW(ctx->submissionService.submit(*student, hidden, file()));
}

TEST_F(SubmissionServiceTest, StudentListShowsExtensionAndStatus) {
    ctx->assignmentService.grantExtension(*lecturer, assignmentId, student->id(), deadline.plusDays(1), "Funeral");
    submitAt(*student, deadline.plusDays(-1));
    const auto list = ctx->submissionService.myAssignments(*student);
    ASSERT_EQ(list.size(), 1u);
    EXPECT_TRUE(list[0].extension.has_value());
    EXPECT_EQ(list[0].effectiveDeadline, deadline.plusDays(1));
    ASSERT_TRUE(list[0].active.has_value());
    EXPECT_EQ(list[0].active->state(), SubmissionState::SUBMITTED);
    EXPECT_FALSE(list[0].result.has_value());
}
