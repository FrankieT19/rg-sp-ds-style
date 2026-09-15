#!/bin/sh
# DS_STYLE_V1 boot installation: preserve and restore a previous override exactly.
set -u
BASE=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd) || exit 1
ROOT=${DS_STYLE_TEST_ROOT:-}
SD1="$ROOT/mnt/mmc"
TARGET="$SD1/dmenu.bin"
BACKUP="$SD1/.dsstyle-v1-boot"
MODE=${1:-status}
mkdir -p "$BASE/state" || exit 1
exec >> "$BASE/state/boot-manager.log" 2>&1
fail() { printf 'ERROR: %s\n' "$*"; exit 1; }
if [ -z "$ROOT" ]; then
    mountpoint -q "$SD1" || fail 'TF1 ROMs partition is not mounted.'
fi
[ -d "$SD1" ] || fail 'TF1 ROMs partition is absent.'
case "$MODE" in
enable)
    [ -f "$BASE/bin/dsstyle" ] || fail 'DS Style binary missing.'
    if [ -d "$BACKUP" ]; then
        if [ -f "$BACKUP/installed" ] && cmp -s "$TARGET" "$BACKUP/installed"; then
            rm -f "$BASE/DISABLED"
            echo 'Already enabled.'; exit 0
        fi
        fail 'Existing boot backup requires recovery; refusing to replace another launcher.'
    fi
    [ ! -L "$TARGET" ] || fail 'Existing override is a symlink; leave it unchanged.'
    [ ! -e "$TARGET" ] || [ -f "$TARGET" ] || fail 'Existing override is not a regular file.'
    if [ -f "$ROOT/mnt/vendor/muos1.ini" ] || [ -f "$ROOT/mnt/vendor/muos2.ini" ]; then
        fail 'Select old style in stock first. DS Style does not change the theme setting.'
    fi
    # mkdir is the ownership claim; no force/overwrite operation on backups.
    mkdir "$BACKUP" || fail 'Could not create rollback directory.'
    if [ -f "$TARGET" ]; then
        cp -p "$TARGET" "$BACKUP/original" || fail 'Backup failed; override unchanged.'
        cmp -s "$TARGET" "$BACKUP/original" || fail 'Backup verification failed.'
        printf 'present\n' > "$BACKUP/original-state"
    else
        printf 'absent\n' > "$BACKUP/original-state"
    fi
    cp "$BASE/scripts/boot-shim.sh" "$BACKUP/installed" || fail 'Cannot stage shim.'
    cp "$BACKUP/installed" "$SD1/.dsstyle-dmenu.tmp" || fail 'Cannot stage boot override.'
    chmod +x "$SD1/.dsstyle-dmenu.tmp" || fail 'Cannot make shim executable.'
    mv "$SD1/.dsstyle-dmenu.tmp" "$TARGET" || fail 'Cannot install boot override.'
    rm -f "$BASE/DISABLED"
    sync
    echo 'Enabled. Firmware and vendor files unchanged.'
    ;;
disable)
    [ -d "$BACKUP" ] || { echo 'No DS Style boot install exists.'; exit 0; }
    [ -f "$BACKUP/installed" ] || fail 'Rollback incomplete; inspect manually.'
    cmp -s "$TARGET" "$BACKUP/installed" || fail 'Boot override changed since install; refusing to overwrite it.'
    STATE=$(cat "$BACKUP/original-state" 2>/dev/null) || fail 'Rollback metadata missing.'
    case "$STATE" in
        present)
            [ -f "$BACKUP/original" ] || fail 'Original override backup missing.'
            cp -p "$BACKUP/original" "$SD1/.dsstyle-restore.tmp" || fail 'Cannot stage original.'
            cmp -s "$BACKUP/original" "$SD1/.dsstyle-restore.tmp" || fail 'Restore verification failed.'
            mv "$SD1/.dsstyle-restore.tmp" "$TARGET" || fail 'Cannot restore original.'
            ;;
        absent) rm "$TARGET" || fail 'Cannot remove our override.' ;;
        *) fail 'Invalid rollback metadata.' ;;
    esac
    # Keep recovery evidence, allowing a later fresh enable without deleting backups.
    ARCHIVE="$SD1/.dsstyle-v1-restored-$(date +%s)-$$"
    mv "$BACKUP" "$ARCHIVE" || fail 'Restored boot, but could not archive recovery files.'
    sync
    echo 'Disabled; prior boot override restored. Recovery archive retained on TF1.'
    ;;
status)
    if [ -f "$BACKUP/installed" ] && cmp -s "$TARGET" "$BACKUP/installed"; then echo enabled; else echo 'not managed by DS Style'; fi
    ;;
*) fail 'Expected enable, disable, or status.' ;;
esac
