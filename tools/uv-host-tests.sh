#!/bin/sh
# Run libuv's own tests against the host build of the AmigaOS backend
# configuration (build/host/uv-run-tests), one test per process, recording
# each verdict as it finishes in build/host/uv-tests.tsv. Resumable: tests
# already recorded are skipped.
#   tools/uv-host-tests.sh                  every test not yet recorded
#   tools/uv-host-tests.sh name...          just these (re-run, re-recorded)
#   tools/uv-host-tests.sh --failed         re-run the recorded failures
set -u
ROOT=$(cd "$(dirname "$0")/.." && pwd)
RUN=$ROOT/build/host/uv-run-tests
LOG=$ROOT/build/host/uv-tests.tsv
OUT=$ROOT/build/host/uv-tests-out
mkdir -p "$OUT"
touch "$LOG"
cd "$ROOT/vendor/libuv" || exit 1    # tests open test/fixtures/... from here

if [ $# -gt 0 ] && [ "$1" = "--failed" ]; then
	set -- $(awk -F'\t' '$2 ~ /^fail/ {print $1}' "$LOG")
fi
if [ $# -gt 0 ]; then
	list="$*"
	for t in $list; do
		grep -v "^$t	" "$LOG" > "$LOG.tmp"; mv "$LOG.tmp" "$LOG"
	done
else
	list=$("$RUN" --list 2>/dev/null | awk '{print $1}')
fi

for t in $list; do
	grep -q "^$t	" "$LOG" && continue
	"$RUN" "$t" > "$OUT/$t.txt" 2>&1
	rc=$?
	if grep -q "# SKIP" "$OUT/$t.txt"; then v=skip
	elif [ $rc -eq 0 ]; then v=pass; else v="fail($rc)"; fi
	printf '%s\t%s\n' "$t" "$v" >> "$LOG"
done
awk -F'\t' '{n++; if ($2=="pass") p++; if ($2=="skip") k++} END {printf "%d of %d pass, %d skip, %d fail\n", p, n, k, n-p-k}' "$LOG"
