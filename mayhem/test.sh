#!/usr/bin/env bash
#
# mayhem/test.sh — RUN this repo's OWN functional test suite (already built by mayhem/build.sh).
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

O="$SRC/build/oracle"
EG="$SRC/tests/shape_eg_data"
for b in shpdump dbfdump shpcreate dbfcreate shpadd dbfadd shptest kat_sbn; do
  [ -x "$O/$b" ] || { echo "missing $O/$b — mayhem/build.sh should have built it" >&2; emit_ctrf "shapelib-shell" 0 1 0; exit 1; }
done

passed=0; failed=0
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
cd "$WORK"

# Upstream's golden-output suites (diff the CLI tools' output against tests/expect*.out).
export SHPDUMP="$O/shpdump" DBFDUMP="$O/dbfdump" SHPCREATE="$O/shpcreate" DBFCREATE="$O/dbfcreate" \
       SHPADD="$O/shpadd" DBFADD="$O/dbfadd" SHPTEST="$O/shptest"
for n in 1 2 3; do
  if sh "$SRC/tests/test$n.sh" "$SRC/tests/expect$n.out" "$EG" > "t$n.log" 2>&1 && grep -q "Stream $n Succeeded" "t$n.log"; then
    passed=$((passed+1))
  else
    failed=$((failed+1)); echo "FAIL: tests/test$n.sh"; tail -20 "t$n.log"
  fi
done

# Known-answer probe through the SBN, DBF and SHP readers (dynamically linked binary).
cat > kat.expected <<'KAT'
sbn shapes=4 0 1 2 3
dbf records=4 fields=8
r0 f0=15001
r0 f1=Hawaii
r0 f2=HAWAII
r0 f3=4028.43700
r0 f4=2578199.68000
r0 f5=County
r0 f6=001
r0 f7=1
r1 f0=15003
r1 f1=Honolulu
r1 f2=HONOLULU
r1 f3=600.62600
r1 f4=384400.64000
r1 f5=County
r1 f6=003
r1 f7=3
shp entities=4 type=5 x=[-160.555771,-154.809379] y=[18.917466,22.233170]
obj0 parts=1 vertices=230
KAT
if "$O/kat_sbn" "$EG/CoHI_GCS12.sbn" "$EG/CoHI_GCS12.dbf" "$EG/CoHI_GCS12.shp" > kat.out 2>/dev/null && diff kat.expected kat.out; then
  passed=$((passed+1))
else
  failed=$((failed+1)); echo "FAIL: kat_sbn known-answer probe"
fi

# Direct assertions on the dump tools (not just a runner's exit code).
if "$SHPDUMP" "$EG/mexico/cities.shp" | grep -q "^Shapefile Type: Point   # of Shapes: 36" ; then passed=$((passed+1)); else failed=$((failed+1)); echo "FAIL: shpdump cities.shp header"; fi
if "$DBFDUMP" "$EG/CoHI_GCS12.dbf" | grep -q "Hawaii"; then passed=$((passed+1)); else failed=$((failed+1)); echo "FAIL: dbfdump CoHI Hawaii"; fi

emit_ctrf "shapelib-shell" "$passed" "$failed" 0
