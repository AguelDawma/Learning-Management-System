#include "TestSupport.hpp"

// End-to-end process from submission to released mark (Figures 5, 6 and 8).
class MarkingWorkflowTest : public LmsFixture {
protected:
    std::optional<StudentAssignmentView> viewFor(const User& s) {
        for (auto& v : ctx->submissionService.myAssignments(s)) {
            if (v.assignment.id() == assignmentId) return v;
        }
        return std::nullopt;
    }
    SubmissionState stateOf(int id) { return ctx->submissions.find(id)->state(); }
};

TEST_F(MarkingWorkflowTest, FullLifeCycleWithLatePenaltyAndAudit) {
    const int sid = submitAt(*student, deadline.plusDays(2).plusSeconds(-60)); // 2 days late
    ctx->marking.saveMark(*lecturer, sid, 80, "Well structured answer", "");
    EXPECT_EQ(stateOf(sid), SubmissionState::MARKED);
    EXPECT_EQ(auditCount(sid), 3); // raw_mark, feedback, state

    // Principal's rule: nothing is visible to the student before release.
    EXPECT_FALSE(viewFor(*student)->result.has_value());

    const auto sheet = ctx->moderation.moderationSheet(*hod, assignmentId);
    ASSERT_EQ(sheet.size(), 1u);
    EXPECT_EQ(sheet[0].daysLate, 2);
    EXPECT_DOUBLE_EQ(sheet[0].result->penalty, 20);
    EXPECT_DOUBLE_EQ(sheet[0].result->finalMark, 60);

    EXPECT_EQ(ctx->moderation.approve(*hod, {sid}), 1);
    EXPECT_EQ(stateOf(sid), SubmissionState::MODERATED);
    EXPECT_EQ(ctx->marks.find(sid)->moderatedBy(), hod->id());
    EXPECT_FALSE(viewFor(*student)->result.has_value());

    clock->t = clock->t.plusDays(1);
    EXPECT_EQ(ctx->moderation.release(*hod, {sid}), 1);
    EXPECT_EQ(stateOf(sid), SubmissionState::RELEASED);
    const auto result = viewFor(*student)->result;
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(result->rawMark, 80);
    EXPECT_DOUBLE_EQ(result->penalty, 20);
    EXPECT_DOUBLE_EQ(result->finalMark, 60);
    EXPECT_EQ(result->feedback, "Well structured answer");
    EXPECT_EQ(result->releasedAt, clock->t);
    EXPECT_EQ(auditCount(sid), 5);
}

TEST_F(MarkingWorkflowTest, AuditRecordsWhoWhenOldNewAndWhy) {
    const int sid = submitAt(*student, deadline.plusDays(-1));
    ctx->marking.saveMark(*lecturer, sid, 60, "Needs more detail", "");
    clock->t = clock->t.plusSeconds(3600);
    ctx->marking.saveMark(*lecturer, sid, 65, "Needs more detail", "Missed page 3 on first read");
    const auto log = ctx->audit.findBySubmission(sid);
    const auto& last = log.back();
    EXPECT_EQ(last.userId, lecturer->id());
    EXPECT_EQ(last.field, "raw_mark");
    EXPECT_EQ(last.oldValue, std::optional<std::string>("60"));
    EXPECT_EQ(last.newValue, "65");
    EXPECT_EQ(last.reason, "Missed page 3 on first read");
    EXPECT_EQ(last.timestamp, clock->t);
}

TEST_F(MarkingWorkflowTest, MarkValidation) {
    const int sid = submitAt(*student, deadline.plusDays(-1));
    EXPECT_THROW(ctx->marking.saveMark(*lecturer, sid, 101, "Good effort here", ""), ValidationError);
    EXPECT_THROW(ctx->marking.saveMark(*lecturer, sid, -1, "Good effort here", ""), ValidationError);
    EXPECT_THROW(ctx->marking.saveMark(*lecturer, sid, 50, "Too short", ""), ValidationError);
    EXPECT_EQ(stateOf(sid), SubmissionState::SUBMITTED);
    EXPECT_EQ(auditCount(sid), 0);
}

TEST_F(MarkingWorkflowTest, EditingAMarkNeedsAReason) {
    const int sid = submitAt(*student, deadline.plusDays(-1));
    ctx->marking.saveMark(*lecturer, sid, 60, "Needs more detail", "");
    EXPECT_THROW(ctx->marking.saveMark(*lecturer, sid, 70, "Needs more detail", "  "), ValidationError);
    EXPECT_THROW(ctx->marking.saveMark(*lecturer, sid, 60, "Needs more detail", "no change"), ValidationError);
    EXPECT_DOUBLE_EQ(ctx->marks.find(sid)->rawMark(), 60);
}

TEST_F(MarkingWorkflowTest, OnlyTheCourseLecturerMayMark) {
    const int sid = submitAt(*student, deadline.plusDays(-1));
    auto otherLecturer = makeUser("lecturer2", Role::LECTURER);
    EXPECT_THROW(ctx->marking.saveMark(*otherLecturer, sid, 50, "Reasonable work", ""), AccessDenied);
    EXPECT_THROW(ctx->marking.markingList(*otherLecturer, assignmentId), AccessDenied);
    EXPECT_THROW(ctx->marking.saveMark(*hod, sid, 50, "Reasonable work", ""), AccessDenied);
}

