#!/bin/sh
# Run libuv's own tests against the host build of the AmigaOS backend
# configuration (build/host/uv-run-tests), one test per process, recording
# each verdict as it finishes in build/host/uv-tests.tsv. Resumable: tests
# already recorded are skipped.
#   tools/uv-host-tests.sh                  every test not yet recorded
#   tools/uv-host-tests.sh name...          just these (re-run, re-recorded)
#   tools/uv-host-tests.sh --failed         re-run the recorded failures
# UVT=v012 runs the 0.12 port's libuv 1.52 (build/v012/host) instead of
# the 0.4.4 baseline's libuv 1.30 (build/host).
set -u
ROOT=$(cd "$(dirname "$0")/.." && pwd)
if [ "${UVT:-}" = v012 ]; then
	HB=$ROOT/build/v012/host; UVSRC=$ROOT/vendor012/libuv
else
	HB=$ROOT/build/host; UVSRC=$ROOT/vendor/libuv
fi
RUN=$HB/uv-run-tests
LOG=$HB/uv-tests.tsv
OUT=$HB/uv-tests-out
mkdir -p "$OUT"
touch "$LOG"
# tests open test/fixtures/... relative to the cwd and leave files there
# (watch_dir, watch_file): a scratch cwd in build/, not the vendor tree
CWD=$HB/uv-cwd
mkdir -p "$CWD" && ln -sfn "$UVSRC/test" "$CWD/test"
cd "$CWD" || exit 1

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
