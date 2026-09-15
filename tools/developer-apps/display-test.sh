#!/bin/sh
# Timed, non-launching display trial. No boot/theme/config changes.
BASE=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd) || exit 1
mkdir -p "$BASE/state" || exit 1
LOG="$BASE/state/display-test.log"
exec > "$LOG" 2>&1
echo 'DS Style v1.0.2 timed display test'
date
if ! command -v timeout >/dev/null 2>&1; then
    echo 'No stock timeout utility found; refusing an unbounded display trial.'
    exit 0
fi
LIMIT=20
# Shorten only in the isolated host test environment.
if [ -n "${DS_STYLE_TEST_ROOT:-}" ]; then LIMIT=${DS_STYLE_TEST_TIMEOUT:-20}; fi
case "$LIMIT" in ''|*[!0-9]*) echo 'Invalid test timeout'; exit 0 ;; esac
# Native code exits after 12 seconds; the independent stock timeout also covers
# startup stalls and escalates after two seconds. Games/apps cannot be launched.
timeout -s TERM -k 2 "$LIMIT" "$BASE/bin/dsstyle" --base "$BASE" --display-test
RC=$?
printf 'Display test process exit code: %s\n' "$RC"
echo 'Test complete; returning to stock APPS wrapper.'
# Do not trigger the stock scheduler failure delay for a diagnostic app.
exit 0
