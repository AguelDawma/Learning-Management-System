#pragma once

#include <algorithm>

class PenaltyStrategy {
public:
    virtual ~PenaltyStrategy() = default;
    virtual double calculateMark(double rawMark, bool isLate) const = 0;
};

class StandardPenaltyStrategy : public PenaltyStrategy {
    double lateDeductionPercentage_;

public:
    explicit StandardPenaltyStrategy(double lateDeductionPercentage = 10.0) 
        : lateDeductionPercentage_(lateDeductionPercentage) {}

    double calculateMark(double rawMark, bool isLate) const override {
        if (!isLate) {
            return rawMark;
        }
        double penalized = rawMark - (rawMark * (lateDeductionPercentage_ / 100.0));
        return std::max(0.0, penalized);
    }
};