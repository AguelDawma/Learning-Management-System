#include "LatePenaltyPolicy.hpp"

#include <algorithm>

PenaltyResult TenPercentPerDayPolicy::apply(double raw, int maxMark, int daysLate) const {
    if (daysLate <= 0) return {0.0, raw};
    if (daysLate > maxDays_) return {raw, 0.0};
    const double penalty = std::min(raw, ratePerDay_ * daysLate * maxMark);
    return {penalty, raw - penalty};
}

PenaltyResult NoPenaltyPolicy::apply(double raw, int /*maxMark*/, int /*daysLate*/) const {
    return {0.0, raw};
}