TEST_F(MarkingWorkflowTest, OlderVersionsCannotBeMarked) {
    const int v1 = submitAt(*student, deadline.plusDays(-2), "%PDF v1");
    submitAt(*student, deadline.plusDays(-1), "%PDF v2");
    EXPECT_THROW(ctx->marking.saveMark(*lecturer, v1, 50, "Reasonable work", ""), InvalidTransition);
}

TEST_F(MarkingWorkflowTest, ReturnForRemarkingRoundTrip) {
    const int sid = submitAt(*student, deadline.plusDays(-1));
    ctx->marking.saveMark(*lecturer, sid, 90, "Excellent throughout", "");
    EXPECT_THROW(ctx->moderation.returnForRemarking(*hod, sid, " "), ValidationError);
    ctx->moderation.returnForRemarking(*hod, sid, "Too generous for question 2");
    EXPECT_EQ(stateOf(sid), SubmissionState::RETURNED);
    EXPECT_EQ(ctx->marking.markingList(*lecturer, assignmentId)[0].mark->returnComment(),
              std::optional<std::string>("Too generous for question 2"));

    EXPECT_THROW(ctx->moderation.approve(*hod, {sid}), InvalidTransition);
    EXPECT_THROW(ctx->marking.saveMark(*lecturer, sid, 75, "Excellent throughout", ""), ValidationError);
    ctx->marking.saveMark(*lecturer, sid, 75, "Excellent throughout", "Re-marked Q2 as advised by HoD");
    EXPECT_EQ(stateOf(sid), SubmissionState::MARKED);
    EXPECT_FALSE(ctx->marks.find(sid)->returnComment().has_value());
    EXPECT_NO_THROW(ctx->moderation.approve(*hod, {sid}));
}

TEST_F(MarkingWorkflowTest, LecturerCannotChangeMarkAfterModeration) {
    const int sid = submitAt(*student, deadline.plusDays(-1));
    ctx->marking.saveMark(*lecturer, sid, 60, "Needs more detail", "");
    ctx->moderation.approve(*hod, {sid});
    EXPECT_THROW(ctx->marking.saveMark(*lecturer, sid, 70, "Needs more detail", "late change"), InvalidTransition);
}

TEST_F(MarkingWorkflowTest, LecturerCannotRelease) {
    const int sid = submitAt(*student, deadline.plusDays(-1));
    ctx->marking.saveMark(*lecturer, sid, 60, "Needs more detail", "");
    EXPECT_THROW(ctx->moderation.approve(*lecturer, {sid}), AccessDenied);
    EXPECT_THROW(ctx->moderation.release(*lecturer, {sid}), AccessDenied);
}

TEST_F(MarkingWorkflowTest, HodOfAnotherDepartmentIsDenied) {
    const int sid = submitAt(*student, deadline.plusDays(-1));
    ctx->marking.saveMark(*lecturer, sid, 60, "Needs more detail", "");
    EXPECT_THROW(ctx->moderation.moderationSheet(*otherHod, assignmentId), AccessDenied);
    EXPECT_THROW(ctx->moderation.approve(*otherHod, {sid}), AccessDenied);
    EXPECT_TRUE(ctx->moderation.departmentAssignments(*otherHod).empty());
    EXPECT_EQ(ctx->moderation.departmentAssignments(*hod).size(), 1u);
}

TEST_F(MarkingWorkflowTest, ReleaseIsAllOrNothing) {
    const int a = submitAt(*student, deadline.plusDays(-1));
    const int b = submitAt(*student2, deadline.plusDays(-1));
    ctx->marking.saveMark(*lecturer, a, 60, "Needs more detail", "");
    ctx->marking.saveMark(*lecturer, b, 70, "Good structure overall", "");
    ctx->moderation.approve(*hod, {a});
    const int auditBefore = auditCount(a);
    // b is only MARKED, so the whole batch is rolled back.
    EXPECT_THROW(ctx->moderation.release(*hod, {a, b}), InvalidTransition);
    EXPECT_EQ(stateOf(a), SubmissionState::MODERATED);
    EXPECT_FALSE(ctx->marks.find(a)->releasedAt().has_value());
    EXPECT_EQ(auditCount(a), auditBefore);
}

