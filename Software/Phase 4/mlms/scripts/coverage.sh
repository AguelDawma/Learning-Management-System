#!/usr/bin/env bash
# Line coverage of the business-logic layer (domain + services) using plain gcov.
# Usage (from the mlms folder): scripts/coverage.sh [build-dir]   default: build-cov
set -euo pipefail
BUILD="${1:-build-cov}"

cmake -S . -B "$BUILD" -G Ninja -DMLMS_COVERAGE=ON -DMLMS_BUILD_GUI=OFF >/dev/null
cmake --build "$BUILD"
find "$BUILD" -name '*.gcda' -delete
"$BUILD/tests/mlms_tests" --gtest_brief=1

OBJ="$BUILD/src/CMakeFiles/mlms_core.dir"
REPORT="$BUILD/coverage.txt"
printf '%-40s %8s %8s\n' "File" "Lines" "Covered" >"$REPORT"
for layer in domain services data infrastructure; do
    for gcda in "$OBJ/$layer"/*.gcda; do
        gcov -n -o "$OBJ/$layer" "$gcda" 2>/dev/null |
            awk -v layer="$layer" '
                { sub(/\r$/, "") } # gcov on Windows prints CRLF
                /^File / { file = substr($0, 6); gsub(/\x27/, "", file); keep = (file ~ ("src/" layer "/.*[.]cpp$")) }
                keep && /^Lines executed:/ {
                    split($2, a, ":"); pct = a[2]; sub(/%/, "", pct); n = $4
                    sub(/.*src\//, "", file)
                    printf "%-40s %8d %7.1f%%\n", file, n, pct
                    keep = 0
                }'
    done
done >>"$REPORT"
awk '$1 ~ /^(domain|services)\// { total += $2; covered += $2 * $3 / 100 }
     END { printf "\nBusiness logic (domain + services): %.1f%% of %d lines\n", 100 * covered / total, total }' \
    "$REPORT" >>"$REPORT"
cat "$REPORT"
