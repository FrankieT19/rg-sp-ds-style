#!/bin/sh
APPDIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
exec /bin/sh "$APPDIR/DSStyle/scripts/run.sh"
