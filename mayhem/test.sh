#!/usr/bin/env bash
#
# mayhem/test.sh — RUN Catime's functional checks (built by mayhem/build.sh).
# exit 0 = pass. EDIT per repo. PATCH-grade oracle: after an agent patches the source, the grader
# rebuilds (build.sh) then runs this. DELETE this file if the repo has no meaningful tests.
#
# IMPORTANT:
#  * Must assert BEHAVIOR/OUTPUT, not just exit status. The oracle has to check asserted values /
#    golden-output diffs / known-answer results — so a PATCH that "fixes" a bug by making the program
#    exit(0) (or any no-op) FAILS here. Running inputs and checking only "exit 0 / didn't crash" is
#    NOT a functional test (it's trivially reward-hackable) — use the project's real assertion suite.
#  * Do NOT build here — mayhem/build.sh already compiled the test suite (with the project's normal
#    flags). This script only RUNS the pre-built tests and reports counts. If the test runner is
#    missing, that's a build.sh bug — fail loudly rather than silently rebuilding.
#  * REQUIRED OUTPUT — a CTRF (https://ctrf.io) summary so Mayhem/the PATCH grader reads the counts:
#      - writes a CTRF JSON report to ${CTRF_REPORT:-$SRC/ctrf-report.json}, and
#      - prints a one-line `CTRF {...}` marker to stdout (same JSON, compact).
#    Only `results.summary` (with tests/passed/failed/pending/skipped/other) is required.
#    Use the emit_ctrf helper below; it computes tests = passed+failed+skipped and sets the exit
#    code (0 iff failed==0). Map your framework's output to passed/failed/skipped.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${MAYHEM_JOBS:=$(nproc)}"   # build parallelism; env-overridable, falls back to nproc (use -j"$MAYHEM_JOBS")
cd "$SRC"

# emit_ctrf <tool> <passed> <failed> [skipped] [pending] [other]
# Writes a CTRF report (file + stdout `CTRF {...}` marker) and returns non-zero iff failed>0.
emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}


OUT="$SRC/build-mayhem"
for b in time_parser_tests font_name_check ani_check; do
  [ -x "$OUT/$b" ] || { echo "missing $OUT/$b — build.sh did not build it" >&2; emit_ctrf "catime-checks" 0 1; exit 1; }
done
passed=0; failed=0
# 1) upstream's tests/time_parser_tests.c (asserts parsed seconds for known inputs)
if "$OUT/time_parser_tests"; then passed=$((passed+1)); echo "PASS time_parser_tests"; else failed=$((failed+1)); echo "FAIL time_parser_tests"; fi
# 2) known-answer: family names decoded from upstream's bundled fonts
export LC_ALL=C
if diff -u "$SRC/mayhem/tests/font_names.expected" <("$OUT/font_name_check" "$SRC"/asset/font/*/*.ttf "$SRC"/asset/font/*/*.otf); then
  passed=$((passed+1)); echo "PASS font_names"
else
  failed=$((failed+1)); echo "FAIL font_names"
fi
# 3) known-answer: frame count / per-frame delays decoded from the .ani samples in mayhem/tests/ani
if diff -u "$SRC/mayhem/tests/ani.expected" <("$OUT/ani_check" "$SRC"/mayhem/tests/ani/*.ani); then
  passed=$((passed+1)); echo "PASS ani_decode"
else
  failed=$((failed+1)); echo "FAIL ani_decode"
fi
emit_ctrf "catime-checks" "$passed" "$failed"
