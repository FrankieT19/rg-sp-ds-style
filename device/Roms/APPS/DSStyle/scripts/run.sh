#!/bin/sh
# DS_STYLE_V1: launch only our UI; stock services and global environment stay intact.
BASE=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd) || exit 1
mkdir -p "$BASE/state" || exit 1
LOG="$BASE/state/launch.log"
if [ -f "$LOG" ]; then cp "$LOG" "$BASE/state/launch-previous.log"; fi
{ echo 'DS Style v1.0 shell entry'; date; printf 'Base: %s\nMode: %s\n' "$BASE" "${1:-manual}"; } > "$LOG"
ROOT=${DS_STYLE_TEST_ROOT:-}
if [ -n "$ROOT" ]; then ROOT=$(CDPATH= cd -- "$ROOT" && pwd) || exit 1; fi
STOCK_SESSION="$ROOT/tmp/dsstyle-stock-session"
if [ "${1:-}" = --boot ] && [ -f "$STOCK_SESSION" ]; then
    exec "$ROOT/mnt/vendor/bin/dmenu.bin"
fi
ARCH=$(uname -m)
if [ -n "${DS_STYLE_TEST_ROOT:-}" ]; then ARCH=aarch64; fi
if [ "$ARCH" != aarch64 ]; then
    echo 'This device binary requires aarch64 Linux.' >> "$LOG"
    exit 1
fi
if [ "${1:-}" = --boot ]; then
    /bin/sh "$BASE/scripts/stock-session.sh" boot >> "$LOG" 2>&1
    "$BASE/bin/dsstyle" --base "$BASE" --splash >> "$LOG" 2>&1
else
    /bin/sh "$BASE/scripts/stock-session.sh" enter >> "$LOG" 2>&1
    "$BASE/bin/dsstyle" --base "$BASE" >> "$LOG" 2>&1
fi
RC=$?
/bin/sh "$BASE/scripts/stock-session.sh" leave >> "$LOG" 2>&1
if [ "$RC" = 42 ]; then mkdir -p "$ROOT/tmp"; : > "$STOCK_SESSION"; fi
printf 'Frontend exit code: %s\n' "$RC" >> "$LOG"
if [ "$RC" = 42 ] && [ "${1:-}" != --boot ]; then
    echo 'Return to existing stock scheduler; retain its selected theme.' >> "$LOG"
    exit 0
fi
exit "$RC"
