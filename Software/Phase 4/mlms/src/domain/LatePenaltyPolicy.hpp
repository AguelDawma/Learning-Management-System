#pragma once

struct PenaltyResult {
    double penalty = 0.0;
    double finalMark = 0.0;
};

// Strategy pattern: the late-penalty rule (BR-02) is the policy most likely to
// change, so MarkingService and ModerationService only see this interface.
class ILatePenaltyPolicy {
public:
    virtual ~ILatePenaltyPolicy() = default;
    virtual PenaltyResult apply(double raw, int maxMark, int daysLate) const = 0;
};

// BR-02: lose 10% of the available (maximum) mark for each day late, for up to
// three days; after that the work receives zero. The final mark is never negative.
class TenPercentPerDayPolicy : public ILatePenaltyPolicy {
    double ratePerDay_;
    int maxDays_;

public:
    explicit TenPercentPerDayPolicy(double ratePerDay = 0.10, int maxDays = 3)
        : ratePerDay_(ratePerDay), maxDays_(maxDays) {}
    PenaltyResult apply(double raw, int maxMark, int daysLate) const override;
};

class NoPenaltyPolicy : public ILatePenaltyPolicy {
public:
    PenaltyResult apply(double raw, int maxMark, int daysLate) const override;
};
