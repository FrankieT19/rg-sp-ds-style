#!/bin/sh
# Same synchronous entry point that stock dmenu uses for lid and idle sleep.
# No alternate blank-screen mode, sysfs suspend writes, or service replacement.
BASE=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd) || exit 1
ROOT=${DS_STYLE_TEST_ROOT:-}
mkdir -p "$BASE/state/stock-power"
for item in vendor mod; do
    [ ! -f "$ROOT/mnt/$item/ctrl/pwr_new.sh" ] || head -c 65536 "$ROOT/mnt/$item/ctrl/pwr_new.sh" > "$BASE/state/stock-power/$item-pwr_new.sh"
done
[ ! -f "$ROOT/mnt/data/dmenu/dmenu_attr.ini" ] || head -c 1024 "$ROOT/mnt/data/dmenu/dmenu_attr.ini" > "$BASE/state/stock-power/dmenu_attr.bin"
[ -f "$ROOT/mnt/vendor/ctrl/pwr_new.sh" ] || exit 1
if [ -x "$BASE/bin/dsstyle-stock-state" ]; then "$BASE/bin/dsstyle-stock-state" "$BASE" sleep-policy; fi
exec /bin/sh "$ROOT/mnt/vendor/ctrl/pwr_new.sh" auto
