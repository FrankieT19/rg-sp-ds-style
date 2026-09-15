#!/bin/sh
# Stock applications only. No eval, no sourcing editable configuration.
set -u
BASE=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd) || exit 1
ROOT=${DS_STYLE_TEST_ROOT:-}
MODE=${1:-}
ROM=${2:-}
mkdir -p "$BASE/state" || exit 1
LOG="$BASE/state/last-launch.txt"
[ "$MODE" = plan ] || [ ! -x "$BASE/bin/dsstyle-stock-state" ] || "$BASE/bin/dsstyle-stock-state" "$BASE" save
trap '[ "$MODE" = plan ] || [ ! -x "$BASE/bin/dsstyle-stock-state" ] || "$BASE/bin/dsstyle-stock-state" "$BASE" save' EXIT
fail() { printf '%s\n' "$*" > "$LOG"; exit 20; }
. "$BASE/scripts/stock-profile.sh"
case "$MODE" in
retroarch)
    SYS=__MENU
    prepare_profile
    printf 'Open existing stock config: %s\nHOME policy: %s\n' "$CFG" "$PROFILE_HOME" > "$LOG"
    run_profile >> "$LOG" 2>&1
    ;;
ppsspp)
    EXE="$ROOT/mnt/vendor/deep/ppsspp/PPSSPPSDL"
    [ -x "$EXE" ] || fail 'Stock PPSSPP executable was not found.'
    printf 'Open stock PPSSPP: %s\n' "$EXE" > "$LOG"
    cd "$ROOT/mnt/vendor/deep/ppsspp" || exit 20
    "$EXE" >> "$LOG" 2>&1
    ;;
app)
    case "$ROM" in "$ROOT/mnt/mmc/Roms/APPS/"*.sh|"$ROOT/mnt/sdcard/Roms/APPS/"*.sh) ;; *) fail 'App must be an installed APPS shell script.' ;; esac
    [ -f "$ROM" ] || fail 'App was not found.'
    printf 'Stock app: %s\n' "$ROM" > "$LOG"
    cd "$(dirname -- "$ROM")" || exit 20
    # Honor an installed app's bash shebang, as stock mod apps often use bash.
    FIRST=$(head -n 1 "$ROM")
    case "$FIRST" in *bash*) SHELL_APP=/bin/bash ;; *) SHELL_APP=/bin/sh ;; esac
    "$SHELL_APP" "$ROM" >> "$LOG" 2>&1
    ;;
game|plan)
    [ -f "$ROM" ] || fail 'ROM was not found.'
    case "$ROM" in
        "$ROOT/mnt/mmc/Roms/"*) REL=${ROM#"$ROOT/mnt/mmc/Roms/"} ;;
        "$ROOT/mnt/sdcard/Roms/"*) REL=${ROM#"$ROOT/mnt/sdcard/Roms/"} ;;
        *) fail 'ROM must be inside a stock Roms directory.' ;;
    esac
    SYS=${REL%%/*}
    [ "$SYS" != "$REL" ] || fail 'Place ROMs inside a system folder such as Roms/GBA.'
    case "$REL" in ../*|*/../*|./*|*/./*) fail 'ROM path must not contain traversal components.' ;; esac
    case "$SYS" in *[!A-Za-z0-9_-]*) fail 'Unsupported system folder name.' ;; esac
    CHOICE=retroarch
    if [ -f "$BASE/state/launch-modes/$SYS.txt" ]; then
        IFS= read -r CHOICE < "$BASE/state/launch-modes/$SYS.txt" || true
        CHOICE=$(printf '%s' "$CHOICE" | tr -d '\r')
    fi
    # A shared favourite/history record retains stock's RA/Game Rooms identity.
    case "${3:-}" in stock-room) CHOICE=gameroom ;; stock-ra) CHOICE=retroarch ;; "") : ;; *) fail 'Invalid stock collection route.' ;; esac
    if [ "$CHOICE" = gameroom ]; then
        [ -x "$BASE/bin/dsstyle-stock-room" ] || fail 'Stock Game Rooms reader is missing.'
        export LD_LIBRARY_PATH=/usr/lib32:/usr/lib:/mnt/vendor/lib
        "$BASE/bin/dsstyle-stock-room" "$MODE" "$SYS" "$ROM" > "$LOG" 2>&1
        RC=$?
        [ "$MODE" != plan ] || cat "$LOG"
        exit "$RC"
    fi
    [ "$CHOICE" = retroarch ] || fail 'Invalid saved launch mode.'
    [ -x "$BASE/bin/dsstyle-stock-ra" ] || fail 'Stock menu reader is missing.'
    (
        unset HOME XDG_CONFIG_HOME SDL_VIDEODRIVER SDL_AUDIODRIVER
        export LD_LIBRARY_PATH=/usr/lib32:/usr/lib:/mnt/vendor/lib
        "$BASE/bin/dsstyle-stock-ra" "$MODE" "$SYS" "$ROM"
    ) > "$LOG" 2>&1
    RC=$?
    [ "$MODE" != plan ] || cat "$LOG"
    exit "$RC"
    ;;
*) fail "Unknown launch mode: $MODE" ;;
esac
RC=$?
printf '\nApplication exit code: %s\n' "$RC" >> "$LOG"
exit "$RC"