TEST_F(MarkingWorkflowTest, HodAdjustsModeratedAndCorrectsReleasedMarks) {
    const int sid = submitAt(*student, deadline.plusDays(-1));
    ctx->marking.saveMark(*lecturer, sid, 60, "Needs more detail", "");
    EXPECT_THROW(ctx->moderation.adjustMark(*hod, sid, 65, "Too early"), InvalidTransition);
    ctx->moderation.approve(*hod, {sid});
    EXPECT_THROW(ctx->moderation.adjustMark(*hod, sid, 65, ""), ValidationError);
    EXPECT_THROW(ctx->moderation.adjustMark(*hod, sid, 150, "Too high"), ValidationError);
    ctx->moderation.adjustMark(*hod, sid, 65, "Moderation sample showed under-marking");
    ctx->moderation.release(*hod, {sid});
    ctx->moderation.adjustMark(*hod, sid, 68, "Addition error found after release");
    EXPECT_DOUBLE_EQ(viewFor(*student)->result->finalMark, 68);
    EXPECT_EQ(ctx->audit.findBySubmission(sid).back().field, "raw_mark (corrected after release)");
}

TEST_F(MarkingWorkflowTest, StaleRowVersionFromAnotherScreenIsRejected) {
    const int sid = submitAt(*student, deadline.plusDays(-1));
    ctx->marking.saveMark(*lecturer, sid, 60, "Needs more detail", "");
    const int loaded = ctx->marks.find(sid)->rowVersion();
    ctx->marking.saveMark(*lecturer, sid, 62, "Needs more detail", "first edit", loaded);
    EXPECT_THROW(ctx->marking.saveMark(*lecturer, sid, 64, "Needs more detail", "second edit", loaded),
                 ConcurrencyError);
}

TEST_F(MarkingWorkflowTest, ExtensionTurnsLateSubmissionBackToSubmitted) {
    const int sid = submitAt(*student, deadline.plusDays(1));
    EXPECT_EQ(stateOf(sid), SubmissionState::LATE);
    EXPECT_THROW(ctx->assignmentService.grantExtension(*lecturer, assignmentId, student->id(), deadline.plusDays(2), " "),
                 ValidationError);
    EXPECT_THROW(ctx->assignmentService.grantExtension(*lecturer, assignmentId, student->id(), deadline, "Illness"),
                 ValidationError);
    EXPECT_THROW(ctx->assignmentService.grantExtension(*lecturer, assignmentId, outsider->id(), deadline.plusDays(2),
                                                       "Illness"),
                 ValidationError);
    ctx->assignmentService.grantExtension(*lecturer, assignmentId, student->id(), deadline.plusDays(2), "Illness");
    EXPECT_EQ(stateOf(sid), SubmissionState::SUBMITTED);
    EXPECT_EQ(ctx->audit.findBySubmission(sid).back().reason, "Extension granted: Illness");

    ctx->marking.saveMark(*lecturer, sid, 80, "Very good analysis", "");
    EXPECT_DOUBLE_EQ(ctx->marking.markingList(*lecturer, assignmentId)[0].result->penalty, 0);
    // Once marked, the deadline can no longer be changed for this work.
    EXPECT_THROW(ctx->assignmentService.grantExtension(*lecturer, assignmentId, student->id(), deadline.plusDays(5),
                                                       "Again"),
                 ValidationError);
}

TEST_F(MarkingWorkflowTest, ExtensionShorterThanLateSubmissionKeepsItLate) {
    const int sid = submitAt(*student, deadline.plusDays(3));
    ctx->assignmentService.grantExtension(*lecturer, assignmentId, student->id(), deadline.plusDays(1), "Illness");
    EXPECT_EQ(stateOf(sid), SubmissionState::LATE);
    EXPECT_EQ(ctx->marking.markingList(*lecturer, assignmentId)[0].daysLate, 2);
}

TEST_F(MarkingWorkflowTest, AuditViewIsScopedByRole) {
    const int sid = submitAt(*student, deadline.plusDays(-1));
    ctx->marking.saveMark(*lecturer, sid, 60, "Needs more detail", "");
    const auto mine = ctx->auditService.entries(*hod);
    ASSERT_EQ(mine.size(), 3u);
    EXPECT_EQ(mine[0].studentName, student->fullName());
    EXPECT_EQ(mine[0].assignmentTitle, "Loops");
    EXPECT_TRUE(ctx->auditService.entries(*otherHod).empty());
    EXPECT_EQ(ctx->auditService.entries(*admin).size(), 3u);
    EXPECT_THROW(ctx->auditService.entries(*student), AccessDenied);
}

TEST_F(MarkingWorkflowTest, AssignmentCreationRules) {
    EXPECT_THROW(ctx->assignmentService.create(*lecturer, courseId, "Past", "", clock->t, 10, true), ValidationError);
    EXPECT_THROW(ctx->assignmentService.create(*lecturer, courseId, "Big", "", deadline, 1001, true), ValidationError);
    auto other = makeUser("lecturer3", Role::LECTURER);
    EXPECT_THROW(ctx->assignmentService.create(*other, courseId, "Not mine", "", deadline, 10, true), AccessDenied);
    EXPECT_THROW(ctx->assignmentService.create(*student, courseId, "No", "", deadline, 10, true), AccessDenied);
    submitAt(*student, deadline.plusDays(-1));
    EXPECT_THROW(ctx->assignmentService.setPublished(*lecturer, assignmentId, false), ValidationError);
    EXPECT_EQ(ctx->assignmentService.myAssignments(*lecturer)[0].submissionCount, 1);
}
